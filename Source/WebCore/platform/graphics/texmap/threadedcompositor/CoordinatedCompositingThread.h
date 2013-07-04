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

#ifndef CoordinatedCompositingThread_h
#define CoordinatedCompositingThread_h

#if USE(COORDINATED_GRAPHICS_THREADED)

#include "GLContext.h"
#include "IntSize.h"
#include "PageViewportController.h"
#include "TransformationMatrix.h"
#include <wtf/FastMalloc.h>
#include <wtf/Noncopyable.h>
#include <wtf/RunLoop.h>
#include <wtf/Threading.h>
#include <wtf/ThreadSafeRefCounted.h>

namespace WebCore {

class CoordinatedGraphicsScene;
class CoordinatedGraphicsSceneClient;
class CoordinatedGraphicsState;
class ThreadedCoordinatedCompositor;

class CoordinatedCompositingThread : public PageViewportController::Client, public ThreadSafeRefCounted<CoordinatedCompositingThread> {
    WTF_MAKE_NONCOPYABLE(CoordinatedCompositingThread);
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<CoordinatedCompositingThread> create(ThreadedCoordinatedCompositor*, CoordinatedGraphicsSceneClient*);
    virtual ~CoordinatedCompositingThread();

    void run();
    void stop();

    void setNeedsDisplay();

    void setNativeSurfaceHandleForCompositing(uint64_t);

    void updateSceneState(const CoordinatedGraphicsState&);

    void didChangeViewportSize(const IntSize&);
    void didChangeViewportAttribute(const ViewportAttributes&);
    void didChangeContentsSize(const IntSize&);
    void scrollTo(const IntPoint&);
    void scrollBy(const IntSize&);

private:
    CoordinatedCompositingThread(ThreadedCoordinatedCompositor*, CoordinatedGraphicsSceneClient*);

    void renderLayerTree();

    void callOnCompositingThread(std::function<void()>);

    void runLoop();

    void updateTimerFired();
    void scheduleDisplayIfNeeded(double interval = 0.0f);

    virtual void didChangeVisibleRect() override;

    bool ensureGLContext();
    GLContext* glContext();

    PageViewportController* viewportController() { return m_viewportController.get(); }

    ThreadedCoordinatedCompositor* m_compositor;
    RefPtr<CoordinatedGraphicsScene> m_scene;
    std::unique_ptr<PageViewportController> m_viewportController;

    OwnPtr<GLContext> m_context;

    IntSize m_viewportSize;
    uint64_t m_nativeSurfaceHandle;

    RunLoop& m_runLoop;
    RunLoop::Timer<CoordinatedCompositingThread> m_updateTimer;
};

} // namespace WebCore

#endif

#endif // CoordinatedCompositingThread_h
