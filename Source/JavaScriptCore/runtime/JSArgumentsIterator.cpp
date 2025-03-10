/*
 * Copyright (C) 2013, 2015 Apple, Inc. All rights reserved.
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

#include "config.h"
#include "JSArgumentsIterator.h"

#include "ClonedArguments.h"
#include "DirectArguments.h"
#include "JSCInlines.h"
#include "ScopedArguments.h"

namespace JSC {

const ClassInfo JSArgumentsIterator::s_info = { "ArgumentsIterator", &Base::s_info, 0, CREATE_METHOD_TABLE(JSArgumentsIterator) };

void JSArgumentsIterator::finishCreation(VM& vm, JSObject* arguments)
{
    Base::finishCreation(vm);
    m_arguments.set(vm, this, arguments);
}

JSArgumentsIterator* JSArgumentsIterator::clone(ExecState* exec)
{
    auto clone = JSArgumentsIterator::create(exec->vm(), exec->callee()->globalObject()->argumentsIteratorStructure(), m_arguments.get());
    clone->m_nextIndex = m_nextIndex;
    return clone;
}

EncodedJSValue JSC_HOST_CALL argumentsFuncIterator(ExecState* exec)
{
    JSObject* thisObj = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    if (!thisObj->inherits(DirectArguments::info()) && !thisObj->inherits(ScopedArguments::info()) && !thisObj->inherits(ClonedArguments::info()))
        return JSValue::encode(throwTypeError(exec, ASCIILiteral("Attempted to use Arguments iterator on non-Arguments object")));
    return JSValue::encode(JSArgumentsIterator::create(exec->vm(), exec->callee()->globalObject()->argumentsIteratorStructure(), thisObj));
}

}
