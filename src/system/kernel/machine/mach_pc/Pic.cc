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

#include <compiler.h>
#include <Log.h>
#include <Debugger.h>
#include "Pic.h"
#include <LockGuard.h>

// TODO: Needs locking

#define BASE_INTERRUPT_VECTOR 0x20

// Number of IRQs in a single millisecond before an IRQ source is blocked.
// A value of 10, for example, would mean if an IRQ matches the threshold
// and sustained its output for a second, 10,000 IRQs would be triggered.
#define DEFAULT_IRQ_MITIGATE_THRESHOLD      10

Pic Pic::m_Instance;

void Pic::tick()
{
}

bool Pic::control(uint8_t irq, ControlCode code, size_t argument)
{
    if(UNLIKELY(irq > 16))
        return false;

    Spinlock lock;
    LockGuard<Spinlock> guard(lock);
    
    switch(code)
    {
        case MitigationThreshold:
            if(LIKELY(argument))
            {
                if(UNLIKELY(m_Handler[irq].count() > 1))
                    m_MitigationThreshold[irq] += argument;
                else
                    m_MitigationThreshold[irq] = argument;
            }
            else
                m_MitigationThreshold[irq] = DEFAULT_IRQ_MITIGATE_THRESHOLD;
            return true;
    }
    
    return false;
}

irq_id_t Pic::registerIsaIrqHandler(uint8_t irq, IrqHandler *handler, bool bEdge)
{
  if (UNLIKELY(irq >= 16))
    return 0;

  // Save the IrqHandler
  m_Handler[irq].pushBack(handler);
  m_HandlerEdge[irq] = bEdge;

  // Enable/Unmask the IRQ
  enable(irq, true);

  return irq + BASE_INTERRUPT_VECTOR;
}
irq_id_t Pic::registerPciIrqHandler(IrqHandler *handler, Device *pDevice)
{
  if (UNLIKELY(!pDevice))
    return 0;
  irq_id_t irq = pDevice->getInterruptNumber();
  if (UNLIKELY(irq >= 16))
    return 0;

  // Save the IrqHandler
  m_Handler[irq].pushBack(handler);
  m_HandlerEdge[irq] = false; // PCI bus uses level triggered IRQs

  // Enable/Unmask the IRQ
  enable(irq, true);

  return irq + BASE_INTERRUPT_VECTOR;
}
void Pic::acknowledgeIrq(irq_id_t Id)
{
  uint8_t irq = Id - BASE_INTERRUPT_VECTOR;

  // Enable the irq again (the interrupt reason got removed)
  enable(irq, true);
  eoi(irq);
}
void Pic::unregisterHandler(irq_id_t Id, IrqHandler *handler)
{
  uint8_t irq = Id - BASE_INTERRUPT_VECTOR;

  // Disable the IRQ
  if(!m_Handler[irq].count())
      enable(irq, false);

  // Remove the handler
  for(List<IrqHandler*>::Iterator it = m_Handler[irq].begin();
      it != m_Handler[irq].end();
      it++)
  {
    if(*it == handler)
    {
        m_Handler[irq].erase(it);
        return;
    }
  }
}

bool Pic::initialise()
{
  // Allocate the I/O ports
  if (m_SlavePort.allocate(0xA0, 4) == false)
    return false;
  if (m_MasterPort.allocate(0x20, 4) == false)
    return false;

  // Initialise the slave and master PIC
  m_MasterPort.write8(0x11, 0);
  m_SlavePort.write8(0x11, 0);
  m_MasterPort.write8(BASE_INTERRUPT_VECTOR, 1);
  m_SlavePort.write8(BASE_INTERRUPT_VECTOR + 0x08, 1);
  m_MasterPort.write8(0x04, 1);
  m_SlavePort.write8(0x02, 1);
  m_MasterPort.write8(0x01, 1);
  m_SlavePort.write8(0x01, 1);
  m_MasterPort.write8(0x00, 1);
  m_SlavePort.write8(0x00, 1);

  // Register the interrupts
  InterruptManager &IntManager = InterruptManager::instance();
  for (size_t i = 0;i < 16;i++)
    if (IntManager.registerInterruptHandler(i + BASE_INTERRUPT_VECTOR, this) == false)
      return false;

  for(size_t i = 0; i < 16; i++)
  {
    m_IrqCount[i] = 0;
    m_MitigatedIrqs[i] = false;
    m_MitigationThreshold[i] = DEFAULT_IRQ_MITIGATE_THRESHOLD;
  }

  // Disable all IRQ's (exept IRQ2)
  enableAll(false);

  return true;
}

