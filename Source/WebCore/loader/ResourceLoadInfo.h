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

#ifndef ResourceLoadInfo_h
#define ResourceLoadInfo_h

#include "CachedResource.h"
#include "URL.h"

namespace WebCore {

enum class ResourceType : uint16_t {
    Invalid = 0x0000,
    Document = 0x0001,
    Image = 0x0002,
    StyleSheet = 0x0004,
    Script = 0x0008,
    Font = 0x0010,
    Raw = 0x0020,
    SVGDocument = 0x0040,
    Media = 0x0080,
    PlugInStream = 0x0100,
};

enum class LoadType : uint16_t {
    Invalid = 0x0000,
    FirstParty = 0x0200,
    ThirdParty = 0x0400,
};

typedef uint16_t ResourceFlags;

ResourceType toResourceType(CachedResource::Type);
uint16_t readResourceType(const String&);
uint16_t readLoadType(const String&);

struct ResourceLoadInfo {
    URL resourceURL;
    URL mainDocumentURL;
    ResourceType type;

    bool isThirdParty() const;
    ResourceFlags getResourceFlags() const;
};


} // namespace WebCore

#endif // ResourceLoadInfo_h
