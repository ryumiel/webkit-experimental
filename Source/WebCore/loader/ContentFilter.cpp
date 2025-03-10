/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "ContentFilter.h"

#if ENABLE(CONTENT_FILTERING)

#include "DocumentLoader.h"
#include "Frame.h"
#include "NetworkExtensionContentFilter.h"
#include "ParentalControlsContentFilter.h"
#include "ScriptController.h"
#include <bindings/ScriptValue.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/Vector.h>

namespace WebCore {

Vector<ContentFilter::Type>& ContentFilter::types()
{
    static NeverDestroyed<Vector<ContentFilter::Type>> types {
        Vector<ContentFilter::Type> {
            type<ParentalControlsContentFilter>(),
#if HAVE(NETWORK_EXTENSION)
            type<NetworkExtensionContentFilter>()
#endif
        }
    };
    return types;
}

std::unique_ptr<ContentFilter> ContentFilter::createIfNeeded(const ResourceResponse& response, DocumentLoader& documentLoader)
{
    Container filters;
    for (auto& type : types()) {
        if (type.canHandleResponse(response))
            filters.append(type.create(response));
    }

    if (filters.isEmpty())
        return nullptr;

    return std::make_unique<ContentFilter>(WTF::move(filters), documentLoader);
}

ContentFilter::ContentFilter(Container contentFilters, DocumentLoader& documentLoader)
    : m_contentFilters { WTF::move(contentFilters) }
    , m_documentLoader { documentLoader }
{
    ASSERT(!m_contentFilters.isEmpty());
}

void ContentFilter::addData(const char* data, int length)
{
    ASSERT(needsMoreData());

    for (auto& contentFilter : m_contentFilters)
        contentFilter->addData(data, length);
}
    
void ContentFilter::finishedAddingData()
{
    ASSERT(needsMoreData());

    for (auto& contentFilter : m_contentFilters)
        contentFilter->finishedAddingData();

    ASSERT(!needsMoreData());
}

bool ContentFilter::needsMoreData() const
{
    for (auto& contentFilter : m_contentFilters) {
        if (contentFilter->needsMoreData())
            return true;
    }

    return false;
}

bool ContentFilter::didBlockData() const
{
    for (auto& contentFilter : m_contentFilters) {
        if (contentFilter->didBlockData())
            return true;
    }

    return false;
}

const char* ContentFilter::getReplacementData(int& length) const
{
    ASSERT(!needsMoreData());

    for (auto& contentFilter : m_contentFilters) {
        if (contentFilter->didBlockData())
            return contentFilter->getReplacementData(length);
    }

    return m_contentFilters[0]->getReplacementData(length);
}

ContentFilterUnblockHandler ContentFilter::unblockHandler() const
{
    ASSERT(didBlockData());

    PlatformContentFilter* blockingFilter = nullptr;
    for (auto& contentFilter : m_contentFilters) {
        if (contentFilter->didBlockData()) {
            blockingFilter = contentFilter.get();
            break;
        }
    }
    ASSERT(blockingFilter);

    StringCapture unblockRequestDeniedScript { blockingFilter->unblockRequestDeniedScript() };
    if (unblockRequestDeniedScript.string().isEmpty())
        return blockingFilter->unblockHandler();

    // It would be a layering violation for the unblock handler to access its frame,
    // so we will execute the unblock denied script on its behalf.
    ContentFilterUnblockHandler unblockHandler { blockingFilter->unblockHandler() };
    RefPtr<Frame> frame { m_documentLoader.frame() };
    return ContentFilterUnblockHandler {
        unblockHandler.unblockURLHost(), [unblockHandler, frame, unblockRequestDeniedScript](ContentFilterUnblockHandler::DecisionHandlerFunction decisionHandler) {
            unblockHandler.requestUnblockAsync([decisionHandler, frame, unblockRequestDeniedScript](bool unblocked) {
                decisionHandler(unblocked);
                if (!unblocked && frame)
                    frame->script().executeScript(unblockRequestDeniedScript.string());
            });
        }
    };
}

} // namespace WebCore

#endif // ENABLE(CONTENT_FILTERING)
