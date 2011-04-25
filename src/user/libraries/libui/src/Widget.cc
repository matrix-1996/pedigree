/*
 * Copyright (c) 2011 Matthew Iselin
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
#include <Widget.h>

/// \todo GTFO libc!
#include <unistd.h>

#include "protocol.h"

using namespace LibUiProtocol;

/// Default event handler for new widgets.
static bool defaultEventHandler(WidgetMessages message, size_t dataSize, void *dataBuffer)
{
    return false;
}

Widget::Widget() : m_bConstructed(false), m_pFramebuffer(0), m_Handle(0), m_EventCallback(defaultEventHandler)
{
}

Widget::~Widget()
{
}

bool Widget::construct(widgetCallback_t cb, PedigreeGraphics::Rect &dimensions)
{
    if(m_Handle)
    {
        // Already constructed!
        return false;
    }

    /// \todo Maybe we can get a decent way of having a default handler?
    if(!cb)
        return false;

    // Construct the handle first.
    uint64_t pid = getpid();
    uintptr_t widgetPointer = reinterpret_cast<uintptr_t>(this);
#ifdef BITS_64
    m_Handle = (pid << 32) | (((widgetPointer >> 32) | (widgetPointer & 0xFFFFFFFF)) & 0xFFFFFFFF);
#else
    m_Handle = (pid << 32) | (widgetPointer);
#endif

    // Prepare a message to send.
    size_t totalSize = sizeof(WindowManagerMessage) + sizeof(CreateMessage);
    char *messageData = new char[totalSize];
    WindowManagerMessage *pWinMan = reinterpret_cast<WindowManagerMessage*>(messageData);
    CreateMessage *pCreate = reinterpret_cast<CreateMessage*>(messageData + sizeof(WindowManagerMessage));

    // Fill.
    pWinMan->messageCode = Create;
    pWinMan->messageSize = sizeof(CreateMessage);
    pWinMan->widgetHandle = m_Handle;
    pCreate->x = dimensions.getX();
    pCreate->y = dimensions.getY();
    pCreate->width = dimensions.getW();
    pCreate->height = dimensions.getH();

    // Send the message off to the window manager and wait for a response. The
    // response will contain a GraphicsProvider that we can use to create our
    // Framebuffer for drawing on.
    if(!sendMessage(messageData, totalSize))
    {
        m_Handle = 0;
        delete [] messageData;
        return false;
    }

    delete [] messageData;

    totalSize = sizeof(WindowManagerMessage) + sizeof(CreateMessageResponse);
    messageData = new char[totalSize];

    if(!recvMessage(messageData, totalSize))
    {
        m_Handle = 0;
        delete [] messageData;
        return false;
    }

    /// \todo Verify this message is actually for us!

    // Grab the results and use them.
    CreateMessageResponse *pCreateResp = reinterpret_cast<CreateMessageResponse*>(messageData + sizeof(WindowManagerMessage));
    m_pFramebuffer = new PedigreeGraphics::Framebuffer(pCreateResp->provider);
    m_EventCallback = cb;

    return true;
}

bool Widget::setProperty(std::string propName, void *propVal, size_t maxSize)
{
    return false;
}

bool Widget::getProperty(std::string propName, char **buffer, size_t maxSize)
{
    return false;
}

void Widget::setParent(Widget *pWidget)
{
}

Widget *Widget::getParent()
{
    return 0;
}

bool Widget::redraw(PedigreeGraphics::Rect &rt)
{
    return false;
}

bool Widget::visibility(bool vis)
{
    return false;
}

void Widget::destroy()
{
}

void Widget::checkForEvents()
{
}