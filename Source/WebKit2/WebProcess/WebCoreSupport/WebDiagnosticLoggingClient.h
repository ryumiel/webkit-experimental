/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebDiagnosticLoggingClient_h
#define WebDiagnosticLoggingClient_h

#include <WebCore/DiagnosticLoggingClient.h>
#include <wtf/Forward.h>

namespace WebKit {

class WebPage;

class WebDiagnosticLoggingClient : public WebCore::DiagnosticLoggingClient {
public:
    WebDiagnosticLoggingClient(WebPage&);
    virtual ~WebDiagnosticLoggingClient();

private:
    virtual void logDiagnosticMessage(const String& message, const String& description, WebCore::ShouldSample) override;
    virtual void logDiagnosticMessageWithResult(const String& message, const String& description, WebCore::DiagnosticLoggingResultType, WebCore::ShouldSample) override;
    virtual void logDiagnosticMessageWithValue(const String& message, const String& description, const String& value, WebCore::ShouldSample) override;

    WebPage& m_page;
};

}

#endif
