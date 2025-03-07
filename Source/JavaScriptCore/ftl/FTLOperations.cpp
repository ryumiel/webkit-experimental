/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
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
#include "FTLOperations.h"

#if ENABLE(FTL_JIT)

#include "ClonedArguments.h"
#include "DirectArguments.h"
#include "JSCInlines.h"

namespace JSC { namespace FTL {

using namespace JSC::DFG;

extern "C" JSCell* JIT_OPERATION operationNewObjectWithButterfly(ExecState* exec, Structure* structure)
{
    VM& vm = exec->vm();
    NativeCallFrameTracer tracer(&vm, exec);
    
    Butterfly* butterfly = Butterfly::create(
        vm, nullptr, 0, structure->outOfLineCapacity(), false, IndexingHeader(), 0);
    
    return JSFinalObject::create(exec, structure, butterfly);
}

extern "C" JSCell* JIT_OPERATION operationMaterializeObjectInOSR(
    ExecState* exec, ExitTimeObjectMaterialization* materialization, EncodedJSValue* values)
{
    VM& vm = exec->vm();
    CodeBlock* codeBlock = exec->codeBlock();

    // We cannot GC. We've got pointers in evil places.
    DeferGCForAWhile deferGC(vm.heap);
    
    switch (materialization->type()) {
    case PhantomNewObject: {
        // First figure out what the structure is.
        Structure* structure = nullptr;
        for (unsigned i = materialization->properties().size(); i--;) {
            const ExitPropertyValue& property = materialization->properties()[i];
            if (property.location() != PromotedLocationDescriptor(StructurePLoc))
                continue;
        
            structure = jsCast<Structure*>(JSValue::decode(values[i]));
            break;
        }
        RELEASE_ASSERT(structure);
    
        // Let's create that object!
        JSFinalObject* result = JSFinalObject::create(vm, structure);
    
        // Now figure out what the heck to populate the object with. Use getPropertiesConcurrently()
        // because that happens to be lower-level and more convenient. It doesn't change the
        // materialization of the property table. We want to have minimal visible effects on the
        // system. Also, don't mind that this is O(n^2). It doesn't matter. We only get here from OSR
        // exit.
        for (PropertyMapEntry entry : structure->getPropertiesConcurrently()) {
            for (unsigned i = materialization->properties().size(); i--;) {
                const ExitPropertyValue& property = materialization->properties()[i];
                if (property.location().kind() != NamedPropertyPLoc)
                    continue;
                if (codeBlock->identifier(property.location().info()).impl() != entry.key)
                    continue;
            
                result->putDirect(vm, entry.offset, JSValue::decode(values[i]));
            }
        }
    
        return result;
    }
        
    case PhantomDirectArguments:
    case PhantomClonedArguments: {
        if (!materialization->origin().inlineCallFrame) {
            switch (materialization->type()) {
            case PhantomDirectArguments:
                return DirectArguments::createByCopying(exec);
            case PhantomClonedArguments:
                return ClonedArguments::createWithMachineFrame(exec, exec, ArgumentsMode::Cloned);
            default:
                RELEASE_ASSERT_NOT_REACHED();
                return nullptr;
            }
        }

        // First figure out the argument count. If there isn't one then we represent the machine frame.
        unsigned argumentCount = 0;
        if (materialization->origin().inlineCallFrame->isVarargs()) {
            for (unsigned i = materialization->properties().size(); i--;) {
                const ExitPropertyValue& property = materialization->properties()[i];
                if (property.location() != PromotedLocationDescriptor(ArgumentCountPLoc))
                    continue;
                
                argumentCount = JSValue::decode(values[i]).asUInt32();
                RELEASE_ASSERT(argumentCount);
                break;
            }
            RELEASE_ASSERT(argumentCount);
        } else
            argumentCount = materialization->origin().inlineCallFrame->arguments.size();
        
        JSFunction* callee = nullptr;
        if (materialization->origin().inlineCallFrame->isClosureCall) {
            for (unsigned i = materialization->properties().size(); i--;) {
                const ExitPropertyValue& property = materialization->properties()[i];
                if (property.location() != PromotedLocationDescriptor(ArgumentsCalleePLoc))
                    continue;
                
                callee = jsCast<JSFunction*>(JSValue::decode(values[i]));
                break;
            }
        } else
            callee = materialization->origin().inlineCallFrame->calleeConstant();
        RELEASE_ASSERT(callee);
        
        CodeBlock* codeBlock = baselineCodeBlockForOriginAndBaselineCodeBlock(
            materialization->origin(), exec->codeBlock());
        
        // We have an inline frame and we have all of the data we need to recreate it.
        switch (materialization->type()) {
        case PhantomDirectArguments: {
            unsigned length = argumentCount - 1;
            unsigned capacity = std::max(length, static_cast<unsigned>(codeBlock->numParameters() - 1));
            DirectArguments* result = DirectArguments::create(
                vm, codeBlock->globalObject()->directArgumentsStructure(), length, capacity);
            result->callee().set(vm, result, callee);
            for (unsigned i = materialization->properties().size(); i--;) {
                const ExitPropertyValue& property = materialization->properties()[i];
                if (property.location().kind() != ArgumentPLoc)
                    continue;
                
                unsigned index = property.location().info();
                if (index >= capacity)
                    continue;
                result->setIndexQuickly(vm, index, JSValue::decode(values[i]));
            }
            return result;
        }
        case PhantomClonedArguments: {
            unsigned length = argumentCount - 1;
            ClonedArguments* result = ClonedArguments::createEmpty(
                vm, codeBlock->globalObject()->outOfBandArgumentsStructure(), callee);
            
            for (unsigned i = materialization->properties().size(); i--;) {
                const ExitPropertyValue& property = materialization->properties()[i];
                if (property.location().kind() != ArgumentPLoc)
                    continue;
                
                unsigned index = property.location().info();
                if (index >= length)
                    continue;
                result->putDirectIndex(exec, index, JSValue::decode(values[i]));
            }
            
            result->putDirect(vm, vm.propertyNames->length, jsNumber(length));
            return result;
        }
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return nullptr;
        }
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }
}

} } // namespace JSC::FTL

#endif // ENABLE(FTL_JIT)

