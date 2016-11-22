/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include "DevFs.h"

#define MACHINE_FORWARD_DECL_ONLY
#include <machine/Machine.h>
#include <machine/Vga.h>

#include <console/Console.h>
#include <utilities/assert.h>

#include <graphics/Graphics.h>
#include <graphics/GraphicsService.h>

#include <utilities/utility.h>

#include <sys/fb.h>

uint64_t RandomFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    /// \todo Endianness issues?

    size_t realSize = size;

    if(size < sizeof(uint64_t))
    {
        uint64_t val = random_next();
        char *pBuffer = reinterpret_cast<char *>(buffer);
        while(size--)
        {
            *pBuffer++ = val & 0xFF;
            val >>= 8;
        }
    }
    else
    {
        // Align.
        char *pBuffer = reinterpret_cast<char *>(buffer);
        if(size % 8)
        {
            uint64_t align = random_next();
            while(size % 8)
            {
                *pBuffer++ = align & 0xFF;
                --size;
                align >>= 8;
            }
        }

        uint64_t *pBuffer64 = reinterpret_cast<uint64_t *>(buffer);
        while(size)
        {
            *pBuffer64++ = random_next();
            size -= 8;
        }
    }

    return realSize - size;
}

uint64_t RandomFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return 0;
}

uint64_t NullFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return 0;
}

uint64_t NullFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return size;
}

PtmxFile::PtmxFile(String str, size_t inode, Filesystem *pParentFS, File *pParent, DevFsDirectory *ptsDirectory) :
    File(str, 0, 0, 0, inode, pParentFS, 0, pParent), m_Terminals(), m_pPtsDirectory(ptsDirectory)
{
    setPermissionsOnly(FILE_UR | FILE_UW | FILE_GR | FILE_GW | FILE_OR | FILE_OW);
    setUidOnly(0);
    setGidOnly(0);
}

PtmxFile::~PtmxFile()
{
    delete m_pPtsDirectory;
}

uint64_t PtmxFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return 0;
}

uint64_t PtmxFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return 0;
}

File *PtmxFile::open()
{
    // find a new terminal ID that we can safely use
    size_t terminal = m_Terminals.getFirstClear();
    m_Terminals.set(terminal);

    // create the terminals
    String masterName, slaveName;
    masterName.Format("pty%d", terminal);
    slaveName.Format("%d", terminal);

    ConsoleMasterFile *pMaster = new ConsoleMasterFile(terminal, masterName, m_pPtsDirectory->getFilesystem());
    ConsoleSlaveFile *pSlave = new ConsoleSlaveFile(terminal, slaveName, m_pPtsDirectory->getFilesystem());

    pMaster->setOther(pSlave);
    pSlave->setOther(pMaster);

    m_pPtsDirectory->addEntry(slaveName, pSlave);

    // we actually open the newly-created master, which does not exist in the
    // filesystem at all
    /// \todo so, when this master is closed, we'll leak these resources...
    return pMaster;
}

uint64_t ZeroFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    ByteSet(reinterpret_cast<void *>(buffer), 0, size);
    return size;
}

uint64_t ZeroFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return size;
}

FramebufferFile::FramebufferFile(String str, size_t inode, Filesystem *pParentFS, File *pParentNode) :
    File(str, 0, 0, 0, inode, pParentFS, 0, pParentNode), m_pGraphicsParameters(0), m_bTextMode(false), m_nDepth(0)
{
    // r/w only for root
    setPermissionsOnly(FILE_GR | FILE_GW | FILE_UR | FILE_UW);
    setUidOnly(0);
    setGidOnly(0);
}

FramebufferFile::~FramebufferFile()
{
    delete m_pGraphicsParameters;
}

bool FramebufferFile::initialise()
{
    ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("graphics"));
    Service         *pService  = ServiceManager::instance().getService(String("graphics"));
    if(pFeatures && pFeatures->provides(ServiceFeatures::probe))
    {
        if(pService)
        {
            m_pGraphicsParameters = new GraphicsService::GraphicsParameters;
            m_pGraphicsParameters->wantTextMode = false;
            if(!pService->serve(ServiceFeatures::probe, m_pGraphicsParameters, sizeof(*m_pGraphicsParameters)))
            {
                delete m_pGraphicsParameters;
                m_pGraphicsParameters = 0;

                return false;
            }
            else
            {
                // Set the file size to reflect the size of the framebuffer.
                setSize(m_pGraphicsParameters->providerResult.pFramebuffer->getHeight() * m_pGraphicsParameters->providerResult.pFramebuffer->getBytesPerLine());
            }
        }
    }

    return pFeatures && pService;
}

