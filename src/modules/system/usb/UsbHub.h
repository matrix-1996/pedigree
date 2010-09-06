/*
 * Copyright (c) 2010 Eduard Burtescu
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
#ifndef USBHUB_H
#define USBHUB_H

#include <machine/Device.h>
#include <process/Mutex.h>
#include <process/Semaphore.h>
#include <processor/types.h>
#include <utilities/ExtensibleBitmap.h>
#include <usb/Usb.h>

class UsbHub : public virtual Device
{
    public:

        inline UsbHub()
        {
            m_UsedAddresses.set(0);
        }

        inline virtual ~UsbHub() {}

        /// Adds a new transfer to an existent transaction
        virtual void addTransferToTransaction(uintptr_t pTransaction, bool bToggle, UsbPid pid, uintptr_t pBuffer, size_t nBytes) =0;

        /// Creates a new transaction with the given endpoint data
        virtual uintptr_t createTransaction(UsbEndpoint endpointInfo) =0;

        /// Performs a transaction asynchronously, calling the given callback on completion
        virtual void doAsync(uintptr_t pTransaction, void (*pCallback)(uintptr_t, ssize_t)=0, uintptr_t pParam=0) =0;

        /// Adds a new handler for an interrupt IN transaction
        virtual void addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam=0) =0;

        /// Called when a device is connected to a port on the hub
        bool deviceConnected(uint8_t nPort, UsbSpeed speed);

        /// Called when a device is disconnected from a port on the hub
        void deviceDisconnected(uint8_t nPort);

        /// Performs a transaction, blocks until it's completed and returns the result
        ssize_t doSync(uintptr_t nTransaction, uint32_t timeout=5000);

        /// Gets a UsbDevice from a given vendor:product pair
        //void getDeviceByIds(size_t vendor, size_t product, void (*pCallback)(class UsbDevice *));

    private:
        /// Structure used synchronous transactions
        struct SyncParam
        {
            inline SyncParam() : semaphore(0), nResult(-1), timedOut(false) {}

            Semaphore semaphore;
            ssize_t nResult;
            
            bool timedOut;
        };

        /// Callback used by synchronous transactions
        static void syncCallback(uintptr_t pParam, ssize_t ret);

        /// Bitmap of used addresses under this hub
        /// \note valid only for root hubs
        ExtensibleBitmap m_UsedAddresses;
};

#endif
