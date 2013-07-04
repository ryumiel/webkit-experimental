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

#include "config.h"

#if USE(COORDINATED_GRAPHICS_THREADED)
#include "PageViewportController.h"

namespace WebCore {

PageViewportController::PageViewportController(Client* client)
    : m_client(client)
    , m_contentsPosition(FloatPoint())
    , m_contentsSize(FloatSize())
    , m_viewportSize(FloatSize())
    , m_pageScaleFactor(1.0)
    , m_allowsUserScaling(false)
    , m_minimumScaleToFit(1)
    , m_initiallyFitToViewport(true)
{
    // Initializing Viewport Raw Attributes to avoid random negative or infinity scale factors
    // if there is a race condition between the first layout and setting the viewport attributes for the first time.
    m_rawAttributes.minimumScale = 1;
    m_rawAttributes.maximumScale = 1;
    m_rawAttributes.userScalable = m_allowsUserScaling;

    // The initial scale might be implicit and set to -1, in this case we have to infer it
    // using the viewport size and the final layout size.
    // To be able to assert for valid scale we initialize it to -1.
    m_rawAttributes.initialScale = -1;
}

void PageViewportController::didChangeViewportSize(const FloatSize& newSize)
{
    if (newSize.isEmpty())
        return;

    m_viewportSize = newSize;
    updateMinimumScaleToFit();
    syncVisibleContents();
}

void PageViewportController::didChangeContentsSize(const IntSize& newSize)
{
    m_contentsSize = newSize;

    updateMinimumScaleToFit();

    if (m_initiallyFitToViewport) {
        // Restrict scale factors to m_minimumScaleToFit.
        ASSERT(m_minimumScaleToFit > 0);
        m_rawAttributes.initialScale = m_minimumScaleToFit;
        restrictScaleFactorToInitialScaleIfNotUserScalable(m_rawAttributes);
    }

    syncVisibleContents();
}

void PageViewportController::didChangeViewportAttribute(const ViewportAttributes& newAttributes)
{
    if (newAttributes.layoutSize.isEmpty())
        return;

    m_rawAttributes = newAttributes;
    m_allowsUserScaling = !!m_rawAttributes.userScalable;
    m_initiallyFitToViewport = (m_rawAttributes.initialScale < 0);

    if (!m_initiallyFitToViewport)
        restrictScaleFactorToInitialScaleIfNotUserScalable(m_rawAttributes);

    updateMinimumScaleToFit();

    syncVisibleContents();
}

void PageViewportController::scrollBy(const IntSize& scrollOffset)
{
    m_contentsPosition.move(scrollOffset);
    m_contentsPosition = boundContentsPosition(m_contentsPosition);

    syncVisibleContents();
}

void PageViewportController::scrollTo(const IntPoint& position)
{
    if (m_contentsPosition == boundContentsPosition(position))
        return;

    m_contentsPosition = boundContentsPosition(position);
    syncVisibleContents();
}

void PageViewportController::didRenderFrame(const IntSize& contentsSize, const IntRect& coveredRect)
{
    didChangeContentsVisibility(m_contentsPosition, m_pageScaleFactor);
}

void PageViewportController::didChangeContentsVisibility(const FloatPoint& position, float scale)
{
    m_contentsPosition = position;
    m_pageScaleFactor = scale;
    syncVisibleContents();
}

void PageViewportController::syncVisibleContents()
{
    if (m_viewportSize.isEmpty() || m_contentsSize.isEmpty())
        return;

    m_client->didChangeVisibleRect();
}

FloatRect PageViewportController::visibleContentsRect() const
{
    FloatRect visibleContentsRect(boundContentsPosition(m_contentsPosition), visibleContentsSize());
    visibleContentsRect.intersect(FloatRect(FloatPoint::zero(), m_contentsSize));

    return visibleContentsRect;
}

FloatSize PageViewportController::visibleContentsSize() const
{
    return FloatSize(m_viewportSize.width() / m_pageScaleFactor, m_viewportSize.height() / m_pageScaleFactor);
}

FloatPoint PageViewportController::boundContentsPositionAtScale(const FloatPoint& framePosition, float scale) const
{
    // We need to floor the viewport here as to allow aligning the content in device units. If not,
    // it might not be possible to scroll the last pixel and that affects fixed position elements.
    FloatRect bounds;
    bounds.setWidth(std::max(0.f, m_contentsSize.width() - floorf(m_viewportSize.width() / scale)));
    bounds.setHeight(std::max(0.f, m_contentsSize.height() - floorf(m_viewportSize.height() / scale)));

    FloatPoint position;
    position.setX(clampTo(framePosition.x(), bounds.x(), bounds.width()));
    position.setY(clampTo(framePosition.y(), bounds.y(), bounds.height()));

    return position;
}

FloatPoint PageViewportController::boundContentsPosition(const FloatPoint& framePosition) const
{
    return boundContentsPositionAtScale(framePosition, m_pageScaleFactor);
}

bool fuzzyCompare(float a, float b, float epsilon)
{
    return std::abs(a - b) < epsilon;
}

bool PageViewportController::updateMinimumScaleToFit()
{
    if (m_viewportSize.isEmpty() || m_contentsSize.isEmpty())
        return false;

    bool currentlyScaledToFit = fuzzyCompare(m_pageScaleFactor, m_minimumScaleToFit, 0.0001);

    float minimumScale = computeMinimumScaleFactorForContentContained(m_rawAttributes, roundedIntSize(m_viewportSize), roundedIntSize(m_contentsSize));

    if (minimumScale <= 0)
        return false;

    if (!fuzzyCompare(minimumScale, m_minimumScaleToFit, 0.0001)) {
        m_minimumScaleToFit = minimumScale;

        if (currentlyScaledToFit)
            m_pageScaleFactor = m_minimumScaleToFit;
        else {
            // Ensure the effective scale stays within bounds.
            float boundedScale = innerBoundedViewportScale(m_pageScaleFactor);
            if (!fuzzyCompare(boundedScale, m_pageScaleFactor, 0.0001))
                m_pageScaleFactor = boundedScale;
        }

        return true;
    }
    return false;
}

float PageViewportController::innerBoundedViewportScale(float viewportScale) const
{
    return clampTo(viewportScale, m_minimumScaleToFit, m_rawAttributes.maximumScale);
}

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS_THREADED)
