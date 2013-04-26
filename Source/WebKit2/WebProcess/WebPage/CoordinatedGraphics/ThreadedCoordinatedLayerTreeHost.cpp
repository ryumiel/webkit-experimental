/*
 * Copyright (C) 2013 Company 100, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ThreadedCoordinatedLayerTreeHost.h"

#if USE(COORDINATED_GRAPHICS_THREADED)

#include "DrawingAreaImpl.h"
#include "NotImplemented.h"
#include "WebPage.h"

using namespace WebCore;

namespace WebKit {

PassRefPtr<ThreadedCoordinatedLayerTreeHost> ThreadedCoordinatedLayerTreeHost::create(WebPage* webPage)
{
    return adoptRef(new ThreadedCoordinatedLayerTreeHost(webPage));
}

ThreadedCoordinatedLayerTreeHost::~ThreadedCoordinatedLayerTreeHost()
{
}

ThreadedCoordinatedLayerTreeHost::ThreadedCoordinatedLayerTreeHost(WebPage* webPage)
    : LayerTreeHost(webPage)
    , m_forceRepaintAsyncCallbackID(0)
{
    m_compositor = std::make_unique<ThreadedCoordinatedCompositor>(webPage->corePage(), this, m_webPage->size());
}

void ThreadedCoordinatedLayerTreeHost::scheduleLayerFlush()
{
    m_compositor->scheduleLayerFlush();
}

void ThreadedCoordinatedLayerTreeHost::setLayerFlushSchedulingEnabled(bool layerFlushingEnabled)
{
    m_compositor->setLayerFlushSchedulingEnabled(layerFlushingEnabled);
}

void ThreadedCoordinatedLayerTreeHost::setShouldNotifyAfterNextScheduledLayerFlush(bool notifyAfterScheduledLayerFlush)
{
    m_compositor->setShouldNotifyAfterNextScheduledLayerFlush(notifyAfterScheduledLayerFlush);
}

void ThreadedCoordinatedLayerTreeHost::setRootCompositingLayer(WebCore::GraphicsLayer* graphicsLayer)
{
    m_compositor->setRootCompositingLayer(graphicsLayer);
}

void ThreadedCoordinatedLayerTreeHost::invalidate()
{
    notImplemented();
}

void ThreadedCoordinatedLayerTreeHost::scrollNonCompositedContents(const WebCore::IntRect& rect)
{
    m_compositor->scrollTo(rect.location());
}

void ThreadedCoordinatedLayerTreeHost::forceRepaint()
{
    notImplemented();
}

bool ThreadedCoordinatedLayerTreeHost::forceRepaintAsync(uint64_t callbackID)
{
    // We expect the UI process to not require a new repaint until the previous one has finished.
    ASSERT(!m_forceRepaintAsyncCallbackID);
    m_forceRepaintAsyncCallbackID = callbackID;
    m_compositor->scheduleLayerFlush();
    return true;
}

void ThreadedCoordinatedLayerTreeHost::sizeDidChange(const WebCore::IntSize& newSize)
{
    m_compositor->contentsSizeChanged(newSize);

    if (m_pageOverlayLayer)
        m_pageOverlayLayer->setSize(newSize);
}

void ThreadedCoordinatedLayerTreeHost::deviceOrPageScaleFactorChanged()
{
    m_compositor->deviceOrPageScaleFactorChanged();

    if (m_pageOverlayLayer)
        m_pageOverlayLayer->deviceOrPageScaleFactorChanged();
}

void ThreadedCoordinatedLayerTreeHost::didInstallPageOverlay(PageOverlay* pageOverlay)
{
    notImplemented();
}

void ThreadedCoordinatedLayerTreeHost::didUninstallPageOverlay(PageOverlay* pageOverlay)
{
    notImplemented();
}

void ThreadedCoordinatedLayerTreeHost::setPageOverlayNeedsDisplay(PageOverlay*, const WebCore::IntRect& rect)
{
    ASSERT(m_pageOverlayLayer);
    m_pageOverlayLayer->setNeedsDisplayInRect(rect);
    scheduleLayerFlush();
}

void ThreadedCoordinatedLayerTreeHost::setPageOverlayOpacity(PageOverlay*, float value)
{
    ASSERT(m_pageOverlayLayer);
    m_pageOverlayLayer->setOpacity(value);
    scheduleLayerFlush();
}

void ThreadedCoordinatedLayerTreeHost::pauseRendering()
{
    m_compositor->pauseRendering();
}

void ThreadedCoordinatedLayerTreeHost::resumeRendering()
{
    m_compositor->resumeRendering();
}

GraphicsLayerFactory* ThreadedCoordinatedLayerTreeHost::graphicsLayerFactory()
{
    return m_compositor->graphicsLayerFactory();
}

void ThreadedCoordinatedLayerTreeHost::setBackgroundColor(const WebCore::Color& color)
{
    notImplemented();
}

void ThreadedCoordinatedLayerTreeHost::viewportSizeChanged(const WebCore::IntSize& size)
{
    m_compositor->viewportSizeChanged(size);
}

void ThreadedCoordinatedLayerTreeHost::didChangeViewportProperties(const WebCore::ViewportAttributes& attr)
{
    m_compositor->viewportAttributeChanged(attr);
}

void ThreadedCoordinatedLayerTreeHost::compositorDidFlushLayers()
{
    static_cast<DrawingAreaImpl*>(m_webPage->drawingArea())->layerHostDidFlushLayers();
}

void ThreadedCoordinatedLayerTreeHost::didScaleFactorChanged(float scale, const IntPoint& origin)
{
    m_webPage->scalePage(scale, origin);
}

#if PLATFORM(GTK)
void ThreadedCoordinatedLayerTreeHost::setNativeSurfaceHandleForCompositing(uint64_t handle)
{
    m_layerTreeContext.contextID = handle;
    m_compositor->setNativeSurfaceHandleForCompositing(handle);
}
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
void ThreadedCoordinatedLayerTreeHost::scheduleAnimation()
{
    m_compositor->scheduleAnimation();
}
#endif

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS)