uintptr_t FramebufferFile::readBlock(uint64_t location)
{
    if(!m_pGraphicsParameters)
        return 0;

    if(location > getSize())
    {
        ERROR("FramebufferFile::readBlock with location > size: " << location);
        return 0;
    }

    /// \todo If this is NOT virtual, we need to do something about that.
    return reinterpret_cast<uintptr_t>(m_pGraphicsParameters->providerResult.pFramebuffer->getRawBuffer()) + location;
}

bool FramebufferFile::supports(const int command)
{
    return (PEDIGREE_FB_CMD_MIN <= command) && (command <= PEDIGREE_FB_CMD_MAX);
}

int FramebufferFile::command(const int command, void *buffer)
{
    if(!m_pGraphicsParameters)
    {
        ERROR("FramebufferFile::command called on an invalid FramebufferFile");
        return -1;
    }

    Display *pDisplay = m_pGraphicsParameters->providerResult.pDisplay;
    Framebuffer *pFramebuffer = m_pGraphicsParameters->providerResult.pFramebuffer;

    switch(command)
    {
        case PEDIGREE_FB_SETMODE:
            {
                pedigree_fb_modeset *arg = reinterpret_cast<pedigree_fb_modeset *>(buffer);
                size_t desiredWidth = arg->width;
                size_t desiredHeight = arg->height;
                size_t desiredDepth = arg->depth;

                // Are we seeking a text mode?
                if(!(desiredWidth && desiredHeight && desiredDepth))
                {
                    bool bSuccess = false;
                    if(!m_pGraphicsParameters->providerResult.bTextModes)
                    {
                        bSuccess = pDisplay->setScreenMode(0);
                    }
                    else
                    {
                        // Set via VGA method.
                        if(Machine::instance().getNumVga())
                        {
                            /// \todo What if there is no text mode!?
                            Vga *pVga = Machine::instance().getVga(0);
                            pVga->setMode(3); /// \todo Magic number.
                            pVga->rememberMode();
                            pVga->setLargestTextMode();

                            m_nDepth = 0;
                            m_bTextMode = true;

                            bSuccess = true;
                        }
                    }

                    if(bSuccess)
                    {
                        NOTICE("FramebufferFile: set text mode");
                        return 0;
                    }
                    else
                    {
                        return -1;
                    }
                }

                bool bSet = false;
                while(desiredDepth > 8)
                {
                    if(pDisplay->setScreenMode(desiredWidth, desiredHeight, desiredDepth))
                    {
                        NOTICE("FramebufferFile: set mode " << Dec << desiredWidth << "x" << desiredHeight << "x" << desiredDepth << Hex << ".");
                        bSet = true;
                        break;
                    }
                    desiredDepth -= 8;
                }

                if(bSet)
                {
                    m_nDepth = desiredDepth;

                    setSize(pFramebuffer->getHeight() * pFramebuffer->getBytesPerLine());

                    if(m_pGraphicsParameters->providerResult.bTextModes && m_bTextMode)
                    {
                        // Okay, we need to 'undo' the text mode.
                        if(Machine::instance().getNumVga())
                        {
                            /// \todo What if there is no text mode!?
                            Vga *pVga = Machine::instance().getVga(0);
                            pVga->restoreMode();

                            m_bTextMode = false;
                        }
                    }
                }

                return bSet ? 0 : -1;
            }
        case PEDIGREE_FB_GETMODE:
            {
                pedigree_fb_mode *arg = reinterpret_cast<pedigree_fb_mode *>(buffer);
                if(m_bTextMode)
                {
                    ByteSet(arg, 0, sizeof(*arg));
                }
                else
                {
                    arg->width = pFramebuffer->getWidth();
                    arg->height = pFramebuffer->getHeight();
                    arg->depth = m_nDepth;
                    arg->bytes_per_pixel = pFramebuffer->getBytesPerPixel();
                    arg->format = pFramebuffer->getFormat();
                }

                return 0;
            }
        case PEDIGREE_FB_REDRAW:
            {
                pedigree_fb_rect *arg = reinterpret_cast<pedigree_fb_rect *>(buffer);
                if(!arg)
                {
                    // Redraw all.
                    pFramebuffer->redraw(0, 0, pFramebuffer->getWidth(), pFramebuffer->getHeight(), true);
                }
                else
                {
                    pFramebuffer->redraw(arg->x, arg->y, arg->w, arg->h, true);
                }

                return 0;
            }
        default:
            return -1;
    }
}

