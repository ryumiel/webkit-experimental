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

#ifndef ClonedArguments_h
#define ClonedArguments_h

#include "ArgumentsMode.h"
#include "JSObject.h"

namespace JSC {

// This is an Arguments-class object that we create when you do function.arguments, or you say
// "arguments" inside a function in strict mode. It behaves almpst entirely like an ordinary
// JavaScript object. All of the arguments values are simply copied from the stack (possibly via
// some sophisticated ValueRecovery's if an optimizing compiler is in play) and the appropriate
// properties of the object are populated. The only reason why we need a special class is to make
// the object claim to be "Arguments" from a toString standpoint, and to avoid materializing the
// caller/callee properties unless someone asks for them.
class ClonedArguments : public JSNonFinalObject {
public:
    typedef JSNonFinalObject Base;
    
private:
    ClonedArguments(VM&, Structure*);

public:
    static ClonedArguments* createEmpty(VM&, Structure*, JSFunction* callee);
    static ClonedArguments* createEmpty(ExecState*, JSFunction* callee);
    static ClonedArguments* createWithInlineFrame(ExecState* myFrame, ExecState* targetFrame, InlineCallFrame*, ArgumentsMode);
    static ClonedArguments* createWithMachineFrame(ExecState* myFrame, ExecState* targetFrame, ArgumentsMode);
    static ClonedArguments* createByCopyingFrom(ExecState*, Structure*, Register* argumentsStart, unsigned length, JSFunction* callee);
    
    static Structure* createStructure(VM&, JSGlobalObject*, JSValue prototype);

    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesGetPropertyNames | JSObject::StructureFlags;

    DECLARE_INFO;

private:
    static bool getOwnPropertySlot(JSObject*, ExecState*, PropertyName, PropertySlot&);
    static void getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
    static bool deleteProperty(JSCell*, ExecState*, PropertyName);
    static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, const PropertyDescriptor&, bool shouldThrow);
    
    bool specialsMaterialized() const { return !m_callee; }
    void materializeSpecials(ExecState*);
    void materializeSpecialsIfNecessary(ExecState*);
    
    WriteBarrier<JSFunction> m_callee; // Set to nullptr when we materialize all of our special properties.
};

} // namespace JSC

#endif // ClonedArguments_h

