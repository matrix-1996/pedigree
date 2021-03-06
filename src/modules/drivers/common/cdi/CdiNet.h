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

#ifndef CDI_CPP_NET_H
#define CDI_CPP_NET_H

#include <processor/types.h>
#include <machine/Device.h>
#include <machine/Network.h>
#include <processor/IoBase.h>
#include <processor/IoPort.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <machine/IrqHandler.h>
#include <process/Thread.h>
#include <process/Semaphore.h>

#include "cdi/net.h"

/** CDI NIC Device */
class CdiNet : public Network
{
    public:
        CdiNet(struct cdi_net_device* device);
        CdiNet(Network* pDev, struct cdi_net_device* device);
        ~CdiNet();

        virtual void getName(String &str)
        {
            if((!m_Device) || (!m_Device->dev.name))
                str = "cdi-net";
            else
            {
                str = m_Device->dev.name;
            }
        }

        virtual bool send(size_t nBytes, uintptr_t buffer);

        virtual bool setStationInfo(StationInfo info);
        virtual StationInfo getStationInfo();

        const struct cdi_net_device *getCdiDevice() const
        {
            return m_Device;
        }

    private:
        CdiNet(const CdiNet&);
        const CdiNet & operator = (const CdiNet&);

        struct cdi_net_device* m_Device;
};

#endif
