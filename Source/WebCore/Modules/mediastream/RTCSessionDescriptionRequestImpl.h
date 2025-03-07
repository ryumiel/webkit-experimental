/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RTCSessionDescriptionRequestImpl_h
#define RTCSessionDescriptionRequestImpl_h

#if ENABLE(MEDIA_STREAM)

#include "ActiveDOMObject.h"
#include "RTCSessionDescriptionRequest.h"

namespace WebCore {

class RTCPeerConnectionErrorCallback;
class RTCPeerConnection;
class RTCSessionDescriptionCallback;

class RTCSessionDescriptionRequestImpl : public RTCSessionDescriptionRequest, public ActiveDOMObject {
public:
    static PassRefPtr<RTCSessionDescriptionRequestImpl> create(ScriptExecutionContext*, PassRefPtr<RTCSessionDescriptionCallback>, PassRefPtr<RTCPeerConnectionErrorCallback>);
    virtual ~RTCSessionDescriptionRequestImpl();

    virtual void requestSucceeded(PassRefPtr<RTCSessionDescriptionDescriptor>) override;
    virtual void requestFailed(const String& error) override;

private:
    RTCSessionDescriptionRequestImpl(ScriptExecutionContext*, PassRefPtr<RTCSessionDescriptionCallback>, PassRefPtr<RTCPeerConnectionErrorCallback>);

    // ActiveDOMObject API.
    void stop() override;
    const char* activeDOMObjectName() const override;
    bool canSuspend() const override;

    void clear();

    RefPtr<RTCSessionDescriptionCallback> m_successCallback;
    RefPtr<RTCPeerConnectionErrorCallback> m_errorCallback;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // RTCSessionDescriptionRequestImpl_h


