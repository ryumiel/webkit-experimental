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

#import "config.h"
#import "_WKUserContentFilterInternal.h"

#if WK_API_ENABLED

#include "WebCompiledContentExtension.h"
#include <WebCore/ContentExtensionCompiler.h>
#include <WebCore/ContentExtensionError.h>
#include <string>

@implementation _WKUserContentFilter

- (instancetype)initWithName:(NSString *)name serializedRules:(NSString *)serializedRules
{
    if (!(self = [super init]))
        return nil;

    WebCore::ContentExtensions::CompiledContentExtensionData data;
    WebKit::LegacyContentExtensionCompilationClient client(data);

    if (auto compilerError = WebCore::ContentExtensions::compileRuleList(client, String(serializedRules)))
        [NSException raise:NSGenericException format:@"Failed to compile rules with error: %s", compilerError.message().c_str()];

    auto compiledContentExtension = WebKit::WebCompiledContentExtension::createFromCompiledContentExtensionData(data);
    API::Object::constructInWrapper<API::UserContentExtension>(self, String(name), WTF::move(compiledContentExtension));

    return self;
}

- (void)dealloc
{
    _userContentExtension->~UserContentExtension();

    [super dealloc];
}

#pragma mark WKObject protocol implementation

- (API::Object&)_apiObject
{
    return *_userContentExtension;
}

@end

#endif // WK_API_ENABLED
