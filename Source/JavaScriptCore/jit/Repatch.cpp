/*
 * Copyright (C) 2011-2015 Apple Inc. All rights reserved.
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
#include "Repatch.h"

#if ENABLE(JIT)

#include "AccessorCallJITStubRoutine.h"
#include "BinarySwitch.h"
#include "CCallHelpers.h"
#include "DFGOperations.h"
#include "DFGSpeculativeJIT.h"
#include "FTLThunks.h"
#include "GCAwareJITStubRoutine.h"
#include "GetterSetter.h"
#include "JIT.h"
#include "JITInlines.h"
#include "LinkBuffer.h"
#include "JSCInlines.h"
#include "PolymorphicGetByIdList.h"
#include "PolymorphicPutByIdList.h"
#include "RegExpMatchesArray.h"
#include "RepatchBuffer.h"
#include "ScratchRegisterAllocator.h"
#include "StackAlignment.h"
#include "StructureRareDataInlines.h"
#include "StructureStubClearingWatchpoint.h"
#include "ThunkGenerators.h"
#include <wtf/ListDump.h>
#include <wtf/StringPrintStream.h>

namespace JSC {

// Beware: in this code, it is not safe to assume anything about the following registers
// that would ordinarily have well-known values:
// - tagTypeNumberRegister
// - tagMaskRegister

static FunctionPtr readCallTarget(RepatchBuffer& repatchBuffer, CodeLocationCall call)
{
    FunctionPtr result = MacroAssembler::readCallTarget(call);
#if ENABLE(FTL_JIT)
    CodeBlock* codeBlock = repatchBuffer.codeBlock();
    if (codeBlock->jitType() == JITCode::FTLJIT) {
        return FunctionPtr(codeBlock->vm()->ftlThunks->keyForSlowPathCallThunk(
            MacroAssemblerCodePtr::createFromExecutableAddress(
                result.executableAddress())).callTarget());
    }
#else
    UNUSED_PARAM(repatchBuffer);
#endif // ENABLE(FTL_JIT)
    return result;
}

static void repatchCall(RepatchBuffer& repatchBuffer, CodeLocationCall call, FunctionPtr newCalleeFunction)
{
#if ENABLE(FTL_JIT)
    CodeBlock* codeBlock = repatchBuffer.codeBlock();
    if (codeBlock->jitType() == JITCode::FTLJIT) {
        VM& vm = *codeBlock->vm();
        FTL::Thunks& thunks = *vm.ftlThunks;
        FTL::SlowPathCallKey key = thunks.keyForSlowPathCallThunk(
            MacroAssemblerCodePtr::createFromExecutableAddress(
                MacroAssembler::readCallTarget(call).executableAddress()));
        key = key.withCallTarget(newCalleeFunction.executableAddress());
        newCalleeFunction = FunctionPtr(
            thunks.getSlowPathCallThunk(vm, key).code().executableAddress());
    }
#endif // ENABLE(FTL_JIT)
    repatchBuffer.relink(call, newCalleeFunction);
}

static void repatchCall(CodeBlock* codeblock, CodeLocationCall call, FunctionPtr newCalleeFunction)
{
    RepatchBuffer repatchBuffer(codeblock);
    repatchCall(repatchBuffer, call, newCalleeFunction);
}

static void repatchByIdSelfAccess(
    VM& vm, CodeBlock* codeBlock, StructureStubInfo& stubInfo, Structure* structure,
    const Identifier& propertyName, PropertyOffset offset, const FunctionPtr &slowPathFunction,
    bool compact)
{
    if (structure->typeInfo().newImpurePropertyFiresWatchpoints())
        vm.registerWatchpointForImpureProperty(propertyName, stubInfo.addWatchpoint(codeBlock));
    
    RepatchBuffer repatchBuffer(codeBlock);

    // Only optimize once!
    repatchCall(repatchBuffer, stubInfo.callReturnLocation, slowPathFunction);

    // Patch the structure check & the offset of the load.
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(-(intptr_t)stubInfo.patch.deltaCheckImmToCall), bitwise_cast<int32_t>(structure->id()));
    repatchBuffer.setLoadInstructionIsActive(stubInfo.callReturnLocation.convertibleLoadAtOffset(stubInfo.patch.deltaCallToStorageLoad), isOutOfLineOffset(offset));
#if USE(JSVALUE64)
    if (compact)
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.deltaCallToLoadOrStore), offsetRelativeToPatchedStorage(offset));
    else
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.deltaCallToLoadOrStore), offsetRelativeToPatchedStorage(offset));
#elif USE(JSVALUE32_64)
    if (compact) {
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.deltaCallToTagLoadOrStore), offsetRelativeToPatchedStorage(offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.deltaCallToPayloadLoadOrStore), offsetRelativeToPatchedStorage(offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    } else {
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.deltaCallToTagLoadOrStore), offsetRelativeToPatchedStorage(offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.deltaCallToPayloadLoadOrStore), offsetRelativeToPatchedStorage(offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    }
#endif
}

static void addStructureTransitionCheck(
    JSCell* object, Structure* structure, CodeBlock* codeBlock, StructureStubInfo& stubInfo,
    MacroAssembler& jit, MacroAssembler::JumpList& failureCases, GPRReg scratchGPR)
{
    if (object->structure() == structure && structure->transitionWatchpointSetIsStillValid()) {
        structure->addTransitionWatchpoint(stubInfo.addWatchpoint(codeBlock));
        if (!ASSERT_DISABLED) {
            // If we execute this code, the object must have the structure we expect. Assert
            // this in debug modes.
            jit.move(MacroAssembler::TrustedImmPtr(object), scratchGPR);
            MacroAssembler::Jump ok = branchStructure(
                jit,
                MacroAssembler::Equal,
                MacroAssembler::Address(scratchGPR, JSCell::structureIDOffset()),
                structure);
            jit.abortWithReason(RepatchIneffectiveWatchpoint);
            ok.link(&jit);
        }
        return;
    }
    
    jit.move(MacroAssembler::TrustedImmPtr(object), scratchGPR);
    failureCases.append(
        branchStructure(jit,
            MacroAssembler::NotEqual,
            MacroAssembler::Address(scratchGPR, JSCell::structureIDOffset()),
            structure));
}

static void addStructureTransitionCheck(
    JSValue prototype, CodeBlock* codeBlock, StructureStubInfo& stubInfo,
    MacroAssembler& jit, MacroAssembler::JumpList& failureCases, GPRReg scratchGPR)
{
    if (prototype.isNull())
        return;
    
    ASSERT(prototype.isCell());
    
    addStructureTransitionCheck(
        prototype.asCell(), prototype.asCell()->structure(), codeBlock, stubInfo, jit,
        failureCases, scratchGPR);
}

static void replaceWithJump(RepatchBuffer& repatchBuffer, StructureStubInfo& stubInfo, const MacroAssemblerCodePtr target)
{
    if (MacroAssembler::canJumpReplacePatchableBranch32WithPatch()) {
        repatchBuffer.replaceWithJump(
            RepatchBuffer::startOfPatchableBranch32WithPatchOnAddress(
                stubInfo.callReturnLocation.dataLabel32AtOffset(
                    -(intptr_t)stubInfo.patch.deltaCheckImmToCall)),
            CodeLocationLabel(target));
        return;
    }
    
    repatchBuffer.relink(
        stubInfo.callReturnLocation.jumpAtOffset(
            stubInfo.patch.deltaCallToJump),
        CodeLocationLabel(target));
}

static void emitRestoreScratch(MacroAssembler& stubJit, bool needToRestoreScratch, GPRReg scratchGPR, MacroAssembler::Jump& success, MacroAssembler::Jump& fail, MacroAssembler::JumpList failureCases)
{
    if (needToRestoreScratch) {
        stubJit.popToRestore(scratchGPR);
        
        success = stubJit.jump();
        
        // link failure cases here, so we can pop scratchGPR, and then jump back.
        failureCases.link(&stubJit);
        
        stubJit.popToRestore(scratchGPR);
        
        fail = stubJit.jump();
        return;
    }
    
    success = stubJit.jump();
}

static void linkRestoreScratch(LinkBuffer& patchBuffer, bool needToRestoreScratch, MacroAssembler::Jump success, MacroAssembler::Jump fail, MacroAssembler::JumpList failureCases, CodeLocationLabel successLabel, CodeLocationLabel slowCaseBegin)
{
    patchBuffer.link(success, successLabel);
        
    if (needToRestoreScratch) {
        patchBuffer.link(fail, slowCaseBegin);
        return;
    }
    
    // link failure cases directly back to normal path
    patchBuffer.link(failureCases, slowCaseBegin);
}

static void linkRestoreScratch(LinkBuffer& patchBuffer, bool needToRestoreScratch, StructureStubInfo& stubInfo, MacroAssembler::Jump success, MacroAssembler::Jump fail, MacroAssembler::JumpList failureCases)
{
    linkRestoreScratch(patchBuffer, needToRestoreScratch, success, fail, failureCases, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToDone), stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToSlowCase));
}

enum ByIdStubKind {
    GetValue,
    GetUndefined,
    CallGetter,
    CallCustomGetter,
    CallSetter,
    CallCustomSetter
};

static const char* toString(ByIdStubKind kind)
{
    switch (kind) {
    case GetValue:
        return "GetValue";
    case GetUndefined:
        return "GetUndefined";
    case CallGetter:
        return "CallGetter";
    case CallCustomGetter:
        return "CallCustomGetter";
    case CallSetter:
        return "CallSetter";
    case CallCustomSetter:
        return "CallCustomSetter";
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }
}

static ByIdStubKind kindFor(const PropertySlot& slot)
{
    if (slot.isCacheableValue())
        return GetValue;
    if (slot.isUnset())
        return GetUndefined;
    if (slot.isCacheableCustom())
        return CallCustomGetter;
    RELEASE_ASSERT(slot.isCacheableGetter());
    return CallGetter;
}

static FunctionPtr customFor(const PropertySlot& slot)
{
    if (!slot.isCacheableCustom())
        return FunctionPtr();
    return FunctionPtr(slot.customGetter());
}

static ByIdStubKind kindFor(const PutPropertySlot& slot)
{
    RELEASE_ASSERT(!slot.isCacheablePut());
    if (slot.isCacheableSetter())
        return CallSetter;
    RELEASE_ASSERT(slot.isCacheableCustom());
    return CallCustomSetter;
}

static FunctionPtr customFor(const PutPropertySlot& slot)
{
    if (!slot.isCacheableCustom())
        return FunctionPtr();
    return FunctionPtr(slot.customSetter());
}

static bool generateByIdStub(
    ExecState* exec, ByIdStubKind kind, const Identifier& propertyName,
    FunctionPtr custom, StructureStubInfo& stubInfo, StructureChain* chain, size_t count,
    PropertyOffset offset, Structure* structure, bool loadTargetFromProxy, WatchpointSet* watchpointSet,
    CodeLocationLabel successLabel, CodeLocationLabel slowCaseLabel, RefPtr<JITStubRoutine>& stubRoutine)
{

    VM* vm = &exec->vm();
    GPRReg baseGPR = static_cast<GPRReg>(stubInfo.patch.baseGPR);
    JSValueRegs valueRegs = JSValueRegs(
#if USE(JSVALUE32_64)
        static_cast<GPRReg>(stubInfo.patch.valueTagGPR),
#endif
        static_cast<GPRReg>(stubInfo.patch.valueGPR));
    GPRReg scratchGPR = TempRegisterSet(stubInfo.patch.usedRegisters).getFreeGPR();
    bool needToRestoreScratch = scratchGPR == InvalidGPRReg;
    RELEASE_ASSERT(!needToRestoreScratch || (kind == GetValue || kind == GetUndefined));
    
    CCallHelpers stubJit(&exec->vm(), exec->codeBlock());
    if (needToRestoreScratch) {
        scratchGPR = AssemblyHelpers::selectScratchGPR(
            baseGPR, valueRegs.tagGPR(), valueRegs.payloadGPR());
        stubJit.pushToSave(scratchGPR);
        needToRestoreScratch = true;
    }
    
    MacroAssembler::JumpList failureCases;

    GPRReg baseForGetGPR;
    if (loadTargetFromProxy) {
        baseForGetGPR = valueRegs.payloadGPR();
        failureCases.append(stubJit.branch8(
            MacroAssembler::NotEqual, 
            MacroAssembler::Address(baseGPR, JSCell::typeInfoTypeOffset()), 
            MacroAssembler::TrustedImm32(PureForwardingProxyType)));

        stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSProxy::targetOffset()), scratchGPR);
        
        failureCases.append(branchStructure(stubJit,
            MacroAssembler::NotEqual, 
            MacroAssembler::Address(scratchGPR, JSCell::structureIDOffset()),
            structure));
    } else {
        baseForGetGPR = baseGPR;

        failureCases.append(branchStructure(stubJit,
            MacroAssembler::NotEqual, 
            MacroAssembler::Address(baseForGetGPR, JSCell::structureIDOffset()), 
            structure));
    }

    CodeBlock* codeBlock = exec->codeBlock();
    if (structure->typeInfo().newImpurePropertyFiresWatchpoints())
        vm->registerWatchpointForImpureProperty(propertyName, stubInfo.addWatchpoint(codeBlock));

    if (watchpointSet)
        watchpointSet->add(stubInfo.addWatchpoint(codeBlock));

    Structure* currStructure = structure; 
    JSObject* protoObject = 0;
    if (chain) {
        WriteBarrier<Structure>* it = chain->head();
        for (unsigned i = 0; i < count; ++i, ++it) {
            protoObject = asObject(currStructure->prototypeForLookup(exec));
            Structure* protoStructure = protoObject->structure();
            if (protoStructure->typeInfo().newImpurePropertyFiresWatchpoints())
                vm->registerWatchpointForImpureProperty(propertyName, stubInfo.addWatchpoint(codeBlock));
            addStructureTransitionCheck(
                protoObject, protoStructure, codeBlock, stubInfo, stubJit,
                failureCases, scratchGPR);
            currStructure = it->get();
        }
        ASSERT(!protoObject || protoObject->structure() == currStructure);
    }
    
    currStructure->startWatchingPropertyForReplacements(*vm, offset);
    GPRReg baseForAccessGPR = InvalidGPRReg;
    if (kind != GetUndefined) {
        if (chain) {
            // We could have clobbered scratchGPR earlier, so we have to reload from baseGPR to get the target.
            if (loadTargetFromProxy)
                stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSProxy::targetOffset()), baseForGetGPR);
            stubJit.move(MacroAssembler::TrustedImmPtr(protoObject), scratchGPR);
            baseForAccessGPR = scratchGPR;
        } else {
            // For proxy objects, we need to do all the Structure checks before moving the baseGPR into
            // baseForGetGPR because if we fail any of the checks then we would have the wrong value in baseGPR
            // on the slow path.
            if (loadTargetFromProxy)
                stubJit.move(scratchGPR, baseForGetGPR);
            baseForAccessGPR = baseForGetGPR;
        }
    }

    GPRReg loadedValueGPR = InvalidGPRReg;
    if (kind == GetUndefined)
        stubJit.moveTrustedValue(jsUndefined(), valueRegs);
    else if (kind != CallCustomGetter && kind != CallCustomSetter) {
        if (kind == GetValue)
            loadedValueGPR = valueRegs.payloadGPR();
        else
            loadedValueGPR = scratchGPR;
        
        GPRReg storageGPR;
        if (isInlineOffset(offset))
            storageGPR = baseForAccessGPR;
        else {
            stubJit.loadPtr(MacroAssembler::Address(baseForAccessGPR, JSObject::butterflyOffset()), loadedValueGPR);
            storageGPR = loadedValueGPR;
        }
        
#if USE(JSVALUE64)
        stubJit.load64(MacroAssembler::Address(storageGPR, offsetRelativeToBase(offset)), loadedValueGPR);
#else
        if (kind == GetValue)
            stubJit.load32(MacroAssembler::Address(storageGPR, offsetRelativeToBase(offset) + TagOffset), valueRegs.tagGPR());
        stubJit.load32(MacroAssembler::Address(storageGPR, offsetRelativeToBase(offset) + PayloadOffset), loadedValueGPR);
#endif
    }

    // Stuff for custom getters.
    MacroAssembler::Call operationCall;
    MacroAssembler::Call handlerCall;

    // Stuff for JS getters.
    MacroAssembler::DataLabelPtr addressOfLinkFunctionCheck;
    MacroAssembler::Call fastPathCall;
    MacroAssembler::Call slowPathCall;
    std::unique_ptr<CallLinkInfo> callLinkInfo;

    MacroAssembler::Jump success, fail;
    if (kind != GetValue && kind != GetUndefined) {
        // Need to make sure that whenever this call is made in the future, we remember the
        // place that we made it from. It just so happens to be the place that we are at
        // right now!
        stubJit.store32(MacroAssembler::TrustedImm32(exec->locationAsRawBits()),
            CCallHelpers::tagFor(static_cast<VirtualRegister>(JSStack::ArgumentCount)));

        if (kind == CallGetter || kind == CallSetter) {
            // Create a JS call using a JS call inline cache. Assume that:
            //
            // - SP is aligned and represents the extent of the calling compiler's stack usage.
            //
            // - FP is set correctly (i.e. it points to the caller's call frame header).
            //
            // - SP - FP is an aligned difference.
            //
            // - Any byte between FP (exclusive) and SP (inclusive) could be live in the calling
            //   code.
            //
            // Therefore, we temporarily grow the stack for the purpose of the call and then
            // shrink it after.
            
            callLinkInfo = std::make_unique<CallLinkInfo>();
            callLinkInfo->callType = CallLinkInfo::Call;
            callLinkInfo->codeOrigin = stubInfo.codeOrigin;
            callLinkInfo->calleeGPR = loadedValueGPR;
            
            MacroAssembler::JumpList done;
            
            // There is a 'this' argument but nothing else.
            unsigned numberOfParameters = 1;
            // ... unless we're calling a setter.
            if (kind == CallSetter)
                numberOfParameters++;
            
            // Get the accessor; if there ain't one then the result is jsUndefined().
            if (kind == CallSetter) {
                stubJit.loadPtr(
                    MacroAssembler::Address(loadedValueGPR, GetterSetter::offsetOfSetter()),
                    loadedValueGPR);
            } else {
                stubJit.loadPtr(
                    MacroAssembler::Address(loadedValueGPR, GetterSetter::offsetOfGetter()),
                    loadedValueGPR);
            }
            MacroAssembler::Jump returnUndefined = stubJit.branchTestPtr(
                MacroAssembler::Zero, loadedValueGPR);
            
            unsigned numberOfRegsForCall =
                JSStack::CallFrameHeaderSize + numberOfParameters;
            
            unsigned numberOfBytesForCall =
                numberOfRegsForCall * sizeof(Register) - sizeof(CallerFrameAndPC);
            
            unsigned alignedNumberOfBytesForCall =
                WTF::roundUpToMultipleOf(stackAlignmentBytes(), numberOfBytesForCall);
            
            stubJit.subPtr(
                MacroAssembler::TrustedImm32(alignedNumberOfBytesForCall),
                MacroAssembler::stackPointerRegister);
            
            MacroAssembler::Address calleeFrame = MacroAssembler::Address(
                MacroAssembler::stackPointerRegister,
                -static_cast<ptrdiff_t>(sizeof(CallerFrameAndPC)));
            
            stubJit.store32(
                MacroAssembler::TrustedImm32(numberOfParameters),
                calleeFrame.withOffset(
                    JSStack::ArgumentCount * sizeof(Register) + PayloadOffset));
            
            stubJit.storeCell(
                loadedValueGPR, calleeFrame.withOffset(JSStack::Callee * sizeof(Register)));

            stubJit.storeCell(
                baseForGetGPR,
                calleeFrame.withOffset(
                    virtualRegisterForArgument(0).offset() * sizeof(Register)));
            
            if (kind == CallSetter) {
                stubJit.storeValue(
                    valueRegs,
                    calleeFrame.withOffset(
                        virtualRegisterForArgument(1).offset() * sizeof(Register)));
            }
            
            MacroAssembler::Jump slowCase = stubJit.branchPtrWithPatch(
                MacroAssembler::NotEqual, loadedValueGPR, addressOfLinkFunctionCheck,
                MacroAssembler::TrustedImmPtr(0));
            
            fastPathCall = stubJit.nearCall();
            
            stubJit.addPtr(
                MacroAssembler::TrustedImm32(alignedNumberOfBytesForCall),
                MacroAssembler::stackPointerRegister);
            if (kind == CallGetter)
                stubJit.setupResults(valueRegs);
            
            done.append(stubJit.jump());
            slowCase.link(&stubJit);
            
            stubJit.move(loadedValueGPR, GPRInfo::regT0);
#if USE(JSVALUE32_64)
            stubJit.move(MacroAssembler::TrustedImm32(JSValue::CellTag), GPRInfo::regT1);
#endif
            stubJit.move(MacroAssembler::TrustedImmPtr(callLinkInfo.get()), GPRInfo::regT2);
            slowPathCall = stubJit.nearCall();
            
            stubJit.addPtr(
                MacroAssembler::TrustedImm32(alignedNumberOfBytesForCall),
                MacroAssembler::stackPointerRegister);
            if (kind == CallGetter)
                stubJit.setupResults(valueRegs);
            
            done.append(stubJit.jump());
            returnUndefined.link(&stubJit);
            
            if (kind == CallGetter)
                stubJit.moveTrustedValue(jsUndefined(), valueRegs);
            
            done.link(&stubJit);
        } else {
            // getter: EncodedJSValue (*GetValueFunc)(ExecState*, JSObject* slotBase, EncodedJSValue thisValue, PropertyName);
            // setter: void (*PutValueFunc)(ExecState*, JSObject* base, EncodedJSValue thisObject, EncodedJSValue value);
#if USE(JSVALUE64)
            if (kind == CallCustomGetter)
                stubJit.setupArgumentsWithExecState(baseForAccessGPR, baseForGetGPR, MacroAssembler::TrustedImmPtr(propertyName.impl()));
            else
                stubJit.setupArgumentsWithExecState(baseForAccessGPR, baseForGetGPR, valueRegs.gpr());
#else
            if (kind == CallCustomGetter)
                stubJit.setupArgumentsWithExecState(baseForAccessGPR, baseForGetGPR, MacroAssembler::TrustedImm32(JSValue::CellTag), MacroAssembler::TrustedImmPtr(propertyName.impl()));
            else
                stubJit.setupArgumentsWithExecState(baseForAccessGPR, baseForGetGPR, MacroAssembler::TrustedImm32(JSValue::CellTag), valueRegs.payloadGPR(), valueRegs.tagGPR());
#endif
            stubJit.storePtr(GPRInfo::callFrameRegister, &vm->topCallFrame);

            operationCall = stubJit.call();
            if (kind == CallCustomGetter)
                stubJit.setupResults(valueRegs);
            MacroAssembler::Jump noException = stubJit.emitExceptionCheck(CCallHelpers::InvertedExceptionCheck);
            
            stubJit.setupArguments(CCallHelpers::TrustedImmPtr(vm), GPRInfo::callFrameRegister);
            handlerCall = stubJit.call();
            stubJit.jumpToExceptionHandler();
            
            noException.link(&stubJit);
        }
    }
    emitRestoreScratch(stubJit, needToRestoreScratch, scratchGPR, success, fail, failureCases);
    
    LinkBuffer patchBuffer(*vm, stubJit, exec->codeBlock(), JITCompilationCanFail);
    if (patchBuffer.didFailToAllocate())
        return false;
    
    linkRestoreScratch(patchBuffer, needToRestoreScratch, success, fail, failureCases, successLabel, slowCaseLabel);
    if (kind == CallCustomGetter || kind == CallCustomSetter) {
        patchBuffer.link(operationCall, custom);
        patchBuffer.link(handlerCall, lookupExceptionHandler);
    } else if (kind == CallGetter || kind == CallSetter) {
        callLinkInfo->hotPathOther = patchBuffer.locationOfNearCall(fastPathCall);
        callLinkInfo->hotPathBegin = patchBuffer.locationOf(addressOfLinkFunctionCheck);
        callLinkInfo->callReturnLocation = patchBuffer.locationOfNearCall(slowPathCall);

        ThunkGenerator generator = linkThunkGeneratorFor(
            CodeForCall, RegisterPreservationNotRequired);
        patchBuffer.link(
            slowPathCall, CodeLocationLabel(vm->getCTIStub(generator).code()));
    }
    
    MacroAssemblerCodeRef code = FINALIZE_CODE_FOR(
        exec->codeBlock(), patchBuffer,
        ("%s access stub for %s, return point %p",
            toString(kind), toCString(*exec->codeBlock()).data(),
            successLabel.executableAddress()));
    
    if (kind == CallGetter || kind == CallSetter)
        stubRoutine = adoptRef(new AccessorCallJITStubRoutine(code, *vm, WTF::move(callLinkInfo)));
    else
        stubRoutine = createJITStubRoutine(code, *vm, codeBlock->ownerExecutable(), true);
    
    return true;
}

enum InlineCacheAction {
    GiveUpOnCache,
    RetryCacheLater,
    AttemptToCache
};

static InlineCacheAction actionForCell(VM& vm, JSCell* cell)
{
    Structure* structure = cell->structure(vm);

    TypeInfo typeInfo = structure->typeInfo();
    if (typeInfo.prohibitsPropertyCaching())
        return GiveUpOnCache;

    if (structure->isUncacheableDictionary()) {
        if (structure->hasBeenFlattenedBefore())
            return GiveUpOnCache;
        // Flattening could have changed the offset, so return early for another try.
        asObject(cell)->flattenDictionaryObject(vm);
        return RetryCacheLater;
    }
    ASSERT(!structure->isUncacheableDictionary());
    
    if (typeInfo.hasImpureGetOwnPropertySlot() && !typeInfo.newImpurePropertyFiresWatchpoints())
        return GiveUpOnCache;

    return AttemptToCache;
}

static InlineCacheAction tryCacheGetByID(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    if (Options::forceICFailure())
        return GiveUpOnCache;
    
    // FIXME: Write a test that proves we need to check for recursion here just
    // like the interpreter does, then add a check for recursion.

    CodeBlock* codeBlock = exec->codeBlock();
    VM* vm = &exec->vm();

    if ((isJSArray(baseValue) || isJSString(baseValue)) && propertyName == exec->propertyNames().length) {
        GPRReg baseGPR = static_cast<GPRReg>(stubInfo.patch.baseGPR);
#if USE(JSVALUE32_64)
        GPRReg resultTagGPR = static_cast<GPRReg>(stubInfo.patch.valueTagGPR);
#endif
        GPRReg resultGPR = static_cast<GPRReg>(stubInfo.patch.valueGPR);

        MacroAssembler stubJit;

        if (isJSArray(baseValue)) {
            GPRReg scratchGPR = TempRegisterSet(stubInfo.patch.usedRegisters).getFreeGPR();
            bool needToRestoreScratch = false;

            if (scratchGPR == InvalidGPRReg) {
#if USE(JSVALUE64)
                scratchGPR = AssemblyHelpers::selectScratchGPR(baseGPR, resultGPR);
#else
                scratchGPR = AssemblyHelpers::selectScratchGPR(baseGPR, resultGPR, resultTagGPR);
#endif
                stubJit.pushToSave(scratchGPR);
                needToRestoreScratch = true;
            }

            MacroAssembler::JumpList failureCases;

            stubJit.load8(MacroAssembler::Address(baseGPR, JSCell::indexingTypeOffset()), scratchGPR);
            failureCases.append(stubJit.branchTest32(MacroAssembler::Zero, scratchGPR, MacroAssembler::TrustedImm32(IsArray)));
            failureCases.append(stubJit.branchTest32(MacroAssembler::Zero, scratchGPR, MacroAssembler::TrustedImm32(IndexingShapeMask)));

            stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR);
            stubJit.load32(MacroAssembler::Address(scratchGPR, ArrayStorage::lengthOffset()), scratchGPR);
            failureCases.append(stubJit.branch32(MacroAssembler::LessThan, scratchGPR, MacroAssembler::TrustedImm32(0)));

            stubJit.move(scratchGPR, resultGPR);
#if USE(JSVALUE64)
            stubJit.or64(AssemblyHelpers::TrustedImm64(TagTypeNumber), resultGPR);
#elif USE(JSVALUE32_64)
            stubJit.move(AssemblyHelpers::TrustedImm32(JSValue::Int32Tag), resultTagGPR);
#endif

            MacroAssembler::Jump success, fail;

            emitRestoreScratch(stubJit, needToRestoreScratch, scratchGPR, success, fail, failureCases);
            
            LinkBuffer patchBuffer(*vm, stubJit, codeBlock, JITCompilationCanFail);
            if (patchBuffer.didFailToAllocate())
                return GiveUpOnCache;

            linkRestoreScratch(patchBuffer, needToRestoreScratch, stubInfo, success, fail, failureCases);

            stubInfo.stubRoutine = FINALIZE_CODE_FOR_STUB(
                exec->codeBlock(), patchBuffer,
                ("GetById array length stub for %s, return point %p",
                    toCString(*exec->codeBlock()).data(), stubInfo.callReturnLocation.labelAtOffset(
                        stubInfo.patch.deltaCallToDone).executableAddress()));

            RepatchBuffer repatchBuffer(codeBlock);
            replaceWithJump(repatchBuffer, stubInfo, stubInfo.stubRoutine->code().code());
            repatchCall(repatchBuffer, stubInfo.callReturnLocation, operationGetById);

            return RetryCacheLater;
        }

        // String.length case
        MacroAssembler::Jump failure = stubJit.branch8(MacroAssembler::NotEqual, MacroAssembler::Address(baseGPR, JSCell::typeInfoTypeOffset()), MacroAssembler::TrustedImm32(StringType));

        stubJit.load32(MacroAssembler::Address(baseGPR, JSString::offsetOfLength()), resultGPR);

#if USE(JSVALUE64)
        stubJit.or64(AssemblyHelpers::TrustedImm64(TagTypeNumber), resultGPR);
#elif USE(JSVALUE32_64)
        stubJit.move(AssemblyHelpers::TrustedImm32(JSValue::Int32Tag), resultTagGPR);
#endif

        MacroAssembler::Jump success = stubJit.jump();

        LinkBuffer patchBuffer(*vm, stubJit, codeBlock, JITCompilationCanFail);
        if (patchBuffer.didFailToAllocate())
            return GiveUpOnCache;
        
        patchBuffer.link(success, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToDone));
        patchBuffer.link(failure, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToSlowCase));

        stubInfo.stubRoutine = FINALIZE_CODE_FOR_STUB(
            exec->codeBlock(), patchBuffer,
            ("GetById string length stub for %s, return point %p",
                toCString(*exec->codeBlock()).data(), stubInfo.callReturnLocation.labelAtOffset(
                    stubInfo.patch.deltaCallToDone).executableAddress()));

        RepatchBuffer repatchBuffer(codeBlock);
        replaceWithJump(repatchBuffer, stubInfo, stubInfo.stubRoutine->code().code());
        repatchCall(repatchBuffer, stubInfo.callReturnLocation, operationGetById);

        return RetryCacheLater;
    }

    // FIXME: Cache property access for immediates.
    if (!baseValue.isCell())
        return GiveUpOnCache;

    if (!slot.isCacheable() && !slot.isUnset())
        return GiveUpOnCache;

    JSCell* baseCell = baseValue.asCell();
    Structure* structure = baseCell->structure(*vm);

    InlineCacheAction action = actionForCell(*vm, baseCell);
    if (action != AttemptToCache)
        return action;

    // Optimize self access.
    if (slot.isCacheableValue()
        && slot.slotBase() == baseValue
        && !slot.watchpointSet()
        && MacroAssembler::isCompactPtrAlignedAddressOffset(maxOffsetRelativeToPatchedStorage(slot.cachedOffset()))) {
        structure->startWatchingPropertyForReplacements(*vm, slot.cachedOffset());
        repatchByIdSelfAccess(*vm, codeBlock, stubInfo, structure, propertyName, slot.cachedOffset(), operationGetByIdBuildList, true);
        stubInfo.initGetByIdSelf(*vm, codeBlock->ownerExecutable(), structure);
        return RetryCacheLater;
    }

    repatchCall(codeBlock, stubInfo.callReturnLocation, operationGetByIdBuildList);
    return RetryCacheLater;
}

void repatchGetByID(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    GCSafeConcurrentJITLocker locker(exec->codeBlock()->m_lock, exec->vm().heap);
    
    if (tryCacheGetByID(exec, baseValue, propertyName, slot, stubInfo) == GiveUpOnCache)
        repatchCall(exec->codeBlock(), stubInfo.callReturnLocation, operationGetById);
}

static void patchJumpToGetByIdStub(CodeBlock* codeBlock, StructureStubInfo& stubInfo, JITStubRoutine* stubRoutine)
{
    RELEASE_ASSERT(stubInfo.accessType == access_get_by_id_list);
    RepatchBuffer repatchBuffer(codeBlock);
    if (stubInfo.u.getByIdList.list->didSelfPatching()) {
        repatchBuffer.relink(
            stubInfo.callReturnLocation.jumpAtOffset(
                stubInfo.patch.deltaCallToJump),
            CodeLocationLabel(stubRoutine->code().code()));
        return;
    }
    
    replaceWithJump(repatchBuffer, stubInfo, stubRoutine->code().code());
}

static InlineCacheAction tryBuildGetByIDList(ExecState* exec, JSValue baseValue, const Identifier& ident, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    if (!baseValue.isCell()
        || (!slot.isCacheable() && !slot.isUnset()))
        return GiveUpOnCache;

    JSCell* baseCell = baseValue.asCell();
    bool loadTargetFromProxy = false;
    if (baseCell->type() == PureForwardingProxyType) {
        baseValue = jsCast<JSProxy*>(baseCell)->target();
        baseCell = baseValue.asCell();
        loadTargetFromProxy = true;
    }

    VM* vm = &exec->vm();
    CodeBlock* codeBlock = exec->codeBlock();

    InlineCacheAction action = actionForCell(*vm, baseCell);
    if (action != AttemptToCache)
        return action;

    Structure* structure = baseCell->structure(*vm);
    TypeInfo typeInfo = structure->typeInfo();

    if (stubInfo.patch.spillMode == NeedToSpill) {
        // We cannot do as much inline caching if the registers were not flushed prior to this GetById. In particular,
        // non-Value cached properties require planting calls, which requires registers to have been flushed. Thus,
        // if registers were not flushed, don't do non-Value caching.
        if (!slot.isCacheableValue() && !slot.isUnset())
            return GiveUpOnCache;
    }

    PropertyOffset offset = slot.isUnset() ? invalidOffset : slot.cachedOffset();
    StructureChain* prototypeChain = 0;
    size_t count = 0;
    
    if (slot.isUnset() || slot.slotBase() != baseValue) {
        if (typeInfo.prohibitsPropertyCaching() || structure->isDictionary())
            return GiveUpOnCache;

        if (slot.isUnset())
            count = normalizePrototypeChain(exec, structure);
        else
            count = normalizePrototypeChainForChainAccess(
                exec, structure, slot.slotBase(), ident, offset);
        if (count == InvalidPrototypeChain)
            return GiveUpOnCache;
        prototypeChain = structure->prototypeChain(exec);
    }
    
    PolymorphicGetByIdList* list = PolymorphicGetByIdList::from(stubInfo);
    if (list->isFull()) {
        // We need this extra check because of recursion.
        return GiveUpOnCache;
    }
    
    RefPtr<JITStubRoutine> stubRoutine;
    bool result = generateByIdStub(
        exec, kindFor(slot), ident, customFor(slot), stubInfo, prototypeChain, count, offset, 
        structure, loadTargetFromProxy, slot.watchpointSet(), 
        stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToDone),
        CodeLocationLabel(list->currentSlowPathTarget(stubInfo)), stubRoutine);
    if (!result)
        return GiveUpOnCache;
    
    GetByIdAccess::AccessType accessType;
    if (slot.isCacheableValue())
        accessType = slot.watchpointSet() ? GetByIdAccess::WatchedStub : GetByIdAccess::SimpleStub;
    else if (slot.isUnset())
        accessType = GetByIdAccess::SimpleMiss;
    else if (slot.isCacheableGetter())
        accessType = GetByIdAccess::Getter;
    else
        accessType = GetByIdAccess::CustomGetter;
    
    list->addAccess(GetByIdAccess(
        *vm, codeBlock->ownerExecutable(), accessType, stubRoutine, structure,
        prototypeChain, count));
    
    patchJumpToGetByIdStub(codeBlock, stubInfo, stubRoutine.get());
    
    return list->isFull() ? GiveUpOnCache : RetryCacheLater;
}

void buildGetByIDList(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    GCSafeConcurrentJITLocker locker(exec->codeBlock()->m_lock, exec->vm().heap);
    
    if (tryBuildGetByIDList(exec, baseValue, propertyName, slot, stubInfo) == GiveUpOnCache)
        repatchCall(exec->codeBlock(), stubInfo.callReturnLocation, operationGetById);
}

static V_JITOperation_ESsiJJI appropriateGenericPutByIdFunction(const PutPropertySlot &slot, PutKind putKind)
{
    if (slot.isStrictMode()) {
        if (putKind == Direct)
            return operationPutByIdDirectStrict;
        return operationPutByIdStrict;
    }
    if (putKind == Direct)
        return operationPutByIdDirectNonStrict;
    return operationPutByIdNonStrict;
}

static V_JITOperation_ESsiJJI appropriateListBuildingPutByIdFunction(const PutPropertySlot &slot, PutKind putKind)
{
    if (slot.isStrictMode()) {
        if (putKind == Direct)
            return operationPutByIdDirectStrictBuildList;
        return operationPutByIdStrictBuildList;
    }
    if (putKind == Direct)
        return operationPutByIdDirectNonStrictBuildList;
    return operationPutByIdNonStrictBuildList;
}

static bool emitPutReplaceStub(
    ExecState* exec,
    const Identifier&,
    const PutPropertySlot& slot,
    StructureStubInfo& stubInfo,
    Structure* structure,
    CodeLocationLabel failureLabel,
    RefPtr<JITStubRoutine>& stubRoutine)
{
    VM* vm = &exec->vm();
    GPRReg baseGPR = static_cast<GPRReg>(stubInfo.patch.baseGPR);
#if USE(JSVALUE32_64)
    GPRReg valueTagGPR = static_cast<GPRReg>(stubInfo.patch.valueTagGPR);
#endif
    GPRReg valueGPR = static_cast<GPRReg>(stubInfo.patch.valueGPR);

    ScratchRegisterAllocator allocator(stubInfo.patch.usedRegisters);
    allocator.lock(baseGPR);
#if USE(JSVALUE32_64)
    allocator.lock(valueTagGPR);
#endif
    allocator.lock(valueGPR);
    
    GPRReg scratchGPR1 = allocator.allocateScratchGPR();

    CCallHelpers stubJit(vm, exec->codeBlock());

    allocator.preserveReusedRegistersByPushing(stubJit);

    MacroAssembler::Jump badStructure = branchStructure(stubJit,
        MacroAssembler::NotEqual,
        MacroAssembler::Address(baseGPR, JSCell::structureIDOffset()),
        structure);

#if USE(JSVALUE64)
    if (isInlineOffset(slot.cachedOffset()))
        stubJit.store64(valueGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue)));
    else {
        stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR1);
        stubJit.store64(valueGPR, MacroAssembler::Address(scratchGPR1, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue)));
    }
#elif USE(JSVALUE32_64)
    if (isInlineOffset(slot.cachedOffset())) {
        stubJit.store32(valueGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        stubJit.store32(valueTagGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    } else {
        stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR1);
        stubJit.store32(valueGPR, MacroAssembler::Address(scratchGPR1, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        stubJit.store32(valueTagGPR, MacroAssembler::Address(scratchGPR1, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    }
#endif
    
    MacroAssembler::Jump success;
    MacroAssembler::Jump failure;
    
    if (allocator.didReuseRegisters()) {
        allocator.restoreReusedRegistersByPopping(stubJit);
        success = stubJit.jump();
        
        badStructure.link(&stubJit);
        allocator.restoreReusedRegistersByPopping(stubJit);
        failure = stubJit.jump();
    } else {
        success = stubJit.jump();
        failure = badStructure;
    }
    
    LinkBuffer patchBuffer(*vm, stubJit, exec->codeBlock(), JITCompilationCanFail);
    if (patchBuffer.didFailToAllocate())
        return false;
    
    patchBuffer.link(success, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToDone));
    patchBuffer.link(failure, failureLabel);
            
    stubRoutine = FINALIZE_CODE_FOR_STUB(
        exec->codeBlock(), patchBuffer,
        ("PutById replace stub for %s, return point %p",
            toCString(*exec->codeBlock()).data(), stubInfo.callReturnLocation.labelAtOffset(
                stubInfo.patch.deltaCallToDone).executableAddress()));
    
    return true;
}

static Structure* emitPutTransitionStubAndGetOldStructure(ExecState* exec, VM* vm, Structure*& structure, const Identifier& ident, 
    const PutPropertySlot& slot, StructureStubInfo& stubInfo, PutKind putKind)
{
    PropertyName pname(ident);
    Structure* oldStructure = structure;
    if (!oldStructure->isObject() || oldStructure->isDictionary() || pname.asIndex() != PropertyName::NotAnIndex)
        return nullptr;

    PropertyOffset propertyOffset;
    structure = Structure::addPropertyTransitionToExistingStructureConcurrently(oldStructure, ident.impl(), 0, propertyOffset);

    if (!structure || !structure->isObject() || structure->isDictionary() || !structure->propertyAccessesAreCacheable())
        return nullptr;

    // Skip optimizing the case where we need a realloc, if we don't have
    // enough registers to make it happen.
    if (GPRInfo::numberOfRegisters < 6
        && oldStructure->outOfLineCapacity() != structure->outOfLineCapacity()
        && oldStructure->outOfLineCapacity()) {
        return nullptr;
    }

    // Skip optimizing the case where we need realloc, and the structure has
    // indexing storage.
    // FIXME: We shouldn't skip this! Implement it!
    // https://bugs.webkit.org/show_bug.cgi?id=130914
    if (oldStructure->couldHaveIndexingHeader())
        return nullptr;

    if (normalizePrototypeChain(exec, structure) == InvalidPrototypeChain)
        return nullptr;

    StructureChain* prototypeChain = structure->prototypeChain(exec);

    // emitPutTransitionStub

    CodeLocationLabel failureLabel = stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToSlowCase);
    RefPtr<JITStubRoutine>& stubRoutine = stubInfo.stubRoutine;

    GPRReg baseGPR = static_cast<GPRReg>(stubInfo.patch.baseGPR);
#if USE(JSVALUE32_64)
    GPRReg valueTagGPR = static_cast<GPRReg>(stubInfo.patch.valueTagGPR);
#endif
    GPRReg valueGPR = static_cast<GPRReg>(stubInfo.patch.valueGPR);
    
    ScratchRegisterAllocator allocator(stubInfo.patch.usedRegisters);
    allocator.lock(baseGPR);
#if USE(JSVALUE32_64)
    allocator.lock(valueTagGPR);
#endif
    allocator.lock(valueGPR);
    
    CCallHelpers stubJit(vm);
    
    bool needThirdScratch = false;
    if (structure->outOfLineCapacity() != oldStructure->outOfLineCapacity()
        && oldStructure->outOfLineCapacity()) {
        needThirdScratch = true;
    }

    GPRReg scratchGPR1 = allocator.allocateScratchGPR();
    ASSERT(scratchGPR1 != baseGPR);
    ASSERT(scratchGPR1 != valueGPR);
    
    GPRReg scratchGPR2 = allocator.allocateScratchGPR();
    ASSERT(scratchGPR2 != baseGPR);
    ASSERT(scratchGPR2 != valueGPR);
    ASSERT(scratchGPR2 != scratchGPR1);

    GPRReg scratchGPR3;
    if (needThirdScratch) {
        scratchGPR3 = allocator.allocateScratchGPR();
        ASSERT(scratchGPR3 != baseGPR);
        ASSERT(scratchGPR3 != valueGPR);
        ASSERT(scratchGPR3 != scratchGPR1);
        ASSERT(scratchGPR3 != scratchGPR2);
    } else
        scratchGPR3 = InvalidGPRReg;
    
    allocator.preserveReusedRegistersByPushing(stubJit);

    MacroAssembler::JumpList failureCases;
            
    ASSERT(oldStructure->transitionWatchpointSetHasBeenInvalidated());
    
    failureCases.append(branchStructure(stubJit,
        MacroAssembler::NotEqual, 
        MacroAssembler::Address(baseGPR, JSCell::structureIDOffset()), 
        oldStructure));
    
    addStructureTransitionCheck(
        oldStructure->storedPrototype(), exec->codeBlock(), stubInfo, stubJit, failureCases,
        scratchGPR1);
            
    if (putKind == NotDirect) {
        for (WriteBarrier<Structure>* it = prototypeChain->head(); *it; ++it) {
            addStructureTransitionCheck(
                (*it)->storedPrototype(), exec->codeBlock(), stubInfo, stubJit, failureCases,
                scratchGPR1);
        }
    }

    MacroAssembler::JumpList slowPath;
    
    bool scratchGPR1HasStorage = false;
    
    if (structure->outOfLineCapacity() != oldStructure->outOfLineCapacity()) {
        size_t newSize = structure->outOfLineCapacity() * sizeof(JSValue);
        CopiedAllocator* copiedAllocator = &vm->heap.storageAllocator();
        
        if (!oldStructure->outOfLineCapacity()) {
            stubJit.loadPtr(&copiedAllocator->m_currentRemaining, scratchGPR1);
            slowPath.append(stubJit.branchSubPtr(MacroAssembler::Signed, MacroAssembler::TrustedImm32(newSize), scratchGPR1));
            stubJit.storePtr(scratchGPR1, &copiedAllocator->m_currentRemaining);
            stubJit.negPtr(scratchGPR1);
            stubJit.addPtr(MacroAssembler::AbsoluteAddress(&copiedAllocator->m_currentPayloadEnd), scratchGPR1);
            stubJit.addPtr(MacroAssembler::TrustedImm32(sizeof(JSValue)), scratchGPR1);
        } else {
            size_t oldSize = oldStructure->outOfLineCapacity() * sizeof(JSValue);
            ASSERT(newSize > oldSize);
            
            stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR3);
            stubJit.loadPtr(&copiedAllocator->m_currentRemaining, scratchGPR1);
            slowPath.append(stubJit.branchSubPtr(MacroAssembler::Signed, MacroAssembler::TrustedImm32(newSize), scratchGPR1));
            stubJit.storePtr(scratchGPR1, &copiedAllocator->m_currentRemaining);
            stubJit.negPtr(scratchGPR1);
            stubJit.addPtr(MacroAssembler::AbsoluteAddress(&copiedAllocator->m_currentPayloadEnd), scratchGPR1);
            stubJit.addPtr(MacroAssembler::TrustedImm32(sizeof(JSValue)), scratchGPR1);
            // We have scratchGPR1 = new storage, scratchGPR3 = old storage, scratchGPR2 = available
            for (size_t offset = 0; offset < oldSize; offset += sizeof(void*)) {
                stubJit.loadPtr(MacroAssembler::Address(scratchGPR3, -static_cast<ptrdiff_t>(offset + sizeof(JSValue) + sizeof(void*))), scratchGPR2);
                stubJit.storePtr(scratchGPR2, MacroAssembler::Address(scratchGPR1, -static_cast<ptrdiff_t>(offset + sizeof(JSValue) + sizeof(void*))));
            }
        }
        
        stubJit.storePtr(scratchGPR1, MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()));
        scratchGPR1HasStorage = true;
    }

    ASSERT(oldStructure->typeInfo().type() == structure->typeInfo().type());
    ASSERT(oldStructure->typeInfo().inlineTypeFlags() == structure->typeInfo().inlineTypeFlags());
    ASSERT(oldStructure->indexingType() == structure->indexingType());
#if USE(JSVALUE64)
    uint32_t val = structure->id();
#else
    uint32_t val = reinterpret_cast<uint32_t>(structure->id());
#endif
    stubJit.store32(MacroAssembler::TrustedImm32(val), MacroAssembler::Address(baseGPR, JSCell::structureIDOffset()));
#if USE(JSVALUE64)
    if (isInlineOffset(slot.cachedOffset()))
        stubJit.store64(valueGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue)));
    else {
        if (!scratchGPR1HasStorage)
            stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR1);
        stubJit.store64(valueGPR, MacroAssembler::Address(scratchGPR1, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue)));
    }
#elif USE(JSVALUE32_64)
    if (isInlineOffset(slot.cachedOffset())) {
        stubJit.store32(valueGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        stubJit.store32(valueTagGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    } else {
        if (!scratchGPR1HasStorage)
            stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR1);
        stubJit.store32(valueGPR, MacroAssembler::Address(scratchGPR1, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        stubJit.store32(valueTagGPR, MacroAssembler::Address(scratchGPR1, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    }
#endif
    
    ScratchBuffer* scratchBuffer = nullptr;

#if ENABLE(GGC)
    MacroAssembler::Call callFlushWriteBarrierBuffer;
    MacroAssembler::Jump ownerIsRememberedOrInEden = stubJit.jumpIfIsRememberedOrInEden(baseGPR);
    {
        WriteBarrierBuffer* writeBarrierBuffer = &stubJit.vm()->heap.writeBarrierBuffer();
        stubJit.move(MacroAssembler::TrustedImmPtr(writeBarrierBuffer), scratchGPR1);
        stubJit.load32(MacroAssembler::Address(scratchGPR1, WriteBarrierBuffer::currentIndexOffset()), scratchGPR2);
        MacroAssembler::Jump needToFlush =
            stubJit.branch32(MacroAssembler::AboveOrEqual, scratchGPR2, MacroAssembler::Address(scratchGPR1, WriteBarrierBuffer::capacityOffset()));

        stubJit.add32(MacroAssembler::TrustedImm32(1), scratchGPR2);
        stubJit.store32(scratchGPR2, MacroAssembler::Address(scratchGPR1, WriteBarrierBuffer::currentIndexOffset()));

        stubJit.loadPtr(MacroAssembler::Address(scratchGPR1, WriteBarrierBuffer::bufferOffset()), scratchGPR1);
        // We use an offset of -sizeof(void*) because we already added 1 to scratchGPR2.
        stubJit.storePtr(baseGPR, MacroAssembler::BaseIndex(scratchGPR1, scratchGPR2, MacroAssembler::ScalePtr, static_cast<int32_t>(-sizeof(void*))));

        MacroAssembler::Jump doneWithBarrier = stubJit.jump();
        needToFlush.link(&stubJit);

        scratchBuffer = vm->scratchBufferForSize(allocator.desiredScratchBufferSizeForCall());
        allocator.preserveUsedRegistersToScratchBufferForCall(stubJit, scratchBuffer, scratchGPR2);
        stubJit.setupArgumentsWithExecState(baseGPR);
        callFlushWriteBarrierBuffer = stubJit.call();
        allocator.restoreUsedRegistersFromScratchBufferForCall(stubJit, scratchBuffer, scratchGPR2);

        doneWithBarrier.link(&stubJit);
    }
    ownerIsRememberedOrInEden.link(&stubJit);
#endif

    MacroAssembler::Jump success;
    MacroAssembler::Jump failure;
            
    if (allocator.didReuseRegisters()) {
        allocator.restoreReusedRegistersByPopping(stubJit);
        success = stubJit.jump();

        failureCases.link(&stubJit);
        allocator.restoreReusedRegistersByPopping(stubJit);
        failure = stubJit.jump();
    } else
        success = stubJit.jump();
    
    MacroAssembler::Call operationCall;
    MacroAssembler::Jump successInSlowPath;
    
    if (structure->outOfLineCapacity() != oldStructure->outOfLineCapacity()) {
        slowPath.link(&stubJit);
        
        allocator.restoreReusedRegistersByPopping(stubJit);
        if (!scratchBuffer)
            scratchBuffer = vm->scratchBufferForSize(allocator.desiredScratchBufferSizeForCall());
        allocator.preserveUsedRegistersToScratchBufferForCall(stubJit, scratchBuffer, scratchGPR1);
#if USE(JSVALUE64)
        stubJit.setupArgumentsWithExecState(baseGPR, MacroAssembler::TrustedImmPtr(structure), MacroAssembler::TrustedImm32(slot.cachedOffset()), valueGPR);
#else
        stubJit.setupArgumentsWithExecState(baseGPR, MacroAssembler::TrustedImmPtr(structure), MacroAssembler::TrustedImm32(slot.cachedOffset()), valueGPR, valueTagGPR);
#endif
        operationCall = stubJit.call();
        allocator.restoreUsedRegistersFromScratchBufferForCall(stubJit, scratchBuffer, scratchGPR1);
        successInSlowPath = stubJit.jump();
    }
    
    LinkBuffer patchBuffer(*vm, stubJit, exec->codeBlock(), JITCompilationCanFail);
    if (patchBuffer.didFailToAllocate())
        return nullptr;
    
    patchBuffer.link(success, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToDone));
    if (allocator.didReuseRegisters())
        patchBuffer.link(failure, failureLabel);
    else
        patchBuffer.link(failureCases, failureLabel);
#if ENABLE(GGC)
    patchBuffer.link(callFlushWriteBarrierBuffer, operationFlushWriteBarrierBuffer);
#endif
    if (structure->outOfLineCapacity() != oldStructure->outOfLineCapacity()) {
        patchBuffer.link(operationCall, operationReallocateStorageAndFinishPut);
        patchBuffer.link(successInSlowPath, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToDone));
    }
    
    stubRoutine =
        createJITStubRoutine(
            FINALIZE_CODE_FOR(
                exec->codeBlock(), patchBuffer,
                ("PutById %stransition stub (%p -> %p) for %s, return point %p",
                    structure->outOfLineCapacity() != oldStructure->outOfLineCapacity() ? "reallocating " : "",
                    oldStructure, structure,
                    toCString(*exec->codeBlock()).data(), stubInfo.callReturnLocation.labelAtOffset(
                        stubInfo.patch.deltaCallToDone).executableAddress())),
            *vm,
            exec->codeBlock()->ownerExecutable(),
            structure->outOfLineCapacity() != oldStructure->outOfLineCapacity(),
            structure);

    return oldStructure;
}

static InlineCacheAction tryCachePutByID(ExecState* exec, JSValue baseValue, Structure* structure, const Identifier& ident, const PutPropertySlot& slot, StructureStubInfo& stubInfo, PutKind putKind)
{
    if (Options::forceICFailure())
        return GiveUpOnCache;
    
    CodeBlock* codeBlock = exec->codeBlock();
    VM* vm = &exec->vm();

    if (!baseValue.isCell())
        return GiveUpOnCache;
    
    if (!slot.isCacheablePut() && !slot.isCacheableCustom() && !slot.isCacheableSetter())
        return GiveUpOnCache;

    if (!structure->propertyAccessesAreCacheable())
        return GiveUpOnCache;

    // Optimize self access.
    if (slot.base() == baseValue && slot.isCacheablePut()) {
        if (slot.type() == PutPropertySlot::NewProperty) {

            Structure* oldStructure = emitPutTransitionStubAndGetOldStructure(exec, vm, structure, ident, slot, stubInfo, putKind);
            if (!oldStructure)
                return GiveUpOnCache;
            
            StructureChain* prototypeChain = structure->prototypeChain(exec);
            
            RepatchBuffer repatchBuffer(codeBlock);
            repatchBuffer.relink(
                stubInfo.callReturnLocation.jumpAtOffset(
                    stubInfo.patch.deltaCallToJump),
                CodeLocationLabel(stubInfo.stubRoutine->code().code()));
            repatchCall(repatchBuffer, stubInfo.callReturnLocation, appropriateListBuildingPutByIdFunction(slot, putKind));
            
            stubInfo.initPutByIdTransition(*vm, codeBlock->ownerExecutable(), oldStructure, structure, prototypeChain, putKind == Direct);
            
            return RetryCacheLater;
        }

        if (!MacroAssembler::isPtrAlignedAddressOffset(offsetRelativeToPatchedStorage(slot.cachedOffset())))
            return GiveUpOnCache;

        structure->didCachePropertyReplacement(*vm, slot.cachedOffset());
        repatchByIdSelfAccess(*vm, codeBlock, stubInfo, structure, ident, slot.cachedOffset(), appropriateListBuildingPutByIdFunction(slot, putKind), false);
        stubInfo.initPutByIdReplace(*vm, codeBlock->ownerExecutable(), structure);
        return RetryCacheLater;
    }

    if ((slot.isCacheableCustom() || slot.isCacheableSetter())
        && stubInfo.patch.spillMode == DontSpill) {
        RefPtr<JITStubRoutine> stubRoutine;

        StructureChain* prototypeChain = 0;
        PropertyOffset offset = slot.cachedOffset();
        size_t count = 0;
        if (baseValue != slot.base()) {
            count = normalizePrototypeChainForChainAccess(exec, structure, slot.base(), ident, offset);
            if (count == InvalidPrototypeChain)
                return GiveUpOnCache;
            prototypeChain = structure->prototypeChain(exec);
        }
        PolymorphicPutByIdList* list;
        list = PolymorphicPutByIdList::from(putKind, stubInfo);

        bool result = generateByIdStub(
            exec, kindFor(slot), ident, customFor(slot), stubInfo, prototypeChain, count,
            offset, structure, false, nullptr,
            stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToDone),
            stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToSlowCase),
            stubRoutine);
        if (!result)
            return GiveUpOnCache;
        
        list->addAccess(PutByIdAccess::setter(
            *vm, codeBlock->ownerExecutable(),
            slot.isCacheableSetter() ? PutByIdAccess::Setter : PutByIdAccess::CustomSetter,
            structure, prototypeChain, count, slot.customSetter(), stubRoutine));

        RepatchBuffer repatchBuffer(codeBlock);
        repatchBuffer.relink(stubInfo.callReturnLocation.jumpAtOffset(stubInfo.patch.deltaCallToJump), CodeLocationLabel(stubRoutine->code().code()));
        repatchCall(repatchBuffer, stubInfo.callReturnLocation, appropriateListBuildingPutByIdFunction(slot, putKind));
        RELEASE_ASSERT(!list->isFull());
        return RetryCacheLater;
    }

    return GiveUpOnCache;
}

void repatchPutByID(ExecState* exec, JSValue baseValue, Structure* structure, const Identifier& propertyName, const PutPropertySlot& slot, StructureStubInfo& stubInfo, PutKind putKind)
{
    GCSafeConcurrentJITLocker locker(exec->codeBlock()->m_lock, exec->vm().heap);
    
    if (tryCachePutByID(exec, baseValue, structure, propertyName, slot, stubInfo, putKind) == GiveUpOnCache)
        repatchCall(exec->codeBlock(), stubInfo.callReturnLocation, appropriateGenericPutByIdFunction(slot, putKind));
}

static InlineCacheAction tryBuildPutByIdList(ExecState* exec, JSValue baseValue, Structure* structure, const Identifier& propertyName, const PutPropertySlot& slot, StructureStubInfo& stubInfo, PutKind putKind)
{
    CodeBlock* codeBlock = exec->codeBlock();
    VM* vm = &exec->vm();

    if (!baseValue.isCell())
        return GiveUpOnCache;

    if (!slot.isCacheablePut() && !slot.isCacheableCustom() && !slot.isCacheableSetter())
        return GiveUpOnCache;

    if (!structure->propertyAccessesAreCacheable())
        return GiveUpOnCache;

    // Optimize self access.
    if (slot.base() == baseValue && slot.isCacheablePut()) {
        PolymorphicPutByIdList* list;
        RefPtr<JITStubRoutine> stubRoutine;
        
        if (slot.type() == PutPropertySlot::NewProperty) {
            list = PolymorphicPutByIdList::from(putKind, stubInfo);
            if (list->isFull())
                return GiveUpOnCache; // Will get here due to recursion.

            Structure* oldStructure = emitPutTransitionStubAndGetOldStructure(exec, vm, structure, propertyName, slot, stubInfo, putKind);

            if (!oldStructure) 
                return GiveUpOnCache;

            StructureChain* prototypeChain = structure->prototypeChain(exec);
            stubRoutine = stubInfo.stubRoutine;
            list->addAccess(
                PutByIdAccess::transition(
                    *vm, codeBlock->ownerExecutable(),
                    oldStructure, structure, prototypeChain,
                    stubRoutine));

        } else {
            list = PolymorphicPutByIdList::from(putKind, stubInfo);
            if (list->isFull())
                return GiveUpOnCache; // Will get here due to recursion.
            
            structure->didCachePropertyReplacement(*vm, slot.cachedOffset());
            
            // We're now committed to creating the stub. Mogrify the meta-data accordingly.
            bool result = emitPutReplaceStub(
                exec, propertyName, slot, stubInfo, 
                structure, CodeLocationLabel(list->currentSlowPathTarget()), stubRoutine);
            if (!result)
                return GiveUpOnCache;
            
            list->addAccess(
                PutByIdAccess::replace(
                    *vm, codeBlock->ownerExecutable(),
                    structure, stubRoutine));
        }
        RepatchBuffer repatchBuffer(codeBlock);
        repatchBuffer.relink(stubInfo.callReturnLocation.jumpAtOffset(stubInfo.patch.deltaCallToJump), CodeLocationLabel(stubRoutine->code().code()));
        if (list->isFull())
            repatchCall(repatchBuffer, stubInfo.callReturnLocation, appropriateGenericPutByIdFunction(slot, putKind));

        return RetryCacheLater;
    }

    if ((slot.isCacheableCustom() || slot.isCacheableSetter())
        && stubInfo.patch.spillMode == DontSpill) {
        RefPtr<JITStubRoutine> stubRoutine;
        StructureChain* prototypeChain = 0;
        PropertyOffset offset = slot.cachedOffset();
        size_t count = 0;
        if (baseValue != slot.base()) {
            count = normalizePrototypeChainForChainAccess(exec, structure, slot.base(), propertyName, offset);
            if (count == InvalidPrototypeChain)
                return GiveUpOnCache;
            prototypeChain = structure->prototypeChain(exec);
        }
        
        PolymorphicPutByIdList* list;
        list = PolymorphicPutByIdList::from(putKind, stubInfo);

        bool result = generateByIdStub(
            exec, kindFor(slot), propertyName, customFor(slot), stubInfo, prototypeChain, count,
            offset, structure, false, nullptr,
            stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToDone),
            CodeLocationLabel(list->currentSlowPathTarget()),
            stubRoutine);
        if (!result)
            return GiveUpOnCache;
        
        list->addAccess(PutByIdAccess::setter(
            *vm, codeBlock->ownerExecutable(),
            slot.isCacheableSetter() ? PutByIdAccess::Setter : PutByIdAccess::CustomSetter,
            structure, prototypeChain, count, slot.customSetter(), stubRoutine));

        RepatchBuffer repatchBuffer(codeBlock);
        repatchBuffer.relink(stubInfo.callReturnLocation.jumpAtOffset(stubInfo.patch.deltaCallToJump), CodeLocationLabel(stubRoutine->code().code()));
        if (list->isFull())
            repatchCall(repatchBuffer, stubInfo.callReturnLocation, appropriateGenericPutByIdFunction(slot, putKind));

        return RetryCacheLater;
    }
    return GiveUpOnCache;
}

void buildPutByIdList(ExecState* exec, JSValue baseValue, Structure* structure, const Identifier& propertyName, const PutPropertySlot& slot, StructureStubInfo& stubInfo, PutKind putKind)
{
    GCSafeConcurrentJITLocker locker(exec->codeBlock()->m_lock, exec->vm().heap);
    
    if (tryBuildPutByIdList(exec, baseValue, structure, propertyName, slot, stubInfo, putKind) == GiveUpOnCache)
        repatchCall(exec->codeBlock(), stubInfo.callReturnLocation, appropriateGenericPutByIdFunction(slot, putKind));
}

static InlineCacheAction tryRepatchIn(
    ExecState* exec, JSCell* base, const Identifier& ident, bool wasFound,
    const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    if (Options::forceICFailure())
        return GiveUpOnCache;
    
    if (!base->structure()->propertyAccessesAreCacheable())
        return GiveUpOnCache;
    
    if (wasFound) {
        if (!slot.isCacheable())
            return GiveUpOnCache;
    }
    
    CodeBlock* codeBlock = exec->codeBlock();
    VM* vm = &exec->vm();
    Structure* structure = base->structure(*vm);
    
    PropertyOffset offsetIgnored;
    JSValue foundSlotBase = wasFound ? slot.slotBase() : JSValue();
    size_t count = !foundSlotBase || foundSlotBase != base ? 
        normalizePrototypeChainForChainAccess(exec, structure, foundSlotBase, ident, offsetIgnored) : 0;
    if (count == InvalidPrototypeChain)
        return GiveUpOnCache;
    
    PolymorphicAccessStructureList* polymorphicStructureList;
    int listIndex;
    
    CodeLocationLabel successLabel = stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToDone);
    CodeLocationLabel slowCaseLabel;
    
    if (stubInfo.accessType == access_unset) {
        polymorphicStructureList = new PolymorphicAccessStructureList();
        stubInfo.initInList(polymorphicStructureList, 0);
        slowCaseLabel = stubInfo.callReturnLocation.labelAtOffset(
            stubInfo.patch.deltaCallToSlowCase);
        listIndex = 0;
    } else {
        RELEASE_ASSERT(stubInfo.accessType == access_in_list);
        polymorphicStructureList = stubInfo.u.inList.structureList;
        listIndex = stubInfo.u.inList.listSize;
        slowCaseLabel = CodeLocationLabel(polymorphicStructureList->list[listIndex - 1].stubRoutine->code().code());
        
        if (listIndex == POLYMORPHIC_LIST_CACHE_SIZE)
            return GiveUpOnCache;
    }
    
    StructureChain* chain = structure->prototypeChain(exec);
    RefPtr<JITStubRoutine> stubRoutine;
    
    {
        GPRReg baseGPR = static_cast<GPRReg>(stubInfo.patch.baseGPR);
        GPRReg resultGPR = static_cast<GPRReg>(stubInfo.patch.valueGPR);
        GPRReg scratchGPR = TempRegisterSet(stubInfo.patch.usedRegisters).getFreeGPR();
        
        CCallHelpers stubJit(vm);
        
        bool needToRestoreScratch;
        if (scratchGPR == InvalidGPRReg) {
            scratchGPR = AssemblyHelpers::selectScratchGPR(baseGPR, resultGPR);
            stubJit.pushToSave(scratchGPR);
            needToRestoreScratch = true;
        } else
            needToRestoreScratch = false;
        
        MacroAssembler::JumpList failureCases;
        failureCases.append(branchStructure(stubJit,
            MacroAssembler::NotEqual,
            MacroAssembler::Address(baseGPR, JSCell::structureIDOffset()),
            structure));

        CodeBlock* codeBlock = exec->codeBlock();
        if (structure->typeInfo().newImpurePropertyFiresWatchpoints())
            vm->registerWatchpointForImpureProperty(ident, stubInfo.addWatchpoint(codeBlock));

        if (slot.watchpointSet())
            slot.watchpointSet()->add(stubInfo.addWatchpoint(codeBlock));

        Structure* currStructure = structure;
        WriteBarrier<Structure>* it = chain->head();
        for (unsigned i = 0; i < count; ++i, ++it) {
            JSObject* prototype = asObject(currStructure->prototypeForLookup(exec));
            Structure* protoStructure = prototype->structure();
            addStructureTransitionCheck(
                prototype, protoStructure, exec->codeBlock(), stubInfo, stubJit,
                failureCases, scratchGPR);
            if (protoStructure->typeInfo().newImpurePropertyFiresWatchpoints())
                vm->registerWatchpointForImpureProperty(ident, stubInfo.addWatchpoint(codeBlock));
            currStructure = it->get();
        }
        
#if USE(JSVALUE64)
        stubJit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(wasFound))), resultGPR);
#else
        stubJit.move(MacroAssembler::TrustedImm32(wasFound), resultGPR);
#endif
        
        MacroAssembler::Jump success, fail;
        
        emitRestoreScratch(stubJit, needToRestoreScratch, scratchGPR, success, fail, failureCases);
        
        LinkBuffer patchBuffer(*vm, stubJit, exec->codeBlock(), JITCompilationCanFail);
        if (patchBuffer.didFailToAllocate())
            return GiveUpOnCache;
        
        linkRestoreScratch(patchBuffer, needToRestoreScratch, success, fail, failureCases, successLabel, slowCaseLabel);
        
        stubRoutine = FINALIZE_CODE_FOR_STUB(
            exec->codeBlock(), patchBuffer,
            ("In (found = %s) stub for %s, return point %p",
                wasFound ? "yes" : "no", toCString(*exec->codeBlock()).data(),
                successLabel.executableAddress()));
    }
    
    polymorphicStructureList->list[listIndex].set(*vm, codeBlock->ownerExecutable(), stubRoutine, structure, true);
    stubInfo.u.inList.listSize++;
    
    RepatchBuffer repatchBuffer(codeBlock);
    repatchBuffer.relink(stubInfo.callReturnLocation.jumpAtOffset(stubInfo.patch.deltaCallToJump), CodeLocationLabel(stubRoutine->code().code()));
    
    return listIndex < (POLYMORPHIC_LIST_CACHE_SIZE - 1) ? RetryCacheLater : GiveUpOnCache;
}

void repatchIn(
    ExecState* exec, JSCell* base, const Identifier& ident, bool wasFound,
    const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    if (tryRepatchIn(exec, base, ident, wasFound, slot, stubInfo) == GiveUpOnCache)
        repatchCall(exec->codeBlock(), stubInfo.callReturnLocation, operationIn);
}

static void linkSlowFor(
    RepatchBuffer& repatchBuffer, VM* vm, CallLinkInfo& callLinkInfo, ThunkGenerator generator)
{
    repatchBuffer.relink(
        callLinkInfo.callReturnLocation, vm->getCTIStub(generator).code());
}

static void linkSlowFor(
    RepatchBuffer& repatchBuffer, VM* vm, CallLinkInfo& callLinkInfo,
    CodeSpecializationKind kind, RegisterPreservationMode registers)
{
    linkSlowFor(repatchBuffer, vm, callLinkInfo, virtualThunkGeneratorFor(kind, registers));
}

void linkFor(
    ExecState* exec, CallLinkInfo& callLinkInfo, CodeBlock* calleeCodeBlock,
    JSFunction* callee, MacroAssemblerCodePtr codePtr, CodeSpecializationKind kind,
    RegisterPreservationMode registers)
{
    ASSERT(!callLinkInfo.stub);
    
    CodeBlock* callerCodeBlock = exec->callerFrame()->codeBlock();

    VM* vm = callerCodeBlock->vm();
    
    RepatchBuffer repatchBuffer(callerCodeBlock);
    
    ASSERT(!callLinkInfo.isLinked());
    callLinkInfo.callee.set(exec->callerFrame()->vm(), callLinkInfo.hotPathBegin, callerCodeBlock->ownerExecutable(), callee);
    callLinkInfo.lastSeenCallee.set(exec->callerFrame()->vm(), callerCodeBlock->ownerExecutable(), callee);
    if (shouldShowDisassemblyFor(callerCodeBlock))
        dataLog("Linking call in ", *callerCodeBlock, " at ", callLinkInfo.codeOrigin, " to ", pointerDump(calleeCodeBlock), ", entrypoint at ", codePtr, "\n");
    repatchBuffer.relink(callLinkInfo.hotPathOther, codePtr);
    
    if (calleeCodeBlock)
        calleeCodeBlock->linkIncomingCall(exec->callerFrame(), &callLinkInfo);
    
    if (kind == CodeForCall) {
        linkSlowFor(
            repatchBuffer, vm, callLinkInfo, linkPolymorphicCallThunkGeneratorFor(registers));
        return;
    }
    
    ASSERT(kind == CodeForConstruct);
    linkSlowFor(repatchBuffer, vm, callLinkInfo, CodeForConstruct, registers);
}

void linkSlowFor(
    ExecState* exec, CallLinkInfo& callLinkInfo, CodeSpecializationKind kind,
    RegisterPreservationMode registers)
{
    CodeBlock* callerCodeBlock = exec->callerFrame()->codeBlock();
    VM* vm = callerCodeBlock->vm();
    
    RepatchBuffer repatchBuffer(callerCodeBlock);
    
    linkSlowFor(repatchBuffer, vm, callLinkInfo, kind, registers);
}

static void revertCall(
    RepatchBuffer& repatchBuffer, VM* vm, CallLinkInfo& callLinkInfo, ThunkGenerator generator)
{
    repatchBuffer.revertJumpReplacementToBranchPtrWithPatch(
        RepatchBuffer::startOfBranchPtrWithPatchOnRegister(callLinkInfo.hotPathBegin),
        static_cast<MacroAssembler::RegisterID>(callLinkInfo.calleeGPR), 0);
    linkSlowFor(repatchBuffer, vm, callLinkInfo, generator);
    callLinkInfo.hasSeenShouldRepatch = false;
    callLinkInfo.callee.clear();
    callLinkInfo.stub.clear();
    if (callLinkInfo.isOnList())
        callLinkInfo.remove();
}

void unlinkFor(
    RepatchBuffer& repatchBuffer, CallLinkInfo& callLinkInfo,
    CodeSpecializationKind kind, RegisterPreservationMode registers)
{
    if (Options::showDisassembly())
        dataLog("Unlinking call from ", callLinkInfo.callReturnLocation, " in request from ", pointerDump(repatchBuffer.codeBlock()), "\n");
    
    revertCall(
        repatchBuffer, repatchBuffer.codeBlock()->vm(), callLinkInfo,
        linkThunkGeneratorFor(kind, registers));
}

void linkVirtualFor(
    ExecState* exec, CallLinkInfo& callLinkInfo,
    CodeSpecializationKind kind, RegisterPreservationMode registers)
{
    // FIXME: We could generate a virtual call stub here. This would lead to faster virtual calls
    // by eliminating the branch prediction bottleneck inside the shared virtual call thunk.
    
    CodeBlock* callerCodeBlock = exec->callerFrame()->codeBlock();
    VM* vm = callerCodeBlock->vm();
    
    if (shouldShowDisassemblyFor(callerCodeBlock))
        dataLog("Linking virtual call at ", *callerCodeBlock, " ", exec->callerFrame()->codeOrigin(), "\n");
    
    RepatchBuffer repatchBuffer(callerCodeBlock);
    revertCall(repatchBuffer, vm, callLinkInfo, virtualThunkGeneratorFor(kind, registers));
}

namespace {
struct CallToCodePtr {
    CCallHelpers::Call call;
    MacroAssemblerCodePtr codePtr;
};
} // annonymous namespace

void linkPolymorphicCall(
    ExecState* exec, CallLinkInfo& callLinkInfo, CallVariant newVariant,
    RegisterPreservationMode registers)
{
    // Currently we can't do anything for non-function callees.
    // https://bugs.webkit.org/show_bug.cgi?id=140685
    if (!newVariant || !newVariant.executable()) {
        linkVirtualFor(exec, callLinkInfo, CodeForCall, registers);
        return;
    }
    
    CodeBlock* callerCodeBlock = exec->callerFrame()->codeBlock();
    VM* vm = callerCodeBlock->vm();
    
    CallVariantList list;
    if (PolymorphicCallStubRoutine* stub = callLinkInfo.stub.get())
        list = stub->variants();
    else if (JSFunction* oldCallee = callLinkInfo.callee.get())
        list = CallVariantList{ CallVariant(oldCallee) };
    
    list = variantListWithVariant(list, newVariant);

    // If there are any closure calls then it makes sense to treat all of them as closure calls.
    // This makes switching on callee cheaper. It also produces profiling that's easier on the DFG;
    // the DFG doesn't really want to deal with a combination of closure and non-closure callees.
    bool isClosureCall = false;
    for (CallVariant variant : list)  {
        if (variant.isClosureCall()) {
            list = despecifiedVariantList(list);
            isClosureCall = true;
            break;
        }
    }
    
    Vector<PolymorphicCallCase> callCases;
    
    // Figure out what our cases are.
    for (CallVariant variant : list) {
        CodeBlock* codeBlock;
        if (variant.executable()->isHostFunction())
            codeBlock = nullptr;
        else {
            codeBlock = jsCast<FunctionExecutable*>(variant.executable())->codeBlockForCall();
            
            // If we cannot handle a callee, assume that it's better for this whole thing to be a
            // virtual call.
            if (exec->argumentCountIncludingThis() < static_cast<size_t>(codeBlock->numParameters()) || callLinkInfo.callType == CallLinkInfo::CallVarargs || callLinkInfo.callType == CallLinkInfo::ConstructVarargs) {
                linkVirtualFor(exec, callLinkInfo, CodeForCall, registers);
                return;
            }
        }
        
        callCases.append(PolymorphicCallCase(variant, codeBlock));
    }
    
    // If we are over the limit, just use a normal virtual call.
    unsigned maxPolymorphicCallVariantListSize;
    if (callerCodeBlock->jitType() == JITCode::topTierJIT())
        maxPolymorphicCallVariantListSize = Options::maxPolymorphicCallVariantListSizeForTopTier();
    else
        maxPolymorphicCallVariantListSize = Options::maxPolymorphicCallVariantListSize();
    if (list.size() > maxPolymorphicCallVariantListSize) {
        linkVirtualFor(exec, callLinkInfo, CodeForCall, registers);
        return;
    }
    
    GPRReg calleeGPR = static_cast<GPRReg>(callLinkInfo.calleeGPR);
    
    CCallHelpers stubJit(vm, callerCodeBlock);
    
    CCallHelpers::JumpList slowPath;
    
    ptrdiff_t offsetToFrame = -sizeof(CallerFrameAndPC);

    if (!ASSERT_DISABLED) {
        CCallHelpers::Jump okArgumentCount = stubJit.branch32(
            CCallHelpers::Below, CCallHelpers::Address(CCallHelpers::stackPointerRegister, static_cast<ptrdiff_t>(sizeof(Register) * JSStack::ArgumentCount) + offsetToFrame + PayloadOffset), CCallHelpers::TrustedImm32(10000000));
        stubJit.abortWithReason(RepatchInsaneArgumentCount);
        okArgumentCount.link(&stubJit);
    }
    
    GPRReg scratch = AssemblyHelpers::selectScratchGPR(calleeGPR);
    GPRReg comparisonValueGPR;
    
    if (isClosureCall) {
        // Verify that we have a function and stash the executable in scratch.

#if USE(JSVALUE64)
        // We can safely clobber everything except the calleeGPR. We can't rely on tagMaskRegister
        // being set. So we do this the hard way.
        stubJit.move(MacroAssembler::TrustedImm64(TagMask), scratch);
        slowPath.append(stubJit.branchTest64(CCallHelpers::NonZero, calleeGPR, scratch));
#else
        // We would have already checked that the callee is a cell.
#endif
    
        slowPath.append(
            stubJit.branch8(
                CCallHelpers::NotEqual,
                CCallHelpers::Address(calleeGPR, JSCell::typeInfoTypeOffset()),
                CCallHelpers::TrustedImm32(JSFunctionType)));
    
        stubJit.loadPtr(
            CCallHelpers::Address(calleeGPR, JSFunction::offsetOfExecutable()),
            scratch);
        
        comparisonValueGPR = scratch;
    } else
        comparisonValueGPR = calleeGPR;
    
    Vector<int64_t> caseValues(callCases.size());
    Vector<CallToCodePtr> calls(callCases.size());
    std::unique_ptr<uint32_t[]> fastCounts;
    
    if (callerCodeBlock->jitType() != JITCode::topTierJIT())
        fastCounts = std::make_unique<uint32_t[]>(callCases.size());
    
    for (size_t i = callCases.size(); i--;) {
        if (fastCounts)
            fastCounts[i] = 0;
        
        CallVariant variant = callCases[i].variant();
        if (isClosureCall)
            caseValues[i] = bitwise_cast<intptr_t>(variant.executable());
        else
            caseValues[i] = bitwise_cast<intptr_t>(variant.function());
    }
    
    GPRReg fastCountsBaseGPR =
        AssemblyHelpers::selectScratchGPR(calleeGPR, comparisonValueGPR, GPRInfo::regT3);
    stubJit.move(CCallHelpers::TrustedImmPtr(fastCounts.get()), fastCountsBaseGPR);
    
    BinarySwitch binarySwitch(comparisonValueGPR, caseValues, BinarySwitch::IntPtr);
    CCallHelpers::JumpList done;
    while (binarySwitch.advance(stubJit)) {
        size_t caseIndex = binarySwitch.caseIndex();
        
        CallVariant variant = callCases[caseIndex].variant();
        
        ASSERT(variant.executable()->hasJITCodeForCall());
        MacroAssemblerCodePtr codePtr =
            variant.executable()->generatedJITCodeForCall()->addressForCall(
                *vm, variant.executable(), ArityCheckNotRequired, registers);
        
        if (fastCounts) {
            stubJit.add32(
                CCallHelpers::TrustedImm32(1),
                CCallHelpers::Address(fastCountsBaseGPR, caseIndex * sizeof(uint32_t)));
        }
        calls[caseIndex].call = stubJit.nearCall();
        calls[caseIndex].codePtr = codePtr;
        done.append(stubJit.jump());
    }
    
    slowPath.link(&stubJit);
    binarySwitch.fallThrough().link(&stubJit);
    stubJit.move(calleeGPR, GPRInfo::regT0);
#if USE(JSVALUE32_64)
    stubJit.move(CCallHelpers::TrustedImm32(JSValue::CellTag), GPRInfo::regT1);
#endif
    stubJit.move(CCallHelpers::TrustedImmPtr(&callLinkInfo), GPRInfo::regT2);
    stubJit.move(CCallHelpers::TrustedImmPtr(callLinkInfo.callReturnLocation.executableAddress()), GPRInfo::regT4);
    
    stubJit.restoreReturnAddressBeforeReturn(GPRInfo::regT4);
    AssemblyHelpers::Jump slow = stubJit.jump();
        
    LinkBuffer patchBuffer(*vm, stubJit, callerCodeBlock, JITCompilationCanFail);
    if (patchBuffer.didFailToAllocate()) {
        linkVirtualFor(exec, callLinkInfo, CodeForCall, registers);
        return;
    }
    
    RELEASE_ASSERT(callCases.size() == calls.size());
    for (CallToCodePtr callToCodePtr : calls) {
        patchBuffer.link(
            callToCodePtr.call, FunctionPtr(callToCodePtr.codePtr.executableAddress()));
    }
    if (JITCode::isOptimizingJIT(callerCodeBlock->jitType()))
        patchBuffer.link(done, callLinkInfo.callReturnLocation.labelAtOffset(0));
    else
        patchBuffer.link(done, callLinkInfo.hotPathOther.labelAtOffset(0));
    patchBuffer.link(slow, CodeLocationLabel(vm->getCTIStub(linkPolymorphicCallThunkGeneratorFor(registers)).code()));
    
    RefPtr<PolymorphicCallStubRoutine> stubRoutine = adoptRef(new PolymorphicCallStubRoutine(
        FINALIZE_CODE_FOR(
            callerCodeBlock, patchBuffer,
            ("Polymorphic call stub for %s, return point %p, targets %s",
                toCString(*callerCodeBlock).data(), callLinkInfo.callReturnLocation.labelAtOffset(0).executableAddress(),
                toCString(listDump(callCases)).data())),
        *vm, callerCodeBlock->ownerExecutable(), exec->callerFrame(), callLinkInfo, callCases,
        WTF::move(fastCounts)));
    
    RepatchBuffer repatchBuffer(callerCodeBlock);
    
    repatchBuffer.replaceWithJump(
        RepatchBuffer::startOfBranchPtrWithPatchOnRegister(callLinkInfo.hotPathBegin),
        CodeLocationLabel(stubRoutine->code().code()));
    // This is weird. The original slow path should no longer be reachable.
    linkSlowFor(repatchBuffer, vm, callLinkInfo, CodeForCall, registers);
    
    // If there had been a previous stub routine, that one will die as soon as the GC runs and sees
    // that it's no longer on stack.
    callLinkInfo.stub = stubRoutine.release();
    
    // The call link info no longer has a call cache apart from the jump to the polymorphic call
    // stub.
    if (callLinkInfo.isOnList())
        callLinkInfo.remove();
}

void resetGetByID(RepatchBuffer& repatchBuffer, StructureStubInfo& stubInfo)
{
    repatchCall(repatchBuffer, stubInfo.callReturnLocation, operationGetByIdOptimize);
    CodeLocationDataLabel32 structureLabel = stubInfo.callReturnLocation.dataLabel32AtOffset(-(intptr_t)stubInfo.patch.deltaCheckImmToCall);
    if (MacroAssembler::canJumpReplacePatchableBranch32WithPatch()) {
        repatchBuffer.revertJumpReplacementToPatchableBranch32WithPatch(
            RepatchBuffer::startOfPatchableBranch32WithPatchOnAddress(structureLabel),
            MacroAssembler::Address(
                static_cast<MacroAssembler::RegisterID>(stubInfo.patch.baseGPR),
                JSCell::structureIDOffset()),
            static_cast<int32_t>(unusedPointer));
    }
    repatchBuffer.repatch(structureLabel, static_cast<int32_t>(unusedPointer));
#if USE(JSVALUE64)
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.deltaCallToLoadOrStore), 0);
#else
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.deltaCallToTagLoadOrStore), 0);
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.deltaCallToPayloadLoadOrStore), 0);
#endif
    repatchBuffer.relink(stubInfo.callReturnLocation.jumpAtOffset(stubInfo.patch.deltaCallToJump), stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToSlowCase));
}

void resetPutByID(RepatchBuffer& repatchBuffer, StructureStubInfo& stubInfo)
{
    V_JITOperation_ESsiJJI unoptimizedFunction = bitwise_cast<V_JITOperation_ESsiJJI>(readCallTarget(repatchBuffer, stubInfo.callReturnLocation).executableAddress());
    V_JITOperation_ESsiJJI optimizedFunction;
    if (unoptimizedFunction == operationPutByIdStrict || unoptimizedFunction == operationPutByIdStrictBuildList)
        optimizedFunction = operationPutByIdStrictOptimize;
    else if (unoptimizedFunction == operationPutByIdNonStrict || unoptimizedFunction == operationPutByIdNonStrictBuildList)
        optimizedFunction = operationPutByIdNonStrictOptimize;
    else if (unoptimizedFunction == operationPutByIdDirectStrict || unoptimizedFunction == operationPutByIdDirectStrictBuildList)
        optimizedFunction = operationPutByIdDirectStrictOptimize;
    else {
        ASSERT(unoptimizedFunction == operationPutByIdDirectNonStrict || unoptimizedFunction == operationPutByIdDirectNonStrictBuildList);
        optimizedFunction = operationPutByIdDirectNonStrictOptimize;
    }
    repatchCall(repatchBuffer, stubInfo.callReturnLocation, optimizedFunction);
    CodeLocationDataLabel32 structureLabel = stubInfo.callReturnLocation.dataLabel32AtOffset(-(intptr_t)stubInfo.patch.deltaCheckImmToCall);
    if (MacroAssembler::canJumpReplacePatchableBranch32WithPatch()) {
        repatchBuffer.revertJumpReplacementToPatchableBranch32WithPatch(
            RepatchBuffer::startOfPatchableBranch32WithPatchOnAddress(structureLabel),
            MacroAssembler::Address(
                static_cast<MacroAssembler::RegisterID>(stubInfo.patch.baseGPR),
                JSCell::structureIDOffset()),
            static_cast<int32_t>(unusedPointer));
    }
    repatchBuffer.repatch(structureLabel, static_cast<int32_t>(unusedPointer));
#if USE(JSVALUE64)
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.deltaCallToLoadOrStore), 0);
#else
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.deltaCallToTagLoadOrStore), 0);
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.deltaCallToPayloadLoadOrStore), 0);
#endif
    repatchBuffer.relink(stubInfo.callReturnLocation.jumpAtOffset(stubInfo.patch.deltaCallToJump), stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToSlowCase));
}

void resetIn(RepatchBuffer& repatchBuffer, StructureStubInfo& stubInfo)
{
    repatchBuffer.relink(stubInfo.callReturnLocation.jumpAtOffset(stubInfo.patch.deltaCallToJump), stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToSlowCase));
}

} // namespace JSC

#endif
