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
#include "CacheValidation.h"

#include "HTTPHeaderMap.h"
#include "ResourceResponse.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

// These response headers are not copied from a revalidated response to the
// cached response headers. For compatibility, this list is based on Chromium's
// net/http/http_response_headers.cc.
const char* const headersToIgnoreAfterRevalidation[] = {
    "allow",
    "connection",
    "etag",
    "keep-alive",
    "last-modified"
    "proxy-authenticate",
    "proxy-connection",
    "trailer",
    "transfer-encoding",
    "upgrade",
    "www-authenticate",
    "x-frame-options",
    "x-xss-protection",
};

// Some header prefixes mean "Don't copy this header from a 304 response.".
// Rather than listing all the relevant headers, we can consolidate them into
// this list, also grabbed from Chromium's net/http/http_response_headers.cc.
const char* const headerPrefixesToIgnoreAfterRevalidation[] = {
    "content-",
    "x-content-",
    "x-webkit-"
};

static inline bool shouldUpdateHeaderAfterRevalidation(const String& header)
{
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(headersToIgnoreAfterRevalidation); i++) {
        if (equalIgnoringCase(header, headersToIgnoreAfterRevalidation[i]))
            return false;
    }
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(headerPrefixesToIgnoreAfterRevalidation); i++) {
        if (header.startsWith(headerPrefixesToIgnoreAfterRevalidation[i], false))
            return false;
    }
    return true;
}

void updateResponseHeadersAfterRevalidation(ResourceResponse& response, const ResourceResponse& validatingResponse)
{
    // RFC2616 10.3.5
    // Update cached headers from the 304 response
    for (const auto& header : validatingResponse.httpHeaderFields()) {
        // Entity headers should not be sent by servers when generating a 304
        // response; misconfigured servers send them anyway. We shouldn't allow
        // such headers to update the original request. We'll base this on the
        // list defined by RFC2616 7.1, with a few additions for extension headers
        // we care about.
        if (!shouldUpdateHeaderAfterRevalidation(header.key))
            continue;
        response.setHTTPHeaderField(header.key, header.value);
    }
}

double computeCurrentAge(const ResourceResponse& response, double responseTimestamp)
{
    // RFC2616 13.2.3
    // No compensation for latency as that is not terribly important in practice
    double dateValue = response.date();
    double apparentAge = std::isfinite(dateValue) ? std::max(0., responseTimestamp - dateValue) : 0;
    double ageValue = response.age();
    double correctedReceivedAge = std::isfinite(ageValue) ? std::max(apparentAge, ageValue) : apparentAge;
    double residentTime = currentTime() - responseTimestamp;
    return correctedReceivedAge + residentTime;
}

double computeFreshnessLifetimeForHTTPFamily(const ResourceResponse& response, double responseTimestamp)
{
    ASSERT(response.url().protocolIsInHTTPFamily());
    // RFC2616 13.2.4
    double maxAgeValue = response.cacheControlMaxAge();
    if (std::isfinite(maxAgeValue))
        return maxAgeValue;
    double expiresValue = response.expires();
    double dateValue = response.date();
    double creationTime = std::isfinite(dateValue) ? dateValue : responseTimestamp;
    if (std::isfinite(expiresValue))
        return expiresValue - creationTime;
    double lastModifiedValue = response.lastModified();
    if (std::isfinite(lastModifiedValue))
        return (creationTime - lastModifiedValue) * 0.1;
    // If no cache headers are present, the specification leaves the decision to the UA. Other browsers seem to opt for 0.
    return 0;
}

void updateRedirectChainStatus(RedirectChainCacheStatus& redirectChainCacheStatus, const ResourceResponse& response)
{
    if (redirectChainCacheStatus.status == RedirectChainCacheStatus::NotCachedRedirection)
        return;
    if (response.cacheControlContainsNoStore() || response.cacheControlContainsNoCache() || response.cacheControlContainsMustRevalidate()) {
        redirectChainCacheStatus.status = RedirectChainCacheStatus::NotCachedRedirection;
        return;
    }
    redirectChainCacheStatus.status = RedirectChainCacheStatus::CachedRedirection;
    double responseTimestamp = currentTime();
    // Store the nearest end of cache validity date
    double endOfValidity = responseTimestamp + computeFreshnessLifetimeForHTTPFamily(response, responseTimestamp) - computeCurrentAge(response, responseTimestamp);
    redirectChainCacheStatus.endOfValidity = std::min(redirectChainCacheStatus.endOfValidity, endOfValidity);
}

bool redirectChainAllowsReuse(RedirectChainCacheStatus redirectChainCacheStatus, ReuseExpiredRedirectionOrNot reuseExpiredRedirection)
{
    switch (redirectChainCacheStatus.status) {
    case RedirectChainCacheStatus::NoRedirection:
        return true;
    case RedirectChainCacheStatus::NotCachedRedirection:
        return false;
    case RedirectChainCacheStatus::CachedRedirection:
        return reuseExpiredRedirection || currentTime() <= redirectChainCacheStatus.endOfValidity;
    }
    ASSERT_NOT_REACHED();
    return false;
}

inline bool isCacheHeaderSeparator(UChar c)
{
    // See RFC 2616, Section 2.2
    switch (c) {
    case '(':
    case ')':
    case '<':
    case '>':
    case '@':
    case ',':
    case ';':
    case ':':
    case '\\':
    case '"':
    case '/':
    case '[':
    case ']':
    case '?':
    case '=':
    case '{':
    case '}':
    case ' ':
    case '\t':
        return true;
    default:
        return false;
    }
}

