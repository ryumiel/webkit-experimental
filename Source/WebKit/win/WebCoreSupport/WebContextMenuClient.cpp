/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "WebContextMenuClient.h"

#include "UserGestureIndicator.h"
#include "WebElementPropertyBag.h"
#include "WebLocalizableStrings.h"
#include "WebView.h"
#include <WebCore/ContextMenu.h>
#include <WebCore/ContextMenuController.h>
#include <WebCore/Editor.h>
#include <WebCore/Event.h>
#include <WebCore/FrameLoadRequest.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/MainFrame.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/Page.h>
#include <WebCore/ResourceRequest.h>

using namespace WebCore;

WebContextMenuClient::WebContextMenuClient(WebView* webView)
    : m_webView(webView)
{
}

void WebContextMenuClient::contextMenuDestroyed()
{
    delete this;
}

std::unique_ptr<ContextMenu> WebContextMenuClient::customizeMenu(std::unique_ptr<ContextMenu> popMenu)
{
    std::unique_ptr<ContextMenu> menu = WTF::move(popMenu);

    COMPtr<IWebUIDelegate> uiDelegate;
    if (FAILED(m_webView->uiDelegate(&uiDelegate)))
        return menu;

    ASSERT(uiDelegate);

    HMENU nativeMenu = menu->platformContextMenu();
    COMPtr<WebElementPropertyBag> propertyBag;
    propertyBag.adoptRef(WebElementPropertyBag::createInstance(m_webView->page()->contextMenuController().hitTestResult()));
    // FIXME: We need to decide whether to do the default before calling this delegate method
    if (FAILED(uiDelegate->contextMenuItemsForElement(m_webView, propertyBag.get(), nativeMenu, &nativeMenu))) {
        ::DestroyMenu(nativeMenu);
        return menu;
    }
    
    std::unique_ptr<ContextMenu> customizedMenu = std::unique_ptr<ContextMenu>(new ContextMenu(nativeMenu));
    ::DestroyMenu(nativeMenu);

    return customizedMenu;
}

void WebContextMenuClient::contextMenuItemSelected(ContextMenuItem* item, const ContextMenu* parentMenu)
{
    ASSERT(item->type() == ActionType || item->type() == CheckableActionType);

    COMPtr<IWebUIDelegate> uiDelegate;
    if (FAILED(m_webView->uiDelegate(&uiDelegate)))
        return;

    ASSERT(uiDelegate);

    COMPtr<WebElementPropertyBag> propertyBag;
    propertyBag.adoptRef(WebElementPropertyBag::createInstance(m_webView->page()->contextMenuController().hitTestResult()));

    // This call would leak the MENUITEMINFO's subMenu if it had one, but on Windows, subMenus can't be selected, so there is
    // no way we would get to this point. Also, it can't be a separator, because separators cannot be selected.
    ASSERT(item->type() != SubmenuType);
    ASSERT(item->type() != SeparatorType);

    // ContextMenuItem::platformContextMenuItem doesn't set the dwTypeData of the MENUITEMINFO, but no WebKit clients
    // use the title in IWebUIDelegate::contextMenuItemSelected, so we don't need to populate it here.
    MENUITEMINFO selectedItem = item->platformContextMenuItem();

    uiDelegate->contextMenuItemSelected(m_webView, &selectedItem, propertyBag.get());
}

void WebContextMenuClient::downloadURL(const URL& url)
{
    m_webView->downloadURL(url);
}

void WebContextMenuClient::searchWithGoogle(const Frame* frame)
{
    String searchString = frame->editor().selectedText();
    searchString.stripWhiteSpace();
    String encoded = encodeWithURLEscapeSequences(searchString);
    encoded.replace("%20", "+");
    
    String url("http://www.google.com/search?q=");
    url.append(encoded);
    url.append("&ie=UTF-8&oe=UTF-8");

    if (Page* page = frame->page()) {
        UserGestureIndicator indicator(DefinitelyProcessingUserGesture);
        page->mainFrame().loader().urlSelected(URL(ParsedURLString, url), String(), 0, LockHistory::No, LockBackForwardList::No, MaybeSendReferrer);
    }
}

void WebContextMenuClient::lookUpInDictionary(Frame*)
{
    notImplemented();
}

void WebContextMenuClient::speak(const String&)
{
    notImplemented();
}

void WebContextMenuClient::stopSpeaking()
{
    notImplemented();
}

bool WebContextMenuClient::isSpeaking()
{
    notImplemented();
    return false;
}
