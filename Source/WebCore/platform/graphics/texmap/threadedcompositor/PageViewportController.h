/*
 * Copyright (C) 2011, 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Benjamin Poulain <benjamin@webkit.org>
 * Copyright (C) 2013 Company 100, Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef PageViewportController_h
#define PageViewportController_h

#if USE(COORDINATED_GRAPHICS_THREADED)

#include "FloatPoint.h"
#include "FloatRect.h"
#include "FloatSize.h"
#include "IntRect.h"
#include "IntSize.h"
#include "ViewportArguments.h"
#include <wtf/Noncopyable.h>

namespace WebCore {

class ThreadedCoordinatedCompositor;

class PageViewportController {
    WTF_MAKE_NONCOPYABLE(PageViewportController);
public:
    class Client {
    public:
        virtual void didChangeVisibleRect() = 0;
    };

    explicit PageViewportController(Client*);

    // From WebCore
    void didChangeViewportSize(const FloatSize&);
    void didChangeContentsSize(const IntSize&);
    void didChangeViewportAttribute(const ViewportAttributes&);

    void scrollBy(const IntSize&);
    void scrollTo(const IntPoint&);

    void didRenderFrame(const IntSize& contentsSize, const IntRect& coveredRect);

    FloatRect visibleContentsRect() const;
    float pageScaleFactor() const { return m_pageScaleFactor; }

private:

    FloatSize visibleContentsSize() const;

    void didChangeContentsVisibility(const FloatPoint& position, float scale);
    void syncVisibleContents();

    void applyScaleAfterRenderingContents(float scale);
    void applyPositionAfterRenderingContents(const WebCore::FloatPoint& pos);

    FloatPoint boundContentsPosition(const FloatPoint&) const;
    FloatPoint boundContentsPositionAtScale(const FloatPoint&, float scale) const;

    bool updateMinimumScaleToFit();
    float innerBoundedViewportScale(float) const;

    Client* m_client;

    FloatPoint m_contentsPosition;
    FloatSize m_contentsSize;
    FloatSize m_viewportSize;
    float m_pageScaleFactor;

    bool m_allowsUserScaling;
    float m_minimumScaleToFit;
    bool m_initiallyFitToViewport;

    ViewportAttributes m_rawAttributes;
};

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS_THREADED)

#endif // PageViewportController_h
