/*
 * Copyright (c) 2010 Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <Log.h>
#include <Module.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <utilities/StaticString.h>
#include <processor/MemoryRegion.h>
#include <utilities/List.h>
#include <machine/Machine.h>
#include <machine/Display.h>
#include <config/Config.h>

#include "vm_device_version.h"
#include "svga_reg.h"

#define DEBUG_VMWARE_GFX        1

// Can refer to http://sourceware.org/ml/ecos-devel/2006-10/msg00008/README.xfree86
// for information about programming this device, as well as the Haiku driver.

class VmwareGraphics : public Display
{
    public:
        VmwareGraphics(Device *pDev) :
            Device(pDev), Display(pDev), m_pIo(0),
            m_Framebuffer("vmware-gfx-framebuffer"),
            m_CommandRegion("vmware-gfx-command")
        {
            m_pIo = m_Addresses[0]->m_Io;
            
            // Support ID2, as we are expecting an SVGA2 functional device
            writeRegister(SVGA_REG_ID, SVGA_MAKE_ID(2));
            if(readRegister(SVGA_REG_ID) != SVGA_MAKE_ID(2))
            {
                WARNING("vmware-gfx not a compatible SVGA device");
                return;
            }
            
            // Read the framebuffer base (should match BAR1)
            uintptr_t fbBase = readRegister(SVGA_REG_FB_START);
            size_t fbSize = readRegister(SVGA_REG_VRAM_SIZE);
            
            // Read the command FIFO (should match BAR2)
            uintptr_t cmdBase = readRegister(SVGA_REG_MEM_START);
            size_t cmdSize = readRegister(SVGA_REG_MEM_SIZE);
            
            // Find the capabilities of the device
            size_t caps = readRegister(SVGA_REG_CAPABILITIES);
            
            // Read maximum resolution
            size_t maxWidth = readRegister(SVGA_REG_MAX_WIDTH);
            size_t maxHeight = readRegister(SVGA_REG_MAX_HEIGHT);
            
            // Tell VMware what OS we are - "Other"
            writeRegister(SVGA_REG_GUEST_ID, 0x500A);
            
            // Debug notification
            NOTICE("vmware-gfx found, caps=" << caps << ", maximum resolution is " << Dec << maxWidth << "x" << maxHeight << Hex);
            NOTICE("vmware-gfx        framebuffer at " << fbBase << " - " << (fbBase + fbSize) << ", command FIFO at " << cmdBase);
            
            PhysicalMemoryManager::instance().allocateRegion(m_Framebuffer, (fbSize / 0x1000) + 1, PhysicalMemoryManager::continuous, VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write, fbBase);
            PhysicalMemoryManager::instance().allocateRegion(m_CommandRegion, (cmdSize / 0x1000) + 1, PhysicalMemoryManager::continuous, VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write, cmdBase);
            
            // Set mode
            writeRegister(SVGA_REG_WIDTH, 1023);
            writeRegister(SVGA_REG_HEIGHT, 769);
            writeRegister(SVGA_REG_BITS_PER_PIXEL, 32);
            
            size_t fbOffset = readRegister(SVGA_REG_FB_OFFSET);
            size_t bytesPerLine = readRegister(SVGA_REG_BYTES_PER_LINE);
            size_t depth = readRegister(SVGA_REG_DEPTH);
            
            NOTICE("vmware-gfx entered mode 1024x768x16, mode framebuffer is " << (fbBase + fbOffset));
            
            // Disable the command FIFO in case it was already enabled
            writeRegister(SVGA_REG_CONFIG_DONE, 0);
            
            // Enable the SVGA now
            writeRegister(SVGA_REG_ENABLE, 1);
            
            // Initialise the FIFO
            volatile uint32_t *fifo = reinterpret_cast<volatile uint32_t*>(m_CommandRegion.virtualAddress());
            
            if(caps & SVGA_CAP_EXTENDED_FIFO)
            {
                size_t numFifoRegs = readRegister(SVGA_REG_MEM_REGS);
                size_t fifoExtendedBase = numFifoRegs * 4;
            
                fifo[SVGA_FIFO_MIN] = fifoExtendedBase; // Start right after this information block
                fifo[SVGA_FIFO_MAX] = cmdSize & ~0x3; // Permit the full FIFO to be used
                fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_STOP] = fifo[SVGA_FIFO_MIN]; // Empty FIFO
                
                NOTICE("vmware-gfx using extended fifo, caps=" << fifo[SVGA_FIFO_CAPABILITIES] << ", flags=" << fifo[SVGA_FIFO_FLAGS]);
            }
            else
            {
                fifo[SVGA_FIFO_MIN] = 16; // Start right after this information block
                fifo[SVGA_FIFO_MAX] = cmdSize & ~0x3; // Permit the full FIFO to be used
                fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_STOP] = 16; // Empty FIFO
            }
            
            // Try some hardware accelerated stuff
            if(caps & SVGA_CAP_RECT_FILL)
            {
                NOTICE("vmware-gfx rect fill supported");
            
                writeFifo(2); // RECT FILL
                writeFifo(0xFFFFFFFF); // White
                writeFifo(64); // X
                writeFifo(64); // Y
                writeFifo(256); // Width
                writeFifo(256); // Height
            }
            else
                NOTICE("vmware-gfx no rect fill available");
            
            if(caps & SVGA_CAP_RECT_COPY)
            {
                NOTICE("vmware-gfx rect copy supported");
                
                writeFifo(3); // RECT COPY
                writeFifo(8); // Source X
                writeFifo(8); // Source Y
                writeFifo(256); // Dest X
                writeFifo(256); // Dest Y
                writeFifo(256); // Width
                writeFifo(256); // Height
            }
            else
                NOTICE("vmware-gfx no rect copy available");
            
            // Start running the FIFO
            writeRegister(SVGA_REG_CONFIG_DONE, 1);
        }
        
        virtual ~VmwareGraphics()
        {
        }
        
    private:
    
        IoBase *m_pIo;
        
        /// \todo Lock these
        
        size_t readRegister(size_t offset)
        {
            m_pIo->write32(offset, SVGA_INDEX_PORT);
            return m_pIo->read32(SVGA_VALUE_PORT);
        }
        
        void writeRegister(size_t offset, uint32_t value)
        {
            m_pIo->write32(offset, SVGA_INDEX_PORT);
            m_pIo->write32(value, SVGA_VALUE_PORT);
        }
        
        void writeFifo(uint32_t value)
        {
            volatile uint32_t *fifo = reinterpret_cast<volatile uint32_t*>(m_CommandRegion.virtualAddress());
            
            // Check for sync conditions
            if(((fifo[SVGA_FIFO_NEXT_CMD] + 4) == fifo[SVGA_FIFO_STOP]) ||
                (fifo[SVGA_FIFO_NEXT_CMD] == (fifo[SVGA_FIFO_MAX] - 4)))
            {
#if DEBUG_VMWARE_GFX
                DEBUG_LOG("vmware-gfx synchronising full fifo");
#endif
                syncFifo();
            }
            
#if DEBUG_VMWARE_GFX
            DEBUG_LOG("vmware-gfx fifo write at " << fifo[SVGA_FIFO_NEXT_CMD] << " val=" << value);
            DEBUG_LOG("vmware-gfx min is at " << fifo[SVGA_FIFO_MIN] << " and current position is " << fifo[SVGA_FIFO_STOP]);
#endif
            
            // Load the new item into the FIFO
            fifo[fifo[SVGA_FIFO_NEXT_CMD] / 4] = value;
            if(fifo[SVGA_FIFO_NEXT_CMD] == (fifo[SVGA_FIFO_MAX] - 4))
                fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_MIN];
            else
                fifo[SVGA_FIFO_NEXT_CMD] += 4;
            
            asm volatile("" : : : "memory");
        }
        
        void syncFifo()
        {
            writeRegister(SVGA_REG_SYNC, 1);
            while(readRegister(SVGA_REG_BUSY));
        }
        
        MemoryRegion m_Framebuffer;
        MemoryRegion m_CommandRegion;
};

void callback(Device *pDevice)
{
    VmwareGraphics *pGraphics = new VmwareGraphics(pDevice);
}

void entry()
{
    // Don't care about non-SVGA2 devices, just use VBE for them.
    Device::root().searchByVendorIdAndDeviceId(PCI_VENDOR_ID_VMWARE, PCI_DEVICE_ID_VMWARE_SVGA2, callback);
    
    while(1) asm volatile("hlt");
}

void exit()
{
}

MODULE_INFO("vmware-gfx", &entry, &exit, "pci", "config");
