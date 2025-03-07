/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#ifndef MockContentFilter_h
#define MockContentFilter_h

#include "MockContentFilterSettings.h"
#include "PlatformContentFilter.h"

namespace WebCore {

class MockContentFilter final : public PlatformContentFilter {
    friend std::unique_ptr<MockContentFilter> std::make_unique<MockContentFilter>(const ResourceResponse&);

public:
    static void ensureInstalled();
    static bool canHandleResponse(const ResourceResponse&);
    static std::unique_ptr<MockContentFilter> create(const ResourceResponse&);

    void addData(const char* data, int length) override;
    void finishedAddingData() override;
    bool needsMoreData() const override;
    bool didBlockData() const override;
    const char* getReplacementData(int& length) const override;
    ContentFilterUnblockHandler unblockHandler() const override;
    String unblockRequestDeniedScript() const override;

private:
    enum class Status {
        NeedsMoreData,
        Allowed,
        Blocked
    };

    explicit MockContentFilter(const ResourceResponse&);
    void maybeDetermineStatus(MockContentFilterSettings::DecisionPoint);

    Vector<char> m_replacementData;
    Status m_status { Status::NeedsMoreData };
};

} // namespace WebCore

#endif // MockContentFilter_h
