/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef IncrementalSweeper_h
#define IncrementalSweeper_h

#include "HeapTimer.h"
#include <wtf/Vector.h>

namespace JSC {

class Heap;
class MarkedBlock;

class IncrementalSweeper : public HeapTimer {
    WTF_MAKE_FAST_ALLOCATED;
public:
#if USE(CF)
    JS_EXPORT_PRIVATE IncrementalSweeper(Heap*, CFRunLoopRef);
    JS_EXPORT_PRIVATE void fullSweep();
#else
    explicit IncrementalSweeper(VM*);
#endif

    void startSweeping(Vector<MarkedBlock*>&&);
    void addBlocksAndContinueSweeping(Vector<MarkedBlock*>&&);

    JS_EXPORT_PRIVATE virtual void doWork() override;
    void sweepNextBlock();
    void willFinishSweeping();

#if USE(CF)
private:
    void doSweep(double startTime);
    void scheduleTimer();
    void cancelTimer();
    bool hasWork() const { return !m_blocksToSweep.isEmpty(); }
    
    Vector<MarkedBlock*>& m_blocksToSweep;
#endif
};

} // namespace JSC

#endif
