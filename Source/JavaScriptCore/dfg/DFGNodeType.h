/*
 * Copyright (C) 2012-2015 Apple Inc. All rights reserved.
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

#ifndef DFGNodeType_h
#define DFGNodeType_h

#if ENABLE(DFG_JIT)

#include "DFGNodeFlags.h"

namespace JSC { namespace DFG {

// This macro defines a set of information about all known node types, used to populate NodeId, NodeType below.
#define FOR_EACH_DFG_OP(macro) \
    /* A constant in the CodeBlock's constant pool. */\
    macro(JSConstant, NodeResultJS) \
    \
    /* Constants with specific representations. */\
    macro(DoubleConstant, NodeResultDouble) \
    macro(Int52Constant, NodeResultInt52) \
    \
    /* Marker to indicate that an operation was optimized entirely and all that is left */\
    /* is to make one node alias another. CSE will later usually eliminate this node, */\
    /* though it may choose not to if it would corrupt predictions (very rare). */\
    macro(Identity, NodeResultJS) \
    \
    /* Nodes for handling functions (both as call and as construct). */\
    macro(ToThis, NodeResultJS) \
    macro(CreateThis, NodeResultJS) /* Note this is not MustGenerate since we're returning it anyway. */ \
    macro(GetCallee, NodeResultJS) \
    macro(GetArgumentCount, NodeResultInt32) \
    \
    /* Nodes for local variable access. These nodes are linked together using Phi nodes. */\
    /* Any two nodes that are part of the same Phi graph will share the same */\
    /* VariableAccessData, and thus will share predictions. FIXME: We should come up with */\
    /* better names for a lot of these. https://bugs.webkit.org/show_bug.cgi?id=137307 */\
    macro(GetLocal, NodeResultJS) \
    macro(SetLocal, 0) \
    \
    macro(PutStack, NodeMustGenerate) \
    macro(KillStack, NodeMustGenerate) \
    macro(GetStack, NodeResultJS) \
    \
    macro(MovHint, 0) \
    macro(ZombieHint, 0) \
    macro(Phantom, NodeMustGenerate) \
    macro(HardPhantom, NodeMustGenerate) /* Like Phantom, but we never remove any of its children. */ \
    macro(Check, NodeMustGenerate) /* Used if we want just a type check but not liveness. Non-checking uses will be removed. */\
    macro(Upsilon, NodeRelevantToOSR) \
    macro(Phi, NodeRelevantToOSR) \
    macro(Flush, NodeMustGenerate) \
    macro(PhantomLocal, NodeMustGenerate) \
    \
    /* Hint that this is where bytecode thinks is a good place to OSR. Note that this */\
    /* will exist even in inlined loops. This has no execution semantics but it must */\
    /* survive all DCE. We treat this as being a can-exit because tier-up to FTL may */\
    /* want all state. */\
    macro(LoopHint, NodeMustGenerate) \
    \
    /* Special node for OSR entry into the FTL. Indicates that we're loading a local */\
    /* variable from the scratch buffer. */\
    macro(ExtractOSREntryLocal, NodeResultJS) \
    \
    /* Tier-up checks from the DFG to the FTL. */\
    macro(CheckTierUpInLoop, NodeMustGenerate) \
    macro(CheckTierUpAndOSREnter, NodeMustGenerate) \
    macro(CheckTierUpAtReturn, NodeMustGenerate) \
    \
    /* Get the value of a local variable, without linking into the VariableAccessData */\
    /* network. This is only valid for variable accesses whose predictions originated */\
    /* as something other than a local access, and thus had their own profiling. */\
    macro(GetLocalUnlinked, NodeResultJS) \
    \
    /* Marker for an argument being set at the prologue of a function. */\
    macro(SetArgument, 0) \
    \
    /* Marker of location in the IR where we may possibly perform jump replacement to */\
    /* invalidate this code block. */\
    macro(InvalidationPoint, NodeMustGenerate) \
    \
    /* Nodes for bitwise operations. */\
    macro(BitAnd, NodeResultInt32) \
    macro(BitOr, NodeResultInt32) \
    macro(BitXor, NodeResultInt32) \
    macro(BitLShift, NodeResultInt32) \
    macro(BitRShift, NodeResultInt32) \
    macro(BitURShift, NodeResultInt32) \
    /* Bitwise operators call ToInt32 on their operands. */\
    macro(ValueToInt32, NodeResultInt32) \
    /* Used to box the result of URShift nodes (result has range 0..2^32-1). */\
    macro(UInt32ToNumber, NodeResultNumber) \
    /* Converts booleans to numbers but passes everything else through. */\
    macro(BooleanToNumber, NodeResultJS) \
    \
    /* Attempt to truncate a double to int32; this will exit if it can't do it. */\
    macro(DoubleAsInt32, NodeResultInt32) \
    \
    /* Change the representation of a value. */\
    macro(DoubleRep, NodeResultDouble) \
    macro(Int52Rep, NodeResultInt52) \
    macro(ValueRep, NodeResultJS) \
    \
    /* Bogus type asserting node. Useful for testing, disappears during Fixup. */\
    macro(FiatInt52, NodeResultJS) \
    \
    /* Nodes for arithmetic operations. */\
    macro(ArithAdd, NodeResultNumber) \
    macro(ArithSub, NodeResultNumber) \
    macro(ArithNegate, NodeResultNumber) \
    macro(ArithMul, NodeResultNumber) \
    macro(ArithIMul, NodeResultInt32) \
    macro(ArithDiv, NodeResultNumber) \
    macro(ArithMod, NodeResultNumber) \
    macro(ArithAbs, NodeResultNumber) \
    macro(ArithMin, NodeResultNumber) \
    macro(ArithMax, NodeResultNumber) \
    macro(ArithFRound, NodeResultNumber) \
    macro(ArithPow, NodeResultNumber) \
    macro(ArithSqrt, NodeResultNumber) \
    macro(ArithSin, NodeResultNumber) \
    macro(ArithCos, NodeResultNumber) \
    macro(ArithLog, NodeResultNumber) \
    \
    /* Add of values may either be arithmetic, or result in string concatenation. */\
    macro(ValueAdd, NodeResultJS | NodeMustGenerate) \
    \
    /* Property access. */\
    /* PutByValAlias indicates a 'put' aliases a prior write to the same property. */\
    /* Since a put to 'length' may invalidate optimizations here, */\
    /* this must be the directly subsequent property put. Note that PutByVal */\
    /* opcodes use VarArgs beause they may have up to 4 children. */\
    macro(GetByVal, NodeResultJS | NodeMustGenerate) \
    macro(GetMyArgumentByVal, NodeResultJS | NodeMustGenerate) \
    macro(LoadVarargs, NodeMustGenerate) \
    macro(ForwardVarargs, NodeMustGenerate) \
    macro(PutByValDirect, NodeMustGenerate | NodeHasVarArgs) \
    macro(PutByVal, NodeMustGenerate | NodeHasVarArgs) \
    macro(PutByValAlias, NodeMustGenerate | NodeHasVarArgs) \
    macro(GetById, NodeResultJS | NodeMustGenerate) \
    macro(GetByIdFlush, NodeResultJS | NodeMustGenerate) \
    macro(PutById, NodeMustGenerate) \
    macro(PutByIdFlush, NodeMustGenerate | NodeMustGenerate) \
    macro(PutByIdDirect, NodeMustGenerate) \
    macro(CheckStructure, NodeMustGenerate) \
    macro(GetExecutable, NodeResultJS) \
    macro(PutStructure, NodeMustGenerate) \
    macro(AllocatePropertyStorage, NodeMustGenerate | NodeResultStorage) \
    macro(ReallocatePropertyStorage, NodeMustGenerate | NodeResultStorage) \
    macro(GetButterfly, NodeResultStorage) \
    macro(CheckArray, NodeMustGenerate) \
    macro(Arrayify, NodeMustGenerate) \
    macro(ArrayifyToStructure, NodeMustGenerate) \
    macro(GetIndexedPropertyStorage, NodeResultStorage) \
    macro(ConstantStoragePointer, NodeResultStorage) \
    macro(TypedArrayWatchpoint, NodeMustGenerate) \
    macro(GetGetter, NodeResultJS) \
    macro(GetSetter, NodeResultJS) \
    macro(GetByOffset, NodeResultJS) \
    macro(GetGetterSetterByOffset, NodeResultJS) \
    macro(MultiGetByOffset, NodeResultJS | NodeMustGenerate) \
    macro(PutByOffset, NodeMustGenerate) \
    macro(MultiPutByOffset, NodeMustGenerate) \
    macro(GetArrayLength, NodeResultInt32) \
    macro(GetTypedArrayByteOffset, NodeResultInt32) \
    macro(GetScope, NodeResultJS) \
    macro(SkipScope, NodeResultJS) \
    macro(GetClosureVar, NodeResultJS) \
    macro(PutClosureVar, NodeMustGenerate) \
    macro(GetGlobalVar, NodeResultJS) \
    macro(PutGlobalVar, NodeMustGenerate) \
    macro(NotifyWrite, NodeMustGenerate) \
    macro(VarInjectionWatchpoint, NodeMustGenerate) \
    macro(CheckCell, NodeMustGenerate) \
    macro(CheckNotEmpty, NodeMustGenerate) \
    macro(CheckBadCell, NodeMustGenerate) \
    macro(AllocationProfileWatchpoint, NodeMustGenerate) \
    macro(CheckInBounds, NodeMustGenerate) \
    \
    /* Optimizations for array mutation. */\
    macro(ArrayPush, NodeResultJS | NodeMustGenerate) \
    macro(ArrayPop, NodeResultJS | NodeMustGenerate) \
    \
    /* Optimizations for regular expression matching. */\
    macro(RegExpExec, NodeResultJS | NodeMustGenerate) \
    macro(RegExpTest, NodeResultJS | NodeMustGenerate) \
    \
    /* Optimizations for string access */ \
    macro(StringCharCodeAt, NodeResultInt32) \
    macro(StringCharAt, NodeResultJS) \
    macro(StringFromCharCode, NodeResultJS) \
    \
    /* Nodes for comparison operations. */\
    macro(CompareLess, NodeResultBoolean | NodeMustGenerate) \
    macro(CompareLessEq, NodeResultBoolean | NodeMustGenerate) \
    macro(CompareGreater, NodeResultBoolean | NodeMustGenerate) \
    macro(CompareGreaterEq, NodeResultBoolean | NodeMustGenerate) \
    macro(CompareEq, NodeResultBoolean | NodeMustGenerate) \
    macro(CompareEqConstant, NodeResultBoolean) \
    macro(CompareStrictEq, NodeResultBoolean) \
    \
    /* Calls. */\
    macro(Call, NodeResultJS | NodeMustGenerate | NodeHasVarArgs) \
    macro(Construct, NodeResultJS | NodeMustGenerate | NodeHasVarArgs) \
    macro(CallVarargs, NodeResultJS | NodeMustGenerate) \
    macro(CallForwardVarargs, NodeResultJS | NodeMustGenerate) \
    macro(ConstructVarargs, NodeResultJS | NodeMustGenerate) \
    macro(ConstructForwardVarargs, NodeResultJS | NodeMustGenerate) \
    macro(NativeCall, NodeResultJS | NodeMustGenerate | NodeHasVarArgs) \
    macro(NativeConstruct, NodeResultJS | NodeMustGenerate | NodeHasVarArgs) \
    \
    /* Allocations. */\
    macro(NewObject, NodeResultJS) \
    macro(NewArray, NodeResultJS | NodeHasVarArgs) \
    macro(NewArrayWithSize, NodeResultJS | NodeMustGenerate) \
    macro(NewArrayBuffer, NodeResultJS) \
    macro(NewTypedArray, NodeResultJS | NodeMustGenerate) \
    macro(NewRegexp, NodeResultJS) \
    \
    /* Support for allocation sinking. */\
    macro(PhantomNewObject, NodeResultJS) \
    macro(PutHint, NodeMustGenerate) \
    macro(CheckStructureImmediate, NodeMustGenerate) \
    macro(MaterializeNewObject, NodeResultJS | NodeHasVarArgs) \
    \
    /* Nodes for misc operations. */\
    macro(Breakpoint, NodeMustGenerate) \
    macro(ProfileWillCall, NodeMustGenerate) \
    macro(ProfileDidCall, NodeMustGenerate) \
    macro(CheckHasInstance, NodeMustGenerate) \
    macro(InstanceOf, NodeResultBoolean) \
    macro(IsUndefined, NodeResultBoolean) \
    macro(IsBoolean, NodeResultBoolean) \
    macro(IsNumber, NodeResultBoolean) \
    macro(IsString, NodeResultBoolean) \
    macro(IsObject, NodeResultBoolean) \
    macro(IsObjectOrNull, NodeResultBoolean) \
    macro(IsFunction, NodeResultBoolean) \
    macro(TypeOf, NodeResultJS) \
    macro(LogicalNot, NodeResultBoolean) \
    macro(ToPrimitive, NodeResultJS | NodeMustGenerate) \
    macro(ToString, NodeResultJS | NodeMustGenerate) \
    macro(NewStringObject, NodeResultJS) \
    macro(MakeRope, NodeResultJS) \
    macro(In, NodeResultBoolean | NodeMustGenerate) \
    macro(ProfileType, NodeMustGenerate) \
    macro(ProfileControlFlow, NodeMustGenerate) \
    \
    macro(CreateActivation, NodeResultJS) \
    \
    macro(CreateDirectArguments, NodeResultJS) \
    macro(PhantomDirectArguments, NodeResultJS) \
    macro(CreateScopedArguments, NodeResultJS) \
    macro(CreateClonedArguments, NodeResultJS) \
    macro(PhantomClonedArguments, NodeResultJS) \
    macro(GetFromArguments, NodeResultJS) \
    macro(PutToArguments, NodeMustGenerate) \
    \
    macro(NewFunction, NodeResultJS) \
    \
    /* These aren't terminals but always exit */ \
    macro(Throw, NodeMustGenerate) \
    macro(ThrowReferenceError, NodeMustGenerate) \
    \
    /* Block terminals. */\
    macro(Jump, NodeMustGenerate) \
    macro(Branch, NodeMustGenerate) \
    macro(Switch, NodeMustGenerate) \
    macro(Return, NodeMustGenerate) \
    macro(Unreachable, NodeMustGenerate) \
    \
    /* Count execution. */\
    macro(CountExecution, NodeMustGenerate) \
    \
    /* This is a pseudo-terminal. It means that execution should fall out of DFG at */\
    /* this point, but execution does continue in the basic block - just in a */\
    /* different compiler. */\
    macro(ForceOSRExit, NodeMustGenerate) \
    \
    /* Vends a bottom JS value. It is invalid to ever execute this. Useful for cases */\
    /* where we know that we would have exited but we'd like to still track the control */\
    /* flow. */\
    macro(BottomValue, NodeResultJS) \
    \
    /* Checks the watchdog timer. If the timer has fired, we OSR exit to the */ \
    /* baseline JIT to redo the watchdog timer check, and service the timer. */ \
    macro(CheckWatchdogTimer, NodeMustGenerate) \
    /* Write barriers ! */\
    macro(StoreBarrier, NodeMustGenerate) \
    macro(StoreBarrierWithNullCheck, NodeMustGenerate) \
    \
    /* For-in enumeration opcodes */\
    macro(GetEnumerableLength, NodeMustGenerate | NodeResultJS) \
    macro(HasIndexedProperty, NodeResultBoolean) \
    macro(HasStructureProperty, NodeResultBoolean) \
    macro(HasGenericProperty, NodeResultBoolean) \
    macro(GetDirectPname, NodeMustGenerate | NodeHasVarArgs | NodeResultJS) \
    macro(GetPropertyEnumerator, NodeMustGenerate | NodeResultJS) \
    macro(GetEnumeratorStructurePname, NodeMustGenerate | NodeResultJS) \
    macro(GetEnumeratorGenericPname, NodeMustGenerate | NodeResultJS) \
    macro(ToIndexString, NodeResultJS)

// This enum generates a monotonically increasing id for all Node types,
// and is used by the subsequent enum to fill out the id (as accessed via the NodeIdMask).
enum NodeType {
#define DFG_OP_ENUM(opcode, flags) opcode,
    FOR_EACH_DFG_OP(DFG_OP_ENUM)
#undef DFG_OP_ENUM
    LastNodeType
};

// Specifies the default flags for each node.
inline NodeFlags defaultFlags(NodeType op)
{
    switch (op) {
#define DFG_OP_ENUM(opcode, flags) case opcode: return flags;
    FOR_EACH_DFG_OP(DFG_OP_ENUM)
#undef DFG_OP_ENUM
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGNodeType_h