inline bool isControlCharacter(UChar c)
{
    return c < ' ' || c == 127;
}

inline String trimToNextSeparator(const String& str)
{
    return str.substring(0, str.find(isCacheHeaderSeparator));
}

static Vector<std::pair<String, String>> parseCacheHeader(const String& header)
{
    Vector<std::pair<String, String>> result;

    const String safeHeader = header.removeCharacters(isControlCharacter);
    unsigned max = safeHeader.length();
    unsigned pos = 0;
    while (pos < max) {
        size_t nextCommaPosition = safeHeader.find(',', pos);
        size_t nextEqualSignPosition = safeHeader.find('=', pos);
        if (nextEqualSignPosition == notFound && nextCommaPosition == notFound) {
            // Add last directive to map with empty string as value
            result.append(std::make_pair(trimToNextSeparator(safeHeader.substring(pos, max - pos).stripWhiteSpace()), ""));
            return result;
        }
        if (nextCommaPosition != notFound && (nextCommaPosition < nextEqualSignPosition || nextEqualSignPosition == notFound)) {
            // Add directive to map with empty string as value
            result.append(std::make_pair(trimToNextSeparator(safeHeader.substring(pos, nextCommaPosition - pos).stripWhiteSpace()), ""));
            pos += nextCommaPosition - pos + 1;
            continue;
        }
        // Get directive name, parse right hand side of equal sign, then add to map
        String directive = trimToNextSeparator(safeHeader.substring(pos, nextEqualSignPosition - pos).stripWhiteSpace());
        pos += nextEqualSignPosition - pos + 1;

        String value = safeHeader.substring(pos, max - pos).stripWhiteSpace();
        if (value[0] == '"') {
            // The value is a quoted string
            size_t nextDoubleQuotePosition = value.find('"', 1);
            if (nextDoubleQuotePosition == notFound) {
                // Parse error; just use the rest as the value
                result.append(std::make_pair(directive, trimToNextSeparator(value.substring(1, value.length() - 1).stripWhiteSpace())));
                return result;
            }
            // Store the value as a quoted string without quotes
            result.append(std::make_pair(directive, value.substring(1, nextDoubleQuotePosition - 1).stripWhiteSpace()));
            pos += (safeHeader.find('"', pos) - pos) + nextDoubleQuotePosition + 1;
            // Move past next comma, if there is one
            size_t nextCommaPosition2 = safeHeader.find(',', pos);
            if (nextCommaPosition2 == notFound)
                return result; // Parse error if there is anything left with no comma
            pos += nextCommaPosition2 - pos + 1;
            continue;
        }
        // The value is a token until the next comma
        size_t nextCommaPosition2 = value.find(',');
        if (nextCommaPosition2 == notFound) {
            // The rest is the value; no change to value needed
            result.append(std::make_pair(directive, trimToNextSeparator(value)));
            return result;
        }
        // The value is delimited by the next comma
        result.append(std::make_pair(directive, trimToNextSeparator(value.substring(0, nextCommaPosition2).stripWhiteSpace())));
        pos += (safeHeader.find(',', pos) - pos) + 1;
    }
    return result;
}

CacheControlDirectives parseCacheControlDirectives(const HTTPHeaderMap& headers)
{
    CacheControlDirectives result;

    String cacheControlValue = headers.get(HTTPHeaderName::CacheControl);
    if (!cacheControlValue.isEmpty()) {
        auto directives = parseCacheHeader(cacheControlValue);

        size_t directivesSize = directives.size();
        for (size_t i = 0; i < directivesSize; ++i) {
            // RFC2616 14.9.1: A no-cache directive with a value is only meaningful for proxy caches.
            // It should be ignored by a browser level cache.
            if (equalIgnoringCase(directives[i].first, "no-cache") && directives[i].second.isEmpty())
                result.noCache = true;
            else if (equalIgnoringCase(directives[i].first, "no-store"))
                result.noStore = true;
            else if (equalIgnoringCase(directives[i].first, "must-revalidate"))
                result.mustRevalidate = true;
            else if (equalIgnoringCase(directives[i].first, "max-age")) {
                if (!std::isnan(result.maxAge)) {
                    // First max-age directive wins if there are multiple ones.
                    continue;
                }
                bool ok;
                double maxAge = directives[i].second.toDouble(&ok);
                if (ok)
                    result.maxAge = maxAge;
            } else if (equalIgnoringCase(directives[i].first, "max-stale")) {
                // https://tools.ietf.org/html/rfc7234#section-5.2.1.2
                if (!std::isnan(result.maxStale)) {
                    // First max-stale directive wins if there are multiple ones.
                    continue;
                }
                if (directives[i].second.isEmpty()) {
                    // if no value is assigned to max-stale, then the client is willing to accept a stale response of any age.
                    result.maxStale = std::numeric_limits<double>::max();
                    continue;
                }
                bool ok;
                double maxStale = directives[i].second.toDouble(&ok);
                if (ok)
                    result.maxStale = maxStale;
            }
        }
    }

    if (!result.noCache) {
        // Handle Pragma: no-cache
        // This is deprecated and equivalent to Cache-control: no-cache
        // Don't bother tokenizing the value, it is not important
        String pragmaValue = headers.get(HTTPHeaderName::Pragma);

        result.noCache = pragmaValue.contains("no-cache", false);
    }

    return result;
}

}
