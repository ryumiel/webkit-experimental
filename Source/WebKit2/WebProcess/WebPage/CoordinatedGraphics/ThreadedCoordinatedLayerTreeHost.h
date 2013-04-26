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

#ifndef ThreadedCoordinatedLayerTreeHost_h
#define ThreadedCoordinatedLayerTreeHost_h

#if USE(COORDINATED_GRAPHICS_THREADED)

#include "LayerTreeContext.h"
#include "LayerTreeHost.h"
#include <WebCore/IntPoint.h>
#include <WebCore/ThreadedCoordinatedCompositor.h>
#include <wtf/OwnPtr.h>

namespace WebCore {
class GraphicsLayerFactory;
}

namespace WebKit {

class WebPage;

class ThreadedCoordinatedLayerTreeHost : public LayerTreeHost, public WebCore::ThreadedCoordinatedCompositor::Client {
public:
    static PassRefPtr<ThreadedCoordinatedLayerTreeHost> create(WebPage*);
    virtual ~ThreadedCoordinatedLayerTreeHost();

    virtual const LayerTreeContext& layerTreeContext() override { return m_layerTreeContext; };

    virtual void scheduleLayerFlush() override;
    virtual void setLayerFlushSchedulingEnabled(bool) override;
    virtual void setShouldNotifyAfterNextScheduledLayerFlush(bool) override;
    virtual void setRootCompositingLayer(WebCore::GraphicsLayer*) override;
    virtual void invalidate() override;

    virtual void setNonCompositedContentsNeedDisplay() override { };
    virtual void setNonCompositedContentsNeedDisplayInRect(const WebCore::IntRect&) override { };
    virtual void scrollNonCompositedContents(const WebCore::IntRect& scrollRect) override;
    virtual void forceRepaint() override;
    virtual bool forceRepaintAsync(uint64_t /*callbackID*/) override;
    virtual void sizeDidChange(const WebCore::IntSize& newSize) override;
    virtual void deviceOrPageScaleFactorChanged() override;

    virtual void didInstallPageOverlay(PageOverlay*) override;
    virtual void didUninstallPageOverlay(PageOverlay*) override;
    virtual void setPageOverlayNeedsDisplay(PageOverlay*, const WebCore::IntRect&) override;
    virtual void setPageOverlayOpacity(PageOverlay*, float) override;
    virtual bool pageOverlayShouldApplyFadeWhenPainting() const { return false; }

    virtual void pauseRendering() override;
    virtual void resumeRendering() override;

    virtual WebCore::GraphicsLayerFactory* graphicsLayerFactory() override;
    virtual void setBackgroundColor(const WebCore::Color&) override;
    virtual void pageBackgroundTransparencyChanged() override { };

    virtual void viewportSizeChanged(const WebCore::IntSize&) override;
    virtual void didChangeViewportProperties(const WebCore::ViewportAttributes&) override;

#if PLATFORM(GTK)
    virtual void setNativeSurfaceHandleForCompositing(uint64_t) override;
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
    virtual void scheduleAnimation() override;
#endif

protected:
    explicit ThreadedCoordinatedLayerTreeHost(WebPage*);

private:

    virtual void compositorDidFlushLayers() override;
    virtual void didScaleFactorChanged(float scale, const WebCore::IntPoint& origin) override;

    LayerTreeContext m_layerTreeContext;
    uint64_t m_forceRepaintAsyncCallbackID;

    WebCore::IntPoint m_prevScrollPosition;

    // The page overlay layer. Will be null if there's no page overlay.
    std::unique_ptr<WebCore::GraphicsLayer> m_pageOverlayLayer;
    RefPtr<PageOverlay> m_pageOverlay;

    std::unique_ptr<WebCore::ThreadedCoordinatedCompositor> m_compositor;

};

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS_THREADED)

#endif // ThreadedCoordinatedLayerTreeHost_h
