/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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
#include "Console.h"
#include <panic.h>

UserConsole::UserConsole() :
    RequestQueue(), m_pReq(0), m_pPendingReq(0)
{
}

UserConsole::~UserConsole()
{
}

void UserConsole::requestPending()
{
    m_pPendingReq = m_pReq;
    m_pReq = 0;
}

void UserConsole::respondToPending(size_t response, char *buffer, size_t sz)
{
    if (m_pPendingReq)
    {
        m_pPendingReq->ret = response;

        if (m_pPendingReq->p1 == CONSOLE_READ)
        {
            memcpy(reinterpret_cast<uint8_t*>(m_pPendingReq->p4), buffer, response);
        }

        // RequestQueue finished - post the request's mutex to wake the calling thread.
        m_pPendingReq->mutex.release();

        if (m_pPendingReq->isAsync)
            delete m_pPendingReq;
    }
    m_pPendingReq = 0;
}

size_t UserConsole::nextRequest(size_t responseToLast, char *buffer, size_t *sz, size_t maxBuffSz, size_t *terminalId)
{
    if (m_pReq)
    {
        m_pReq->ret = responseToLast;

        if (m_pReq->p1 == CONSOLE_READ)
        {
            memcpy(reinterpret_cast<uint8_t*>(m_pReq->p4), buffer, responseToLast);
        }

        // RequestQueue finished - post the request's mutex to wake the calling thread.
        m_pReq->mutex.release();

        if (m_pReq->isAsync)
            delete m_pReq;
    }

    // Sleep on the queue length semaphore - wake when there's something to do.
    m_RequestQueueSize.acquire();

    // Check why we were woken - is m_Stop set? If so, quit.
    if (m_Stop)
      return 0;

    // Get the first request from the queue.
    m_RequestQueueMutex.acquire();

    m_pReq = m_pRequestQueue;
    // Quick sanity check:
    if (m_pReq == 0) panic("RequestQueue: Worker thread woken but no requests pending!");
    m_pRequestQueue = m_pReq->next;

    m_RequestQueueMutex.release();

    // Perform the request.
    size_t command = m_pReq->p1;
    *sz = static_cast<size_t>(m_pReq->p3);

    *terminalId = m_pReq->p2;

    if (command == CONSOLE_WRITE)
    {
        if (*sz > maxBuffSz) m_pReq->p3 = maxBuffSz;
        memcpy(buffer, reinterpret_cast<uint8_t*>(m_pReq->p4), m_pReq->p3);
        if (m_pReq->isAsync)
            delete [] reinterpret_cast<uint8_t*>(m_pReq->p4);
    }
    else if (command == TUI_CHAR_RECV)
    {
        memcpy(buffer, reinterpret_cast<uint8_t*>(&m_pReq->p3), 8);
        *sz = 8;
    }

    return command;
}