Pic::Pic()
  : m_SlavePort("PIC #2"), m_MasterPort("PIC #1"), m_InterruptMask(0), m_Lock(false)
{
  for (size_t i = 0;i < 16;i++)
  {
    m_Handler[i].clear();
    m_HandlerEdge[i] = false;
  }
}

bool Pic::spurious(size_t irq)
{
  if (irq > 7)
  {
    // Get ISR for slave.
    uint8_t mask = 1 << (irq - 8);
    m_SlavePort.write8(0x0B, 0);
    uint8_t isr = m_SlavePort.read8(0);
    m_SlavePort.write8(0x0A, 0);
    return (isr & mask) == 0;
  }
  else
  {
    // Get ISR for master.
    uint8_t mask = 1 << irq;
    m_MasterPort.write8(0x0B, 0);
    uint8_t isr = m_MasterPort.read8(0);
    m_SlavePort.write8(0x0A, 0);
    return (isr & mask) == 0;
  }
}

void Pic::interrupt(size_t interruptNumber, InterruptState &state)
{
  size_t irq = (interruptNumber - BASE_INTERRUPT_VECTOR);
  m_IrqCount[irq]++;

  // If disable() has been called for this IRQ, we need to do spurious IRQ
  // detection.
  if (m_InterruptMask & (1 << irq))
  {
    if (spurious(irq))
    {
      ERROR("PIC: spurious IRQ" << Dec << irq << Hex);
      return;
    }
  }

  // Call the irq handler, if any
  if (LIKELY(m_Handler[irq].count() != 0))
  {
    if(m_HandlerEdge[irq])
        eoi(irq);

    bool bHandled = false;
    for(List<IrqHandler*>::Iterator it = m_Handler[irq].begin();
        it != m_Handler[irq].end();
        it++)
    {
        bool tmp = (*it)->irq(irq, state);
        if((!bHandled) && tmp)
            bHandled = true;
    }
    
    if(!bHandled)
    {
      // Disable/Mask the IRQ line (the handler did not remove
      // the interrupt reason, yet)
      enable(irq, false);
    }

    if(!m_HandlerEdge[irq])
        eoi(irq);
  }
  else
  {
    NOTICE("PIC: unhandled irq #" << irq << " occurred");
  }
}

void Pic::eoi(uint8_t irq)
{
  if (irq > 7)
  {
    m_SlavePort.write8(0x60 + (irq - 8), 0);

    // ACK the cascade IRQ (IRQ2).
    m_MasterPort.write8(0x62, 0);
  }
  else
  {
    m_MasterPort.write8(0x60 + irq, 0);
  }
}

void Pic::enable(uint8_t irq, bool enable)
{
  if (enable)
  {
    m_InterruptMask &= ~(1 << irq);
  }
  else
  {
    m_InterruptMask |= 1 << irq;
  }

  if (irq <= 7)
  {
    uint8_t mask = m_MasterPort.read8(1);
    if (enable == true)mask = mask & ~(1 << irq);
    else mask = mask | (1 << irq);

    m_MasterPort.write8(mask, 1);
  }
  else
  {
    uint8_t mask = m_SlavePort.read8(1);
    if (enable == true)mask = mask & ~(1 << (irq - 8));
    else mask = mask | (1 << (irq - 8));

    m_SlavePort.write8(mask, 1);
  }
}
void Pic::enableAll(bool enable)
{
  if (enable == false)
  {
    m_InterruptMask = 0xFB;
    m_MasterPort.write8(0xFB, 1);
    m_SlavePort.write8(0xFB, 1);
  }
  else
  {
    m_InterruptMask = 0;
    m_MasterPort.write8(0x00, 1);
    m_SlavePort.write8(0x00, 1);
  }
}
