/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef ApplicationCache_h
#define ApplicationCache_h

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ApplicationCacheGroup;
class ApplicationCacheResource;
class DocumentLoader;
class URL;
class ResourceRequest;
class SecurityOrigin;

typedef Vector<std::pair<URL, URL>> FallbackURLVector;

class ApplicationCache : public RefCounted<ApplicationCache> {
public:
    static Ref<ApplicationCache> create() { return adoptRef(*new ApplicationCache); }
    
    WEBCORE_EXPORT static void deleteCacheForOrigin(SecurityOrigin*);
    WEBCORE_EXPORT static void deleteAllCaches();

    ~ApplicationCache();

    void addResource(PassRefPtr<ApplicationCacheResource> resource);
    unsigned removeResource(const String& url);
    
    void setManifestResource(PassRefPtr<ApplicationCacheResource> manifest);
    ApplicationCacheResource* manifestResource() const { return m_manifest; }
    
    void setGroup(ApplicationCacheGroup*);
    ApplicationCacheGroup* group() const { return m_group; }

    bool isComplete();

    ApplicationCacheResource* resourceForRequest(const ResourceRequest&);
    ApplicationCacheResource* resourceForURL(const String& url);

    void setAllowsAllNetworkRequests(bool value) { m_allowAllNetworkRequests = value; }
    bool allowsAllNetworkRequests() const { return m_allowAllNetworkRequests; }
    void setOnlineWhitelist(const Vector<URL>& onlineWhitelist);
    const Vector<URL>& onlineWhitelist() const { return m_onlineWhitelist; }
    bool isURLInOnlineWhitelist(const URL&); // There is an entry in online whitelist that has the same origin as the resource's URL and that is a prefix match for the resource's URL.

    void setFallbackURLs(const FallbackURLVector&);
    const FallbackURLVector& fallbackURLs() const { return m_fallbackURLs; }
    bool urlMatchesFallbackNamespace(const URL&, URL* fallbackURL = 0);
    
#ifndef NDEBUG
    void dump();
#endif

    typedef HashMap<String, RefPtr<ApplicationCacheResource>> ResourceMap;
    const ResourceMap& resources() const { return m_resources; }
    
    void setStorageID(unsigned storageID) { m_storageID = storageID; }
    unsigned storageID() const { return m_storageID; }
    void clearStorageID();
    
    static bool requestIsHTTPOrHTTPSGet(const ResourceRequest&);

    WEBCORE_EXPORT static int64_t diskUsageForOrigin(SecurityOrigin*);
    
    int64_t estimatedSizeInStorage() const { return m_estimatedSizeInStorage; }

private:
    ApplicationCache();
    
    ApplicationCacheGroup* m_group;
    ResourceMap m_resources;
    ApplicationCacheResource* m_manifest;

    bool m_allowAllNetworkRequests;
    Vector<URL> m_onlineWhitelist;
    FallbackURLVector m_fallbackURLs;

    // The total size of the resources belonging to this Application Cache instance.
    // This is an estimation of the size this Application Cache occupies in the
    // database file.
    int64_t m_estimatedSizeInStorage;

    unsigned m_storageID;
};

} // namespace WebCore

#endif // ApplicationCache_h
