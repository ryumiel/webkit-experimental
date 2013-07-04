/*
 * Copyright (C) 2013 Company 100, Inc. All rights reserved.
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

#ifndef ThreadedCoordinatedCompositor_h
#define ThreadedCoordinatedCompositor_h

#if USE(COORDINATED_GRAPHICS_THREADED)

#include "CompositingCoordinator.h"
#include "CoordinatedCompositingThread.h"
#include "CoordinatedGraphicsScene.h"
#include "FloatPoint.h"
#include "FloatRect.h"
#include "IntRect.h"
#include "IntSize.h"
#include "Timer.h"
#include <wtf/RunLoop.h>
#include <wtf/Threading.h>

namespace WebCore {

class CoordinatedSurface;
class CoordinatedGraphicsLayerState;
class CoordinatedGraphicsState;
class GraphicsContext;
class GraphicsLayer;
class GraphicsLayerFactory;

class ThreadedCoordinatedCompositor : public CompositingCoordinator::Client, public CoordinatedGraphicsSceneClient {
    WTF_MAKE_NONCOPYABLE(ThreadedCoordinatedCompositor); WTF_MAKE_FAST_ALLOCATED;
public:
    class Client {
    public:
        virtual void compositorDidFlushLayers() { }
        virtual void didScaleFactorChanged(float scale, const IntPoint& origin) = 0;
    };

    ThreadedCoordinatedCompositor(Page*, Client*, const IntSize&);
    virtual ~ThreadedCoordinatedCompositor();

    void setLayerFlushSchedulingEnabled(bool);
    void scheduleLayerFlush();
    void setShouldNotifyAfterNextScheduledLayerFlush(bool);
    void setRootCompositingLayer(GraphicsLayer*);

    void scrollTo(const IntPoint&);
    void scrollBy(const IntSize&);

    void pauseRendering() { m_isSuspended = true; }
    void resumeRendering() { m_isSuspended = false; scheduleLayerFlush(); }

    void viewportSizeChanged(const IntSize&);
    void contentsSizeChanged(const IntSize&);
    void viewportAttributeChanged(const ViewportAttributes&);
    void deviceOrPageScaleFactorChanged();

    void setVisibleContentsRect(const FloatRect&, const FloatPoint&, float);
#if ENABLE(REQUEST_ANIMATION_FRAME)
    void scheduleAnimation();
#endif

    IntPoint scrollPosition() const { return m_lastScrollPosition; }
    float scaleFactor() const { return m_lastScaleFactor; }

    GraphicsLayerFactory* graphicsLayerFactory() const { return m_coordinator.get(); }
    void setNativeSurfaceHandleForCompositing(uint64_t);

    static PassRefPtr<CoordinatedSurface> createCoordinatedSurface(const IntSize&, CoordinatedSurface::Flags);

private:

    void cancelPendingLayerFlush();
    void performScheduledLayerFlush();

    GraphicsLayer* rootLayer() { return m_coordinator->rootLayer(); }

    void layerFlushTimerFired();

    // CoordinatedGraphicsSceneClient
    virtual void purgeBackingStores() override;
    virtual void renderNextFrame() override;
    virtual void updateViewport() override;
    virtual void commitScrollOffset(uint32_t layerID, const IntSize& offset) override;

    // CompositingCoordinator::Client
    virtual void didFlushRootLayer(const FloatRect&) override { }
    virtual void notifyFlushRequired() override;
    virtual void commitSceneState(const CoordinatedGraphicsState&) override;
    virtual void paintLayerContents(const GraphicsLayer*, GraphicsContext&, const IntRect& clipRect) override;

    void createCompositingThread();
    void runCompositingThread();
    void terminateCompositingThread();

    static void compositingThreadEntry(void*);

    Page* m_page;
    Client* m_client;

    std::unique_ptr<CompositingCoordinator> m_coordinator;
    RefPtr<CoordinatedCompositingThread> m_compositingThread;
    std::unique_ptr<PageViewportController> m_viewportController;

    bool m_notifyAfterScheduledLayerFlush;
    bool m_isSuspended;
    bool m_isWaitingForRenderer;

    float m_lastScaleFactor;
    IntPoint m_lastScrollPosition;

    RunLoop::Timer<ThreadedCoordinatedCompositor> m_layerFlushTimer;
    bool m_layerFlushSchedulingEnabled;

    ThreadIdentifier m_threadIdentifier;
    ThreadCondition m_initializeRunLoopCondition;
    Mutex m_initializeRunLoopConditionMutex;
    ThreadCondition m_terminateRunLoopCondition;
    Mutex m_terminateRunLoopConditionMutex;
};

} // namespace WebCore

#endif

#endif // ThreadedCoordinatedCompositor_h
