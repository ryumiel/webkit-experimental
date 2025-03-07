/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004-2011, 2014 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "CachedResource.h"

#include "CachedResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "CachedResourceHandle.h"
#include "CachedResourceLoader.h"
#include "CrossOriginAccessControl.h"
#include "DiagnosticLoggingClient.h"
#include "DiagnosticLoggingKeys.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "HTTPHeaderNames.h"
#include "InspectorInstrumentation.h"
#include "URL.h"
#include "LoaderStrategy.h"
#include "Logging.h"
#include "MainFrame.h"
#include "MemoryCache.h"
#include "PlatformStrategies.h"
#include "ResourceHandle.h"
#include "ResourceLoadScheduler.h"
#include "SchemeRegistry.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include "SubresourceLoader.h"
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>
#include <wtf/Vector.h>

#if USE(QUICK_LOOK)
#include "QuickLook.h"
#endif

using namespace WTF;

namespace WebCore {

static ResourceLoadPriority defaultPriorityForResourceType(CachedResource::Type type)
{
    switch (type) {
    case CachedResource::MainResource:
        return ResourceLoadPriorityVeryHigh;
    case CachedResource::CSSStyleSheet:
        return ResourceLoadPriorityHigh;
    case CachedResource::Script:
#if ENABLE(SVG_FONTS)
    case CachedResource::SVGFontResource:
#endif
    case CachedResource::FontResource:
    case CachedResource::RawResource:
        return ResourceLoadPriorityMedium;
    case CachedResource::ImageResource:
        return ResourceLoadPriorityLow;
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
        return ResourceLoadPriorityHigh;
#endif
    case CachedResource::SVGDocumentResource:
        return ResourceLoadPriorityLow;
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkPrefetch:
        return ResourceLoadPriorityVeryLow;
    case CachedResource::LinkSubresource:
        return ResourceLoadPriorityVeryLow;
#endif
#if ENABLE(VIDEO_TRACK)
    case CachedResource::TextTrackResource:
        return ResourceLoadPriorityLow;
#endif
    }
    ASSERT_NOT_REACHED();
    return ResourceLoadPriorityLow;
}

static std::chrono::milliseconds deadDecodedDataDeletionIntervalForResourceType(CachedResource::Type type)
{
    if (type == CachedResource::Script)
        return std::chrono::milliseconds { 0 };

    return MemoryCache::singleton().deadDecodedDataDeletionInterval();
}

DEFINE_DEBUG_ONLY_GLOBAL(RefCountedLeakCounter, cachedResourceLeakCounter, ("CachedResource"));

CachedResource::CachedResource(const ResourceRequest& request, Type type, SessionID sessionID)
    : m_resourceRequest(request)
    , m_decodedDataDeletionTimer(*this, &CachedResource::decodedDataDeletionTimerFired, deadDecodedDataDeletionIntervalForResourceType(type))
    , m_sessionID(sessionID)
    , m_loadPriority(defaultPriorityForResourceType(type))
    , m_responseTimestamp(currentTime())
    , m_lastDecodedAccessTime(0)
    , m_loadFinishTime(0)
    , m_encodedSize(0)
    , m_decodedSize(0)
    , m_accessCount(0)
    , m_handleCount(0)
    , m_preloadCount(0)
    , m_preloadResult(PreloadNotReferenced)
    , m_requestedFromNetworkingLayer(false)
    , m_inCache(false)
    , m_loading(false)
    , m_switchingClientsToRevalidatedResource(false)
    , m_type(type)
    , m_status(Pending)
#ifndef NDEBUG
    , m_deleted(false)
    , m_lruIndex(0)
#endif
    , m_owningCachedResourceLoader(0)
    , m_resourceToRevalidate(0)
    , m_proxyResource(0)
{
    ASSERT(m_type == unsigned(type)); // m_type is a bitfield, so this tests careless updates of the enum.
    ASSERT(sessionID.isValid());
#ifndef NDEBUG
    cachedResourceLeakCounter.increment();
#endif

    if (!m_resourceRequest.url().hasFragmentIdentifier())
        return;
    URL urlForCache = MemoryCache::removeFragmentIdentifierIfNeeded(m_resourceRequest.url());
    if (urlForCache.hasFragmentIdentifier())
        return;
    m_fragmentIdentifierForRequest = m_resourceRequest.url().fragmentIdentifier();
    m_resourceRequest.setURL(urlForCache);
}

CachedResource::~CachedResource()
{
    ASSERT(!m_resourceToRevalidate); // Should be true because canDelete() checks this.
    ASSERT(canDelete());
    ASSERT(!inCache());
    ASSERT(!m_deleted);
    ASSERT(url().isNull() || MemoryCache::singleton().resourceForRequest(resourceRequest(), sessionID()) != this);

#ifndef NDEBUG
    m_deleted = true;
    cachedResourceLeakCounter.decrement();
#endif

    if (m_owningCachedResourceLoader)
        m_owningCachedResourceLoader->removeCachedResource(*this);
}

void CachedResource::failBeforeStarting()
{
    // FIXME: What if resources in other frames were waiting for this revalidation?
    LOG(ResourceLoading, "Cannot start loading '%s'", url().string().latin1().data());
    if (m_resourceToRevalidate) 
        MemoryCache::singleton().revalidationFailed(*this);
    error(CachedResource::LoadError);
}

void CachedResource::addAdditionalRequestHeaders(CachedResourceLoader& cachedResourceLoader)
{
    // Note: We skip the Content-Security-Policy check here because we check
    // the Content-Security-Policy at the CachedResourceLoader layer so we can
    // handle different resource types differently.

    FrameLoader& frameLoader = cachedResourceLoader.frame()->loader();
    String outgoingReferrer;
    String outgoingOrigin;
    if (m_resourceRequest.httpReferrer().isNull()) {
        outgoingReferrer = frameLoader.outgoingReferrer();
        outgoingOrigin = frameLoader.outgoingOrigin();
    } else {
        outgoingReferrer = m_resourceRequest.httpReferrer();
        outgoingOrigin = SecurityOrigin::createFromString(outgoingReferrer)->toString();
    }

    outgoingReferrer = SecurityPolicy::generateReferrerHeader(cachedResourceLoader.document()->referrerPolicy(), m_resourceRequest.url(), outgoingReferrer);
    if (outgoingReferrer.isEmpty())
        m_resourceRequest.clearHTTPReferrer();
    else
        m_resourceRequest.setHTTPReferrer(outgoingReferrer);
    FrameLoader::addHTTPOriginIfNeeded(m_resourceRequest, outgoingOrigin);

    frameLoader.addExtraFieldsToSubresourceRequest(m_resourceRequest);
}

void CachedResource::load(CachedResourceLoader& cachedResourceLoader, const ResourceLoaderOptions& options)
{
    if (!cachedResourceLoader.frame()) {
        failBeforeStarting();
        return;
    }

    FrameLoader& frameLoader = cachedResourceLoader.frame()->loader();
    if (options.securityCheck() == DoSecurityCheck && (frameLoader.state() == FrameStateProvisional || !frameLoader.activeDocumentLoader() || frameLoader.activeDocumentLoader()->isStopping())) {
        failBeforeStarting();
        return;
    }

    m_options = options;
    m_loading = true;

#if USE(QUICK_LOOK)
    if (!m_resourceRequest.isNull() && m_resourceRequest.url().protocolIs(QLPreviewProtocol())) {
        // When QuickLook is invoked to convert a document, it returns a unique URL in the
        // NSURLReponse for the main document. To make safeQLURLForDocumentURLAndResourceURL()
        // work, we need to use the QL URL not the original URL.
        const URL& documentURL = cachedResourceLoader.frame() ? cachedResourceLoader.frame()->loader().documentLoader()->response().url() : cachedResourceLoader.document()->url();
        m_resourceRequest.setURL(safeQLURLForDocumentURLAndResourceURL(documentURL, url()));
    }
#endif

    if (!accept().isEmpty())
        m_resourceRequest.setHTTPAccept(accept());

    if (isCacheValidator()) {
        CachedResource* resourceToRevalidate = m_resourceToRevalidate;
        ASSERT(resourceToRevalidate->canUseCacheValidator());
        ASSERT(resourceToRevalidate->isLoaded());
        const String& lastModified = resourceToRevalidate->response().httpHeaderField(HTTPHeaderName::LastModified);
        const String& eTag = resourceToRevalidate->response().httpHeaderField(HTTPHeaderName::ETag);
        if (!lastModified.isEmpty() || !eTag.isEmpty()) {
            ASSERT(cachedResourceLoader.cachePolicy(type()) != CachePolicyReload);
            if (cachedResourceLoader.cachePolicy(type()) == CachePolicyRevalidate)
                m_resourceRequest.setHTTPHeaderField(HTTPHeaderName::CacheControl, "max-age=0");
            if (!lastModified.isEmpty())
                m_resourceRequest.setHTTPHeaderField(HTTPHeaderName::IfModifiedSince, lastModified);
            if (!eTag.isEmpty())
                m_resourceRequest.setHTTPHeaderField(HTTPHeaderName::IfNoneMatch, eTag);
        }
    }

#if ENABLE(LINK_PREFETCH)
    if (type() == CachedResource::LinkPrefetch || type() == CachedResource::LinkSubresource)
        m_resourceRequest.setHTTPHeaderField(HTTPHeaderName::Purpose, "prefetch");
#endif
    m_resourceRequest.setPriority(loadPriority());

    if (type() != MainResource)
        addAdditionalRequestHeaders(cachedResourceLoader);

    // FIXME: It's unfortunate that the cache layer and below get to know anything about fragment identifiers.
    // We should look into removing the expectation of that knowledge from the platform network stacks.
    ResourceRequest request(m_resourceRequest);
    if (!m_fragmentIdentifierForRequest.isNull()) {
        URL url = request.url();
        url.setFragmentIdentifier(m_fragmentIdentifierForRequest);
        request.setURL(url);
        m_fragmentIdentifierForRequest = String();
    }

    m_loader = platformStrategies()->loaderStrategy()->resourceLoadScheduler()->scheduleSubresourceLoad(cachedResourceLoader.frame(), this, request, options);
    if (!m_loader) {
        failBeforeStarting();
        return;
    }

    m_status = Pending;
}

void CachedResource::checkNotify()
{
    if (isLoading() || stillNeedsLoad())
        return;

    CachedResourceClientWalker<CachedResourceClient> walker(m_clients);
    while (CachedResourceClient* client = walker.next())
        client->notifyFinished(this);
}

void CachedResource::addDataBuffer(SharedBuffer&)
{
    ASSERT(dataBufferingPolicy() == BufferData);
}

void CachedResource::addData(const char*, unsigned)
{
    ASSERT(dataBufferingPolicy() == DoNotBufferData);
}

void CachedResource::finishLoading(SharedBuffer*)
{
    setLoading(false);
    checkNotify();
}

void CachedResource::error(CachedResource::Status status)
{
    setStatus(status);
    ASSERT(errorOccurred());
    m_data.clear();

    setLoading(false);
    checkNotify();
}
    
void CachedResource::cancelLoad()
{
    if (!isLoading() && !stillNeedsLoad())
        return;

    setStatus(LoadError);
    setLoading(false);
    checkNotify();
}

void CachedResource::finish()
{
    if (!errorOccurred())
        m_status = Cached;
}

bool CachedResource::passesAccessControlCheck(SecurityOrigin* securityOrigin)
{
    String errorDescription;
    return WebCore::passesAccessControlCheck(m_response, resourceRequest().allowCookies() ? AllowStoredCredentials : DoNotAllowStoredCredentials, securityOrigin, errorDescription);
}

bool CachedResource::isExpired() const
{
    if (m_response.isNull())
        return false;

    return computeCurrentAge(m_response, m_responseTimestamp) > freshnessLifetime(m_response);
}

double CachedResource::freshnessLifetime(const ResourceResponse& response) const
{
    if (!response.url().protocolIsInHTTPFamily()) {
        // Don't cache non-HTTP main resources since we can't check for freshness.
        // FIXME: We should not cache subresources either, but when we tried this
        // it caused performance and flakiness issues in our test infrastructure.
        if (m_type == MainResource && !SchemeRegistry::shouldCacheResponsesFromURLSchemeIndefinitely(response.url().protocol()))
            return 0;

        return std::numeric_limits<double>::max();
    }

    return computeFreshnessLifetimeForHTTPFamily(response, m_responseTimestamp);
}

void CachedResource::willSendRequest(ResourceRequest&, const ResourceResponse& response)
{
    m_requestedFromNetworkingLayer = true;
    if (response.isNull())
        return;
    updateRedirectChainStatus(m_redirectChainCacheStatus, response);
}

void CachedResource::responseReceived(const ResourceResponse& response)
{
    setResponse(response);
    m_responseTimestamp = currentTime();
    String encoding = response.textEncodingName();
    if (!encoding.isNull())
        setEncoding(encoding);
}

void CachedResource::clearLoader()
{
    ASSERT(m_loader);
    m_loader = nullptr;
    deleteIfPossible();
}

void CachedResource::addClient(CachedResourceClient* client)
{
    if (addClientToSet(client))
        didAddClient(client);
}

void CachedResource::didAddClient(CachedResourceClient* client)
{
    if (m_decodedDataDeletionTimer.isActive())
        m_decodedDataDeletionTimer.stop();

    if (m_clientsAwaitingCallback.remove(client))
        m_clients.add(client);
    if (!isLoading() && !stillNeedsLoad())
        client->notifyFinished(this);
}

bool CachedResource::addClientToSet(CachedResourceClient* client)
{
    if (m_preloadResult == PreloadNotReferenced) {
        if (isLoaded())
            m_preloadResult = PreloadReferencedWhileComplete;
        else if (m_requestedFromNetworkingLayer)
            m_preloadResult = PreloadReferencedWhileLoading;
        else
            m_preloadResult = PreloadReferenced;
    }
    if (!hasClients() && inCache())
        MemoryCache::singleton().addToLiveResourcesSize(*this);

    if ((m_type == RawResource || m_type == MainResource) && !m_response.isNull() && !m_proxyResource) {
        // Certain resources (especially XHRs and main resources) do crazy things if an asynchronous load returns
        // synchronously (e.g., scripts may not have set all the state they need to handle the load).
        // Therefore, rather than immediately sending callbacks on a cache hit like other CachedResources,
        // we schedule the callbacks and ensure we never finish synchronously.
        ASSERT(!m_clientsAwaitingCallback.contains(client));
        m_clientsAwaitingCallback.add(client, std::make_unique<Callback>(*this, *client));
        return false;
    }

    m_clients.add(client);
    return true;
}

void CachedResource::removeClient(CachedResourceClient* client)
{
    auto callback = m_clientsAwaitingCallback.take(client);
    if (callback) {
        ASSERT(!m_clients.contains(client));
        callback->cancel();
        callback = nullptr;
    } else {
        ASSERT(m_clients.contains(client));
        m_clients.remove(client);
        didRemoveClient(client);
    }

    bool deleted = deleteIfPossible();
    if (!deleted && !hasClients()) {
        auto& memoryCache = MemoryCache::singleton();
        if (inCache()) {
            memoryCache.removeFromLiveResourcesSize(*this);
            memoryCache.removeFromLiveDecodedResourcesList(*this);
        }
        if (!m_switchingClientsToRevalidatedResource)
            allClientsRemoved();
        destroyDecodedDataIfNeeded();
        if (response().cacheControlContainsNoStore() && url().protocolIs("https")) {
            // RFC2616 14.9.2:
            // "no-store: ... MUST make a best-effort attempt to remove the information from volatile storage as promptly as possible"
            // "... History buffers MAY store such responses as part of their normal operation."
            // We allow non-secure content to be reused in history, but we do not allow secure content to be reused.
            memoryCache.remove(*this);
        }
        memoryCache.prune();
    }
    // This object may be dead here.
}

void CachedResource::destroyDecodedDataIfNeeded()
{
    if (!m_decodedSize)
        return;
    if (!MemoryCache::singleton().deadDecodedDataDeletionInterval().count())
        return;
    m_decodedDataDeletionTimer.restart();
}

void CachedResource::decodedDataDeletionTimerFired()
{
    destroyDecodedData();
}

bool CachedResource::deleteIfPossible()
{
    if (canDelete() && !inCache()) {
        InspectorInstrumentation::willDestroyCachedResource(*this);
        delete this;
        return true;
    }
    return false;
}

void CachedResource::setDecodedSize(unsigned size)
{
    if (size == m_decodedSize)
        return;

    int delta = size - m_decodedSize;

    // The object must be moved to a different queue, since its size has been changed.
    // Remove before updating m_decodedSize, so we find the resource in the correct LRU list.
    if (inCache())
        MemoryCache::singleton().removeFromLRUList(*this);
    
    m_decodedSize = size;
   
    if (inCache()) {
        auto& memoryCache = MemoryCache::singleton();
        // Now insert into the new LRU list.
        memoryCache.insertInLRUList(*this);
        
        // Insert into or remove from the live decoded list if necessary.
        // When inserting into the LiveDecodedResourcesList it is possible
        // that the m_lastDecodedAccessTime is still zero or smaller than
        // the m_lastDecodedAccessTime of the current list head. This is a
        // violation of the invariant that the list is to be kept sorted
        // by access time. The weakening of the invariant does not pose
        // a problem. For more details please see: https://bugs.webkit.org/show_bug.cgi?id=30209
        bool inLiveDecodedResourcesList = memoryCache.inLiveDecodedResourcesList(*this);
        if (m_decodedSize && !inLiveDecodedResourcesList && hasClients())
            memoryCache.insertInLiveDecodedResourcesList(*this);
        else if (!m_decodedSize && inLiveDecodedResourcesList)
            memoryCache.removeFromLiveDecodedResourcesList(*this);

        // Update the cache's size totals.
        memoryCache.adjustSize(hasClients(), delta);
    }
}

void CachedResource::setEncodedSize(unsigned size)
{
    if (size == m_encodedSize)
        return;

    int delta = size - m_encodedSize;

    // The object must be moved to a different queue, since its size has been changed.
    // Remove before updating m_encodedSize, so we find the resource in the correct LRU list.
    if (inCache())
        MemoryCache::singleton().removeFromLRUList(*this);

    m_encodedSize = size;

    if (inCache()) {
        auto& memoryCache = MemoryCache::singleton();
        memoryCache.insertInLRUList(*this);
        memoryCache.adjustSize(hasClients(), delta);
    }
}

void CachedResource::didAccessDecodedData(double timeStamp)
{
    m_lastDecodedAccessTime = timeStamp;
    
    if (inCache()) {
        auto& memoryCache = MemoryCache::singleton();
        if (memoryCache.inLiveDecodedResourcesList(*this)) {
            memoryCache.removeFromLiveDecodedResourcesList(*this);
            memoryCache.insertInLiveDecodedResourcesList(*this);
        }
        memoryCache.prune();
    }
}
    
void CachedResource::setResourceToRevalidate(CachedResource* resource) 
{ 
    ASSERT(resource);
    ASSERT(!m_resourceToRevalidate);
    ASSERT(resource != this);
    ASSERT(m_handlesToRevalidate.isEmpty());
    ASSERT(resource->type() == type());

    LOG(ResourceLoading, "CachedResource %p setResourceToRevalidate %p", this, resource);

    // The following assert should be investigated whenever it occurs. Although it should never fire, it currently does in rare circumstances.
    // https://bugs.webkit.org/show_bug.cgi?id=28604.
    // So the code needs to be robust to this assert failing thus the "if (m_resourceToRevalidate->m_proxyResource == this)" in CachedResource::clearResourceToRevalidate.
    ASSERT(!resource->m_proxyResource);

    resource->m_proxyResource = this;
    m_resourceToRevalidate = resource;
}

void CachedResource::clearResourceToRevalidate() 
{ 
    ASSERT(m_resourceToRevalidate);
    if (m_switchingClientsToRevalidatedResource)
        return;

    // A resource may start revalidation before this method has been called, so check that this resource is still the proxy resource before clearing it out.
    if (m_resourceToRevalidate->m_proxyResource == this) {
        m_resourceToRevalidate->m_proxyResource = 0;
        m_resourceToRevalidate->deleteIfPossible();
    }
    m_handlesToRevalidate.clear();
    m_resourceToRevalidate = 0;
    deleteIfPossible();
}
    
void CachedResource::switchClientsToRevalidatedResource()
{
    ASSERT(m_resourceToRevalidate);
    ASSERT(m_resourceToRevalidate->inCache());
    ASSERT(!inCache());

    LOG(ResourceLoading, "CachedResource %p switchClientsToRevalidatedResource %p", this, m_resourceToRevalidate);

    m_switchingClientsToRevalidatedResource = true;
    HashSet<CachedResourceHandleBase*>::iterator end = m_handlesToRevalidate.end();
    for (HashSet<CachedResourceHandleBase*>::iterator it = m_handlesToRevalidate.begin(); it != end; ++it) {
        CachedResourceHandleBase* handle = *it;
        handle->m_resource = m_resourceToRevalidate;
        m_resourceToRevalidate->registerHandle(handle);
        --m_handleCount;
    }
    ASSERT(!m_handleCount);
    m_handlesToRevalidate.clear();

    Vector<CachedResourceClient*> clientsToMove;
    HashCountedSet<CachedResourceClient*>::iterator end2 = m_clients.end();
    for (HashCountedSet<CachedResourceClient*>::iterator it = m_clients.begin(); it != end2; ++it) {
        CachedResourceClient* client = it->key;
        unsigned count = it->value;
        while (count) {
            clientsToMove.append(client);
            --count;
        }
    }

    unsigned moveCount = clientsToMove.size();
    for (unsigned n = 0; n < moveCount; ++n)
        removeClient(clientsToMove[n]);
    ASSERT(m_clients.isEmpty());

    for (unsigned n = 0; n < moveCount; ++n)
        m_resourceToRevalidate->addClientToSet(clientsToMove[n]);
    for (unsigned n = 0; n < moveCount; ++n) {
        // Calling didAddClient may do anything, including trying to cancel revalidation.
        // Assert that it didn't succeed.
        ASSERT(m_resourceToRevalidate);
        // Calling didAddClient for a client may end up removing another client. In that case it won't be in the set anymore.
        if (m_resourceToRevalidate->m_clients.contains(clientsToMove[n]))
            m_resourceToRevalidate->didAddClient(clientsToMove[n]);
    }
    m_switchingClientsToRevalidatedResource = false;
}

void CachedResource::updateResponseAfterRevalidation(const ResourceResponse& validatingResponse)
{
    m_responseTimestamp = currentTime();

    updateResponseHeadersAfterRevalidation(m_response, validatingResponse);
}

void CachedResource::registerHandle(CachedResourceHandleBase* h)
{
    ++m_handleCount;
    if (m_resourceToRevalidate)
        m_handlesToRevalidate.add(h);
}

void CachedResource::unregisterHandle(CachedResourceHandleBase* h)
{
    ASSERT(m_handleCount > 0);
    --m_handleCount;

    if (m_resourceToRevalidate)
         m_handlesToRevalidate.remove(h);

    if (!m_handleCount)
        deleteIfPossible();
}

bool CachedResource::canUseCacheValidator() const
{
    if (m_loading || errorOccurred())
        return false;

    if (m_response.cacheControlContainsNoStore())
        return false;
    return m_response.hasCacheValidatorFields();
}

static inline void logResourceRevalidationReason(Frame* frame, const String& reason)
{
    if (frame)
        frame->mainFrame().diagnosticLoggingClient().logDiagnosticMessageWithValue(DiagnosticLoggingKeys::cachedResourceRevalidationKey(), DiagnosticLoggingKeys::reasonKey(), reason, ShouldSample::Yes);
}

bool CachedResource::mustRevalidateDueToCacheHeaders(const CachedResourceLoader& cachedResourceLoader, CachePolicy cachePolicy) const
{    
    ASSERT(cachePolicy == CachePolicyRevalidate || cachePolicy == CachePolicyVerify);

    if (cachePolicy == CachePolicyRevalidate) {
        logResourceRevalidationReason(cachedResourceLoader.frame(), DiagnosticLoggingKeys::reloadKey());
        return true;
    }

    if (m_response.cacheControlContainsNoCache() || m_response.cacheControlContainsNoStore()) {
        LOG(ResourceLoading, "CachedResource %p mustRevalidate because of m_response.cacheControlContainsNoCache() || m_response.cacheControlContainsNoStore()\n", this);
        if (m_response.cacheControlContainsNoStore())
            logResourceRevalidationReason(cachedResourceLoader.frame(), DiagnosticLoggingKeys::noStoreKey());
        else
            logResourceRevalidationReason(cachedResourceLoader.frame(), DiagnosticLoggingKeys::noCacheKey());

        return true;
    }

    // CachePolicyVerify
    if (isExpired()) {
        LOG(ResourceLoading, "CachedResource %p mustRevalidate because of isExpired()\n", this);
        logResourceRevalidationReason(cachedResourceLoader.frame(), DiagnosticLoggingKeys::isExpiredKey());
        return true;
    }

    return false;
}

bool CachedResource::redirectChainAllowsReuse(ReuseExpiredRedirectionOrNot reuseExpiredRedirection) const
{
    return WebCore::redirectChainAllowsReuse(m_redirectChainCacheStatus, reuseExpiredRedirection);
}

unsigned CachedResource::overheadSize() const
{
    static const int kAverageClientsHashMapSize = 384;
    return sizeof(CachedResource) + m_response.memoryUsage() + kAverageClientsHashMapSize + m_resourceRequest.url().string().length() * 2;
}

bool CachedResource::areAllClientsXMLHttpRequests() const
{
    if (type() != RawResource)
        return false;

    for (auto& client : m_clients) {
        if (!client.key->isXMLHttpRequest())
            return false;
    }
    return true;
}

void CachedResource::setLoadPriority(const Optional<ResourceLoadPriority>& loadPriority)
{
    if (loadPriority)
        m_loadPriority = loadPriority.value();
    else
        m_loadPriority = defaultPriorityForResourceType(type());
}

inline CachedResource::Callback::Callback(CachedResource& resource, CachedResourceClient& client)
    : m_resource(resource)
    , m_client(client)
    , m_timer(*this, &Callback::timerFired)
{
    m_timer.startOneShot(0);
}

inline void CachedResource::Callback::cancel()
{
    if (m_timer.isActive())
        m_timer.stop();
}

void CachedResource::Callback::timerFired()
{
    m_resource.didAddClient(&m_client);
}

#if USE(FOUNDATION)

void CachedResource::tryReplaceEncodedData(SharedBuffer& newBuffer)
{
    if (!m_data)
        return;
    
    if (!mayTryReplaceEncodedData())
        return;

    // We have to do the memcmp because we can't tell if the replacement file backed data is for the
    // same resource or if we made a second request with the same URL which gave us a different
    // resource. We have seen this happen for cached POST resources.
    if (m_data->size() != newBuffer.size() || memcmp(m_data->data(), newBuffer.data(), m_data->size()))
        return;

    m_data->tryReplaceContentsWithPlatformBuffer(newBuffer);
}

#endif

}
