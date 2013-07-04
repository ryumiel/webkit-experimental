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

#include "config.h"

#if USE(COORDINATED_GRAPHICS_THREADED)
#include "ThreadedCoordinatedCompositor.h"

#include "CoordinatedGraphicsLayer.h"
#include "CoordinatedGraphicsState.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "MainFrame.h"
#include "Page.h"
#include "ThreadSafeCoordinatedSurface.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

ThreadedCoordinatedCompositor::ThreadedCoordinatedCompositor(Page* page, Client* client, const IntSize& initSize)
    : m_page(page)
    , m_client(client)
    , m_notifyAfterScheduledLayerFlush(false)
    , m_isSuspended(false)
    , m_isWaitingForRenderer(false)
    , m_layerFlushTimer(RunLoop::main(), this, &ThreadedCoordinatedCompositor::layerFlushTimerFired)
    , m_layerFlushSchedulingEnabled(true)
{
    m_coordinator = std::make_unique<CompositingCoordinator>(m_page, this);

    m_coordinator->createRootLayer(initSize);

    CoordinatedSurface::setFactory(createCoordinatedSurface);

    createCompositingThread();
    scheduleLayerFlush();
}

ThreadedCoordinatedCompositor::~ThreadedCoordinatedCompositor()
{
    terminateCompositingThread();
}

PassRefPtr<CoordinatedSurface> ThreadedCoordinatedCompositor::createCoordinatedSurface(const IntSize& size, CoordinatedSurface::Flags flags)
{
    return ThreadSafeCoordinatedSurface::create(size, flags);
}

void ThreadedCoordinatedCompositor::setLayerFlushSchedulingEnabled(bool layerFlushingEnabled)
{
    if (m_layerFlushSchedulingEnabled == layerFlushingEnabled)
        return;

    m_layerFlushSchedulingEnabled = layerFlushingEnabled;

    if (m_layerFlushSchedulingEnabled) {
        scheduleLayerFlush();
        return;
    }

    cancelPendingLayerFlush();
}

void ThreadedCoordinatedCompositor::scheduleLayerFlush()
{
    if (!m_layerFlushSchedulingEnabled)
        return;

    if (!m_layerFlushTimer.isActive())
        m_layerFlushTimer.startOneShot(0);
}

void ThreadedCoordinatedCompositor::layerFlushTimerFired()
{
    performScheduledLayerFlush();
}

void ThreadedCoordinatedCompositor::viewportSizeChanged(const IntSize& newSize)
{
    m_compositingThread->didChangeViewportSize(newSize);
}

void ThreadedCoordinatedCompositor::viewportAttributeChanged(const ViewportAttributes& attr)
{
    m_compositingThread->didChangeViewportAttribute(attr);
}

void ThreadedCoordinatedCompositor::deviceOrPageScaleFactorChanged()
{
    m_coordinator->deviceOrPageScaleFactorChanged();
}

void ThreadedCoordinatedCompositor::setShouldNotifyAfterNextScheduledLayerFlush(bool notifyAfterScheduledLayerFlush)
{
    m_notifyAfterScheduledLayerFlush = notifyAfterScheduledLayerFlush;
}

void ThreadedCoordinatedCompositor::setRootCompositingLayer(GraphicsLayer* graphicsLayer)
{
    m_coordinator->setRootCompositingLayer(graphicsLayer, 0);
}

void ThreadedCoordinatedCompositor::scrollTo(const IntPoint& position)
{
    m_compositingThread->scrollTo(position);
    scheduleLayerFlush();
}

void ThreadedCoordinatedCompositor::scrollBy(const IntSize& delta)
{
    m_compositingThread->scrollBy(delta);
    scheduleLayerFlush();
}

void ThreadedCoordinatedCompositor::setVisibleContentsRect(const FloatRect& rect, const FloatPoint& trajectoryVector, float scale)
{
    m_coordinator->setVisibleContentsRect(rect, trajectoryVector);
    if (m_lastScrollPosition != roundedIntPoint(rect.location())) {
        m_lastScrollPosition = roundedIntPoint(rect.location());

        if (!m_page->mainFrame().view()->useFixedLayout())
            m_page->mainFrame().view()->notifyScrollPositionChanged(m_lastScrollPosition);
    }

    if (m_lastScaleFactor != scale) {
        m_lastScaleFactor = scale;
        m_client->didScaleFactorChanged(m_lastScaleFactor, m_lastScrollPosition);
    }

    scheduleLayerFlush();
}

