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

#include "CoordinatedCompositingThread.h"

#include "CoordinatedGraphicsScene.h"
#include "ThreadedCoordinatedCompositor.h"
#include "TransformationMatrix.h"
#include <wtf/CurrentTime.h>

#if USE(OPENGL_ES_2)
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#endif

namespace WebCore {

PassRefPtr<CoordinatedCompositingThread> CoordinatedCompositingThread::create(ThreadedCoordinatedCompositor* compositor, CoordinatedGraphicsSceneClient* sceneClient)
{
    return adoptRef(new CoordinatedCompositingThread(compositor, sceneClient));
}

CoordinatedCompositingThread::CoordinatedCompositingThread(ThreadedCoordinatedCompositor* compositor, CoordinatedGraphicsSceneClient* sceneClient)
    : m_compositor(compositor)
    , m_scene(adoptRef(new CoordinatedGraphicsScene(sceneClient)))
    , m_viewportController(std::make_unique<PageViewportController>(this))
    , m_runLoop(RunLoop::current())
    , m_updateTimer(m_runLoop, this, &CoordinatedCompositingThread::updateTimerFired)
{
}

CoordinatedCompositingThread::~CoordinatedCompositingThread()
{
}

void CoordinatedCompositingThread::run()
{
    m_runLoop.run();
}

void CoordinatedCompositingThread::stop()
{
    RefPtr<CoordinatedCompositingThread> protector(this);
    callOnCompositingThread([=] {
        protector->m_scene->purgeGLResources();

        if (protector->m_updateTimer.isActive())
            protector->m_updateTimer.stop();

        protector->m_runLoop.stop();
    });

}

void CoordinatedCompositingThread::setNeedsDisplay()
{
    RefPtr<CoordinatedCompositingThread> protector(this);
    callOnCompositingThread([=] {
        protector->scheduleDisplayIfNeeded();
    });
}

void CoordinatedCompositingThread::setNativeSurfaceHandleForCompositing(uint64_t handle)
{
    RefPtr<CoordinatedCompositingThread> protector(this);
    callOnCompositingThread([=] {
        protector->m_nativeSurfaceHandle = handle;
        protector->m_scene->setActive(true);
    });
}


void CoordinatedCompositingThread::didChangeViewportSize(const IntSize& newSize)
{
    RefPtr<CoordinatedCompositingThread> protector(this);
    callOnCompositingThread([=] {
        protector->viewportController()->didChangeViewportSize(newSize);
    });
}

void CoordinatedCompositingThread::didChangeViewportAttribute(const ViewportAttributes& attr)
{
    RefPtr<CoordinatedCompositingThread> protector(this);
    callOnCompositingThread([=] {
        protector->viewportController()->didChangeViewportAttribute(attr);
    });
}

void CoordinatedCompositingThread::didChangeContentsSize(const IntSize& size)
{
    RefPtr<CoordinatedCompositingThread> protector(this);
    callOnCompositingThread([=] {
        protector->viewportController()->didChangeContentsSize(size);
    });
}

void CoordinatedCompositingThread::scrollTo(const IntPoint& position)
{
    RefPtr<CoordinatedCompositingThread> protector(this);
    callOnCompositingThread([=] {
        protector->viewportController()->scrollTo(position);
    });
}

void CoordinatedCompositingThread::scrollBy(const IntSize& delta)
{
    RefPtr<CoordinatedCompositingThread> protector(this);
    callOnCompositingThread([=] {
        protector->viewportController()->scrollBy(delta);
    });
}

bool CoordinatedCompositingThread::ensureGLContext()
{
    if (!glContext())
        return false;

    glContext()->makeContextCurrent();
    // The window size may be out of sync with the page size at this point, and getting
    // the viewport parameters incorrect, means that the content will be misplaced. Thus
    // we set the viewport parameters directly from the window size.
    IntSize contextSize = glContext()->defaultFrameBufferSize();
    if (m_viewportSize != contextSize) {
        glViewport(0, 0, contextSize.width(), contextSize.height());
        m_viewportSize = contextSize;
    }

    // FIXME : below calls must be removed by introducing viewport and viewport controller.
    //         Non-composited contents layer will be rendered after implement viewport things.
    glClearColor(1, 1, 1, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    return true;
}

GLContext* CoordinatedCompositingThread::glContext()
{
    if (m_context)
        return m_context.get();

    if (!m_nativeSurfaceHandle)
        return 0;

    m_context = GLContext::createContextForWindow(m_nativeSurfaceHandle, GLContext::sharingContext());
    return m_context.get();
}

void CoordinatedCompositingThread::updateTimerFired()
{
    renderLayerTree();
}

void CoordinatedCompositingThread::scheduleDisplayIfNeeded(double interval)
{
    ASSERT(!isMainThread());

    if (m_updateTimer.isActive())
        return;

    m_updateTimer.startOneShot(interval);
}

void CoordinatedCompositingThread::didChangeVisibleRect()
{
    FloatRect visibleRect = viewportController()->visibleContentsRect();
    float scale = viewportController()->pageScaleFactor();
    callOnMainThread([=] {
        m_compositor->setVisibleContentsRect(visibleRect, FloatPoint::zero(), scale);
    });

    setNeedsDisplay();
}

void CoordinatedCompositingThread::renderLayerTree()
{
    if (!m_scene)
        return;

    if (!ensureGLContext())
        return;

    FloatRect clipRect(0, 0, m_viewportSize.width(), m_viewportSize.height());

    TransformationMatrix viewportTransform;
    FloatPoint scrollPostion = viewportController()->visibleContentsRect().location();
    viewportTransform.scale(viewportController()->pageScaleFactor());
    viewportTransform.translate(-scrollPostion.x(), -scrollPostion.y());

    m_scene->setScrollPosition(scrollPostion);
    m_scene->paintToCurrentGLContext(viewportTransform, 1.0, clipRect, 0);

    glContext()->swapBuffers();
}

void CoordinatedCompositingThread::updateSceneState(const CoordinatedGraphicsState& state)
{
    m_scene->appendUpdate(bind(&CoordinatedGraphicsScene::commitSceneState, m_scene.get(), state));
}

void CoordinatedCompositingThread::callOnCompositingThread(std::function<void()> function)
{
    if (&m_runLoop == &RunLoop::current()) {
        function();
        return;
    }

    m_runLoop.dispatch(std::move(function));
}

}
#endif // USE(COORDINATED_GRAPHICS_THREADED)