DevFs::~DevFs()
{
    delete m_pTty;
    delete m_pRoot;
}

bool DevFs::initialise(Disk *pDisk)
{
    // Deterministic inode assignment to each devfs node
    m_NextInode = 0;

    if (m_pRoot)
    {
        delete m_pRoot;
    }

    m_pRoot = new DevFsDirectory(String(""), 0, 0, 0, getNextInode(), this, 0, 0);
    // Allow user/group to read and write, but disallow all others anything
    // other than the ability to list and access files.
    m_pRoot->setPermissions(FILE_UR | FILE_UW | FILE_UX | FILE_GR | FILE_GW | FILE_GX | FILE_OR | FILE_OX);

    // Create /dev/null and /dev/zero nodes
    NullFile *pNull = new NullFile(String("null"), getNextInode(), this, m_pRoot);
    ZeroFile *pZero = new ZeroFile(String("zero"), getNextInode(), this, m_pRoot);
    m_pRoot->addEntry(pNull->getName(), pNull);
    m_pRoot->addEntry(pZero->getName(), pZero);

    // Create the /dev/pts directory for ptys to go into.
    DevFsDirectory *pPts = new DevFsDirectory(String("pts"), 0, 0, 0, getNextInode(), this, 0, m_pRoot);
    pPts->setPermissions(FILE_UR | FILE_UW | FILE_UX | FILE_GR | FILE_GX | FILE_OR | FILE_OX);
    m_pRoot->addEntry(pPts->getName(), pPts);

    // Create the /dev/ptmx device.
    PtmxFile *pPtmx = new PtmxFile(String("ptmx"), getNextInode(), this, m_pRoot, pPts);
    m_pRoot->addEntry(pPtmx->getName(), pPtmx);

    // Create the /dev/pts directory for the terminals the ptmx file will
    // create as side-effects of open().
    //

    // Create nodes for psuedoterminals.
    /*
    const char *ptyletters = "pqrstuvwxyzabcde";
    for(size_t i = 0; i < 16; ++i)
    {
        for(const char *ptyc = ptyletters; *ptyc != 0; ++ptyc)
        {
            const char c = *ptyc;
            char a = 'a' + (i % 10);
            if(i <= 9)
                a = '0' + i;
            char master[] = {'p', 't', 'y', c, a, 0};
            char slave[] = {'t', 't', 'y', c, a, 0};

            String masterName(master), slaveName(slave);

            File *pMaster = ConsoleManager::instance().getConsole(masterName);
            File *pSlave = ConsoleManager::instance().getConsole(slaveName);
            assert(pMaster && pSlave);

            m_pRoot->addEntry(masterName, pMaster);
            m_pRoot->addEntry(slaveName, pSlave);
        }
    }
    */

    // Create /dev/urandom for the RNG.
    RandomFile *pRandom = new RandomFile(String("urandom"), getNextInode(), this, m_pRoot);
    m_pRoot->addEntry(pRandom->getName(), pRandom);

    // Create /dev/fb for the framebuffer device.
    FramebufferFile *pFb = new FramebufferFile(String("fb"), getNextInode(), this, m_pRoot);
    if(pFb->initialise())
        m_pRoot->addEntry(pFb->getName(), pFb);
    else
    {
        WARNING("POSIX: no /dev/fb - framebuffer failed to initialise.");
        revertInode();
        delete pFb;
    }

    // Create /dev/textui for the text-only UI device.
    m_pTty = new TextIO(String("textui"), getNextInode(), this, m_pRoot);
    if(m_pTty->initialise(false))
    {
        m_pRoot->addEntry(m_pTty->getName(), m_pTty);
    }
    else
    {
        WARNING("POSIX: no /dev/tty - TextIO failed to initialise.");
        revertInode();
        delete m_pTty;
    }

    ConsolePhysicalFile *pTty0 = new ConsolePhysicalFile(m_pTty, String("tty0"), this);
    m_pRoot->addEntry(pTty0->getName(), pTty0);

    return true;
}

size_t DevFs::getNextInode()
{
    return m_NextInode++;
}

void DevFs::revertInode()
{
    --m_NextInode;
}