void ThreadedCoordinatedCompositor::renderNextFrame()
{
    m_isWaitingForRenderer = false;
    m_coordinator->renderNextFrame();
    scheduleLayerFlush();
}

void ThreadedCoordinatedCompositor::updateViewport()
{
    m_compositingThread->setNeedsDisplay();
}

void ThreadedCoordinatedCompositor::purgeBackingStores()
{
    m_coordinator->purgeBackingStores();
}

void ThreadedCoordinatedCompositor::commitScrollOffset(uint32_t layerID, const IntSize& offset)
{
    m_coordinator->commitScrollOffset(layerID, offset);
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
void ThreadedCoordinatedCompositor::scheduleAnimation()
{
    if (m_isWaitingForRenderer)
        return;

    if (m_layerFlushTimer.isActive())
        return;

    m_layerFlushTimer.startOneShot(m_coordinator->nextAnimationServiceTime());
    scheduleLayerFlush();
}
#endif

void ThreadedCoordinatedCompositor::setNativeSurfaceHandleForCompositing(uint64_t handle)
{
    m_compositingThread->setNativeSurfaceHandleForCompositing(handle);
}

void ThreadedCoordinatedCompositor::cancelPendingLayerFlush()
{
    m_layerFlushTimer.stop();
}

void ThreadedCoordinatedCompositor::performScheduledLayerFlush()
{
    if (m_isSuspended || m_isWaitingForRenderer)
        return;

    m_coordinator->syncDisplayState();
    bool didSync = m_coordinator->flushPendingLayerChanges();

    if (m_notifyAfterScheduledLayerFlush && didSync) {
        m_client->compositorDidFlushLayers();
        m_notifyAfterScheduledLayerFlush = false;
    }
}

void ThreadedCoordinatedCompositor::notifyFlushRequired()
{
    scheduleLayerFlush();
}

void ThreadedCoordinatedCompositor::commitSceneState(const CoordinatedGraphicsState& state)
{
    m_isWaitingForRenderer = true;
    m_compositingThread->updateSceneState(state);
    updateViewport();
}

void ThreadedCoordinatedCompositor::paintLayerContents(const GraphicsLayer*, GraphicsContext&, const IntRect&)
{
}

void ThreadedCoordinatedCompositor::contentsSizeChanged(const IntSize& size)
{
    m_coordinator->sizeDidChange(size);
    m_compositingThread->didChangeContentsSize(size);
}

void ThreadedCoordinatedCompositor::runCompositingThread()
{
    {
        MutexLocker locker(m_initializeRunLoopConditionMutex);
        m_compositingThread = CoordinatedCompositingThread::create(this, this);
        m_initializeRunLoopCondition.signal();
    }

    m_compositingThread->run();

    {
        MutexLocker locker(m_terminateRunLoopConditionMutex);
        m_compositingThread = nullptr;
        m_terminateRunLoopCondition.signal();
    }

    detachThread(m_threadIdentifier);
}

void ThreadedCoordinatedCompositor::createCompositingThread()
{
    if (m_threadIdentifier)
        return;

    MutexLocker locker(m_initializeRunLoopConditionMutex);
    m_threadIdentifier = createThread(compositingThreadEntry, this, "WebCore: CoordinatedCompositingThread");

    m_initializeRunLoopCondition.wait(m_initializeRunLoopConditionMutex);
}

void ThreadedCoordinatedCompositor::compositingThreadEntry(void* coordinatedCompositor)
{
    static_cast<ThreadedCoordinatedCompositor*>(coordinatedCompositor)->runCompositingThread();
}

void ThreadedCoordinatedCompositor::terminateCompositingThread()
{
    MutexLocker locker(m_terminateRunLoopConditionMutex);

    m_compositingThread->stop();

    m_terminateRunLoopCondition.wait(m_terminateRunLoopConditionMutex);
}

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS_THREADED)
