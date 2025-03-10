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
#include "LLIntSlowPaths.h"

#include "ArrayConstructor.h"
#include "CallFrame.h"
#include "CommonSlowPaths.h"
#include "CommonSlowPathsExceptions.h"
#include "Error.h"
#include "ErrorHandlingScope.h"
#include "ExceptionFuzz.h"
#include "GetterSetter.h"
#include "HostCallReturnValue.h"
#include "Interpreter.h"
#include "JIT.h"
#include "JITExceptions.h"
#include "JSLexicalEnvironment.h"
#include "JSCInlines.h"
#include "JSCJSValue.h"
#include "JSGlobalObjectFunctions.h"
#include "JSNameScope.h"
#include "JSStackInlines.h"
#include "JSString.h"
#include "JSWithScope.h"
#include "LLIntCommon.h"
#include "LLIntExceptions.h"
#include "LegacyProfiler.h"
#include "LowLevelInterpreter.h"
#include "ObjectConstructor.h"
#include "ProtoCallFrame.h"
#include "StructureRareDataInlines.h"
#include <wtf/StringPrintStream.h>

namespace JSC { namespace LLInt {

#define LLINT_BEGIN_NO_SET_PC() \
    VM& vm = exec->vm();      \
    NativeCallFrameTracer tracer(&vm, exec)

#ifndef NDEBUG
#define LLINT_SET_PC_FOR_STUBS() do { \
        exec->codeBlock()->bytecodeOffset(pc); \
        exec->setCurrentVPC(pc + 1); \
    } while (false)
#else
#define LLINT_SET_PC_FOR_STUBS() do { \
        exec->setCurrentVPC(pc + 1); \
    } while (false)
#endif

#define LLINT_BEGIN()                           \
    LLINT_BEGIN_NO_SET_PC();                    \
    LLINT_SET_PC_FOR_STUBS()

#define LLINT_OP(index) (exec->uncheckedR(pc[index].u.operand))
#define LLINT_OP_C(index) (exec->r(pc[index].u.operand))

#define LLINT_RETURN_TWO(first, second) do {       \
        return encodeResult(first, second);        \
    } while (false)

#define LLINT_END_IMPL() LLINT_RETURN_TWO(pc, 0)

#define LLINT_THROW(exceptionToThrow) do {                        \
        vm.throwException(exec, exceptionToThrow);                \
        pc = returnToThrow(exec);                                 \
        LLINT_END_IMPL();                                         \
    } while (false)

#define LLINT_CHECK_EXCEPTION() do {                    \
        doExceptionFuzzingIfEnabled(exec, "LLIntSlowPaths", pc);    \
        if (UNLIKELY(vm.exception())) {                 \
            pc = returnToThrow(exec);                   \
            LLINT_END_IMPL();                           \
        }                                               \
    } while (false)

#define LLINT_END() do {                        \
        LLINT_CHECK_EXCEPTION();                \
        LLINT_END_IMPL();                       \
    } while (false)

#define LLINT_BRANCH(opcode, condition) do {                      \
        bool __b_condition = (condition);                         \
        LLINT_CHECK_EXCEPTION();                                  \
        if (__b_condition)                                        \
            pc += pc[OPCODE_LENGTH(opcode) - 1].u.operand;        \
        else                                                      \
            pc += OPCODE_LENGTH(opcode);                          \
        LLINT_END_IMPL();                                         \
    } while (false)

#define LLINT_RETURN(value) do {                \
        JSValue __r_returnValue = (value);      \
        LLINT_CHECK_EXCEPTION();                \
        LLINT_OP(1) = __r_returnValue;          \
        LLINT_END_IMPL();                       \
    } while (false)

#define LLINT_RETURN_WITH_PC_ADJUSTMENT(value, pcAdjustment) do { \
        JSValue __r_returnValue = (value);      \
        LLINT_CHECK_EXCEPTION();                \
        LLINT_OP(1) = __r_returnValue;          \
        pc += (pcAdjustment);                   \
        LLINT_END_IMPL();                       \
    } while (false)

#define LLINT_RETURN_PROFILED(opcode, value) do {               \
        JSValue __rp_returnValue = (value);                     \
        LLINT_CHECK_EXCEPTION();                                \
        LLINT_OP(1) = __rp_returnValue;                         \
        LLINT_PROFILE_VALUE(opcode, __rp_returnValue);          \
        LLINT_END_IMPL();                                       \
    } while (false)

#define LLINT_PROFILE_VALUE(opcode, value) do { \
        pc[OPCODE_LENGTH(opcode) - 1].u.profile->m_buckets[0] = \
        JSValue::encode(value);                  \
    } while (false)

#define LLINT_CALL_END_IMPL(exec, callTarget) LLINT_RETURN_TWO((callTarget), (exec))

#define LLINT_CALL_THROW(exec, exceptionToThrow) do {                   \
        ExecState* __ct_exec = (exec);                                  \
        vm.throwException(__ct_exec, exceptionToThrow);                 \
        LLINT_CALL_END_IMPL(0, callToThrow(__ct_exec));                 \
    } while (false)

#define LLINT_CALL_CHECK_EXCEPTION(exec, execCallee) do {               \
        ExecState* __cce_exec = (exec);                                 \
        ExecState* __cce_execCallee = (execCallee);                     \
        doExceptionFuzzingIfEnabled(__cce_exec, "LLIntSlowPaths/call", nullptr); \
        if (UNLIKELY(vm.exception()))                                   \
            LLINT_CALL_END_IMPL(0, callToThrow(__cce_execCallee));      \
    } while (false)

#define LLINT_CALL_RETURN(exec, execCallee, callTarget) do {            \
        ExecState* __cr_exec = (exec);                                  \
        ExecState* __cr_execCallee = (execCallee);                      \
        void* __cr_callTarget = (callTarget);                           \
        LLINT_CALL_CHECK_EXCEPTION(__cr_exec, __cr_execCallee);         \
        LLINT_CALL_END_IMPL(__cr_execCallee, __cr_callTarget);          \
    } while (false)

#define LLINT_RETURN_CALLEE_FRAME(execCallee) do {                      \
        ExecState* __rcf_exec = (execCallee);                           \
        LLINT_RETURN_TWO(pc, __rcf_exec);                               \
    } while (false)

extern "C" SlowPathReturnType llint_trace_operand(ExecState* exec, Instruction* pc, int fromWhere, int operand)
{
    LLINT_BEGIN();
    dataLogF("%p / %p: executing bc#%zu, op#%u: Trace(%d): %d: %d\n",
            exec->codeBlock(),
            exec,
            static_cast<intptr_t>(pc - exec->codeBlock()->instructions().begin()),
            exec->vm().interpreter->getOpcodeID(pc[0].u.opcode),
            fromWhere,
            operand,
            pc[operand].u.operand);
    LLINT_END();
}

extern "C" SlowPathReturnType llint_trace_value(ExecState* exec, Instruction* pc, int fromWhere, int operand)
{
    JSValue value = LLINT_OP_C(operand).jsValue();
    union {
        struct {
            uint32_t tag;
            uint32_t payload;
        } bits;
        EncodedJSValue asValue;
    } u;
    u.asValue = JSValue::encode(value);
    dataLogF(
        "%p / %p: executing bc#%zu, op#%u: Trace(%d): %d: %d: %08x:%08x: %s\n",
        exec->codeBlock(),
        exec,
        static_cast<intptr_t>(pc - exec->codeBlock()->instructions().begin()),
        exec->vm().interpreter->getOpcodeID(pc[0].u.opcode),
        fromWhere,
        operand,
        pc[operand].u.operand,
        u.bits.tag,
        u.bits.payload,
        toCString(value).data());
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(trace_prologue)
{
    dataLogF("%p / %p: in prologue.\n", exec->codeBlock(), exec);
    LLINT_END_IMPL();
}

static void traceFunctionPrologue(ExecState* exec, const char* comment, CodeSpecializationKind kind)
{
    JSFunction* callee = jsCast<JSFunction*>(exec->callee());
    FunctionExecutable* executable = callee->jsExecutable();
    CodeBlock* codeBlock = executable->codeBlockFor(kind);
    dataLogF("%p / %p: in %s of function %p, executable %p; numVars = %u, numParameters = %u, numCalleeRegisters = %u, caller = %p.\n",
            codeBlock, exec, comment, callee, executable,
            codeBlock->m_numVars, codeBlock->numParameters(), codeBlock->m_numCalleeRegisters,
            exec->callerFrame());
}

LLINT_SLOW_PATH_DECL(trace_prologue_function_for_call)
{
    traceFunctionPrologue(exec, "call prologue", CodeForCall);
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(trace_prologue_function_for_construct)
{
    traceFunctionPrologue(exec, "construct prologue", CodeForConstruct);
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(trace_arityCheck_for_call)
{
    traceFunctionPrologue(exec, "call arity check", CodeForCall);
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(trace_arityCheck_for_construct)
{
    traceFunctionPrologue(exec, "construct arity check", CodeForConstruct);
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(trace)
{
    dataLogF("%p / %p: executing bc#%zu, %s, pc = %p\n",
            exec->codeBlock(),
            exec,
            static_cast<intptr_t>(pc - exec->codeBlock()->instructions().begin()),
            opcodeNames[exec->vm().interpreter->getOpcodeID(pc[0].u.opcode)], pc);
    if (exec->vm().interpreter->getOpcodeID(pc[0].u.opcode) == op_enter) {
        dataLogF("Frame will eventually return to %p\n", exec->returnPC().value());
        *bitwise_cast<volatile char*>(exec->returnPC().value());
    }
    if (exec->vm().interpreter->getOpcodeID(pc[0].u.opcode) == op_ret) {
        dataLogF("Will be returning to %p\n", exec->returnPC().value());
        dataLogF("The new cfr will be %p\n", exec->callerFrame());
    }
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(special_trace)
{
    dataLogF("%p / %p: executing special case bc#%zu, op#%u, return PC is %p\n",
            exec->codeBlock(),
            exec,
            static_cast<intptr_t>(pc - exec->codeBlock()->instructions().begin()),
            exec->vm().interpreter->getOpcodeID(pc[0].u.opcode),
            exec->returnPC().value());
    LLINT_END_IMPL();
}

enum EntryKind { Prologue, ArityCheck };

#if ENABLE(JIT)
inline bool shouldJIT(ExecState* exec)
{
    // You can modify this to turn off JITting without rebuilding the world.
    return exec->vm().canUseJIT();
}

// Returns true if we should try to OSR.
inline bool jitCompileAndSetHeuristics(CodeBlock* codeBlock, ExecState* exec)
{
    VM& vm = exec->vm();
    DeferGCForAWhile deferGC(vm.heap); // My callers don't set top callframe, so we don't want to GC here at all.
    
    codeBlock->updateAllValueProfilePredictions();
    
    if (!codeBlock->checkIfJITThresholdReached()) {
        if (Options::verboseOSR())
            dataLogF("    JIT threshold should be lifted.\n");
        return false;
    }
    
    switch (codeBlock->jitType()) {
    case JITCode::BaselineJIT: {
        if (Options::verboseOSR())
            dataLogF("    Code was already compiled.\n");
        codeBlock->jitSoon();
        return true;
    }
    case JITCode::InterpreterThunk: {
        CompilationResult result = JIT::compile(&vm, codeBlock, JITCompilationCanFail);
        switch (result) {
        case CompilationFailed:
            if (Options::verboseOSR())
                dataLogF("    JIT compilation failed.\n");
            codeBlock->dontJITAnytimeSoon();
            return false;
        case CompilationSuccessful:
            if (Options::verboseOSR())
                dataLogF("    JIT compilation successful.\n");
            codeBlock->install();
            codeBlock->jitSoon();
            return true;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return false;
        }
    }
    default:
        dataLog("Unexpected code block in LLInt: ", *codeBlock, "\n");
        RELEASE_ASSERT_NOT_REACHED();
        return false;
    }
}

static SlowPathReturnType entryOSR(ExecState* exec, Instruction*, CodeBlock* codeBlock, const char *name, EntryKind kind)
{
    if (Options::verboseOSR()) {
        dataLog(
            *codeBlock, ": Entered ", name, " with executeCounter = ",
            codeBlock->llintExecuteCounter(), "\n");
    }
    
    if (!shouldJIT(exec)) {
        codeBlock->dontJITAnytimeSoon();
        LLINT_RETURN_TWO(0, 0);
    }
    if (!jitCompileAndSetHeuristics(codeBlock, exec))
        LLINT_RETURN_TWO(0, 0);
    
    if (kind == Prologue)
        LLINT_RETURN_TWO(codeBlock->jitCode()->executableAddress(), 0);
    ASSERT(kind == ArityCheck);
    LLINT_RETURN_TWO(codeBlock->jitCode()->addressForCall(
        *codeBlock->vm(), codeBlock->ownerExecutable(), MustCheckArity,
        RegisterPreservationNotRequired).executableAddress(), 0);
}
#else // ENABLE(JIT)
static SlowPathReturnType entryOSR(ExecState* exec, Instruction*, CodeBlock* codeBlock, const char*, EntryKind)
{
    codeBlock->dontJITAnytimeSoon();
    LLINT_RETURN_TWO(0, exec);
}
#endif // ENABLE(JIT)

LLINT_SLOW_PATH_DECL(entry_osr)
{
    return entryOSR(exec, pc, exec->codeBlock(), "entry_osr", Prologue);
}

LLINT_SLOW_PATH_DECL(entry_osr_function_for_call)
{
    return entryOSR(exec, pc, jsCast<JSFunction*>(exec->callee())->jsExecutable()->codeBlockForCall(), "entry_osr_function_for_call", Prologue);
}

LLINT_SLOW_PATH_DECL(entry_osr_function_for_construct)
{
    return entryOSR(exec, pc, jsCast<JSFunction*>(exec->callee())->jsExecutable()->codeBlockForConstruct(), "entry_osr_function_for_construct", Prologue);
}

LLINT_SLOW_PATH_DECL(entry_osr_function_for_call_arityCheck)
{
    return entryOSR(exec, pc, jsCast<JSFunction*>(exec->callee())->jsExecutable()->codeBlockForCall(), "entry_osr_function_for_call_arityCheck", ArityCheck);
}

LLINT_SLOW_PATH_DECL(entry_osr_function_for_construct_arityCheck)
{
    return entryOSR(exec, pc, jsCast<JSFunction*>(exec->callee())->jsExecutable()->codeBlockForConstruct(), "entry_osr_function_for_construct_arityCheck", ArityCheck);
}

LLINT_SLOW_PATH_DECL(loop_osr)
{
    CodeBlock* codeBlock = exec->codeBlock();

#if ENABLE(JIT)
    if (Options::verboseOSR()) {
        dataLog(
            *codeBlock, ": Entered loop_osr with executeCounter = ",
            codeBlock->llintExecuteCounter(), "\n");
    }
    
    if (!shouldJIT(exec)) {
        codeBlock->dontJITAnytimeSoon();
        LLINT_RETURN_TWO(0, 0);
    }
    
    if (!jitCompileAndSetHeuristics(codeBlock, exec))
        LLINT_RETURN_TWO(0, 0);
    
    ASSERT(codeBlock->jitType() == JITCode::BaselineJIT);
    
    Vector<BytecodeAndMachineOffset> map;
    codeBlock->jitCodeMap()->decode(map);
    BytecodeAndMachineOffset* mapping = binarySearch<BytecodeAndMachineOffset, unsigned>(map, map.size(), pc - codeBlock->instructions().begin(), BytecodeAndMachineOffset::getBytecodeIndex);
    ASSERT(mapping);
    ASSERT(mapping->m_bytecodeIndex == static_cast<unsigned>(pc - codeBlock->instructions().begin()));
    
    void* jumpTarget = codeBlock->jitCode()->executableAddressAtOffset(mapping->m_machineCodeOffset);
    ASSERT(jumpTarget);
    
    LLINT_RETURN_TWO(jumpTarget, exec->topOfFrame());
#else // ENABLE(JIT)
    UNUSED_PARAM(pc);
    codeBlock->dontJITAnytimeSoon();
    LLINT_RETURN_TWO(0, 0);
#endif // ENABLE(JIT)
}

LLINT_SLOW_PATH_DECL(replace)
{
    CodeBlock* codeBlock = exec->codeBlock();

#if ENABLE(JIT)
    if (Options::verboseOSR()) {
        dataLog(
            *codeBlock, ": Entered replace with executeCounter = ",
            codeBlock->llintExecuteCounter(), "\n");
    }
    
    if (shouldJIT(exec))
        jitCompileAndSetHeuristics(codeBlock, exec);
    else
        codeBlock->dontJITAnytimeSoon();
    LLINT_END_IMPL();
#else // ENABLE(JIT)
    codeBlock->dontJITAnytimeSoon();
    LLINT_END_IMPL();
#endif // ENABLE(JIT)
}

LLINT_SLOW_PATH_DECL(stack_check)
{
    LLINT_BEGIN();
#if LLINT_SLOW_PATH_TRACING
    dataLogF("Checking stack height with exec = %p.\n", exec);
    dataLogF("CodeBlock = %p.\n", exec->codeBlock());
    dataLogF("Num callee registers = %u.\n", exec->codeBlock()->m_numCalleeRegisters);
    dataLogF("Num vars = %u.\n", exec->codeBlock()->m_numVars);

#if ENABLE(LLINT_C_LOOP)
    dataLogF("Current end is at %p.\n", exec->vm().jsStackLimit());
#else
    dataLogF("Current end is at %p.\n", exec->vm().stackLimit());
#endif

#endif
    // This stack check is done in the prologue for a function call, and the
    // CallFrame is not completely set up yet. For example, if the frame needs
    // a lexical environment object, the lexical environment object will only be
    // set up after we start executing the function. If we need to throw a
    // StackOverflowError here, then we need to tell the prologue to start the
    // stack unwinding from the caller frame (which is fully set up) instead.
    // To do that, we return the caller's CallFrame in the second return value.
    //
    // If the stack check succeeds and we don't need to throw the error, then
    // we'll return 0 instead. The prologue will check for a non-zero value
    // when determining whether to set the callFrame or not.

    // For JIT enabled builds which uses the C stack, the stack is not growable.
    // Hence, if we get here, then we know a stack overflow is imminent. So, just
    // throw the StackOverflowError unconditionally.
#if !ENABLE(JIT)
    ASSERT(!vm.interpreter->stack().containsAddress(exec->topOfFrame()));
    if (LIKELY(vm.interpreter->stack().ensureCapacityFor(exec->topOfFrame())))
        LLINT_RETURN_TWO(pc, 0);
#endif

    exec = exec->callerFrame(vm.topVMEntryFrame);
    vm.topCallFrame = exec;
    ErrorHandlingScope errorScope(vm);
    CommonSlowPaths::interpreterThrowInCaller(exec, createStackOverflowError(exec));
    pc = returnToThrowForThrownException(exec);
    LLINT_RETURN_TWO(pc, exec);
}

LLINT_SLOW_PATH_DECL(slow_path_create_lexical_environment)
{
    LLINT_BEGIN();
#if LLINT_SLOW_PATH_TRACING
    dataLogF("Creating an lexicalEnvironment, exec = %p!\n", exec);
#endif
    int scopeReg = pc[2].u.operand;
    JSScope* scope = exec->uncheckedR(scopeReg).Register::scope();
    JSLexicalEnvironment* lexicalEnvironment = JSLexicalEnvironment::create(vm, exec, scope, exec->codeBlock());
    LLINT_RETURN(JSValue(lexicalEnvironment));
}

LLINT_SLOW_PATH_DECL(slow_path_new_object)
{
    LLINT_BEGIN();
    LLINT_RETURN(constructEmptyObject(exec, pc[3].u.objectAllocationProfile->structure()));
}

LLINT_SLOW_PATH_DECL(slow_path_new_array)
{
    LLINT_BEGIN();
    LLINT_RETURN(constructArrayNegativeIndexed(exec, pc[4].u.arrayAllocationProfile, bitwise_cast<JSValue*>(&LLINT_OP(2)), pc[3].u.operand));
}

LLINT_SLOW_PATH_DECL(slow_path_new_array_with_size)
{
    LLINT_BEGIN();
    LLINT_RETURN(constructArrayWithSizeQuirk(exec, pc[3].u.arrayAllocationProfile, exec->lexicalGlobalObject(), LLINT_OP_C(2).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_new_array_buffer)
{
    LLINT_BEGIN();
    LLINT_RETURN(constructArray(exec, pc[4].u.arrayAllocationProfile, exec->codeBlock()->constantBuffer(pc[2].u.operand), pc[3].u.operand));
}

LLINT_SLOW_PATH_DECL(slow_path_new_regexp)
{
    LLINT_BEGIN();
    RegExp* regExp = exec->codeBlock()->regexp(pc[2].u.operand);
    if (!regExp->isValid())
        LLINT_THROW(createSyntaxError(exec, "Invalid flag supplied to RegExp constructor."));
    LLINT_RETURN(RegExpObject::create(vm, exec->lexicalGlobalObject()->regExpStructure(), regExp));
}

LLINT_SLOW_PATH_DECL(slow_path_check_has_instance)
{
    LLINT_BEGIN();
    
    JSValue value = LLINT_OP_C(2).jsValue();
    JSValue baseVal = LLINT_OP_C(3).jsValue();
    if (baseVal.isObject()) {
        JSObject* baseObject = asObject(baseVal);
        ASSERT(!baseObject->structure()->typeInfo().implementsDefaultHasInstance());
        if (baseObject->structure()->typeInfo().implementsHasInstance()) {
            JSValue result = jsBoolean(baseObject->methodTable()->customHasInstance(baseObject, exec, value));
            LLINT_RETURN_WITH_PC_ADJUSTMENT(result, pc[4].u.operand);
        }
    }
    LLINT_THROW(createInvalidInstanceofParameterError(exec, baseVal));
}

LLINT_SLOW_PATH_DECL(slow_path_instanceof)
{
    LLINT_BEGIN();
    JSValue value = LLINT_OP_C(2).jsValue();
    JSValue proto = LLINT_OP_C(3).jsValue();
    ASSERT(!value.isObject() || !proto.isObject());
    LLINT_RETURN(jsBoolean(JSObject::defaultHasInstance(exec, value, proto)));
}

LLINT_SLOW_PATH_DECL(slow_path_get_by_id)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    const Identifier& ident = codeBlock->identifier(pc[3].u.operand);
    JSValue baseValue = LLINT_OP_C(2).jsValue();
    PropertySlot slot(baseValue);

    JSValue result = baseValue.get(exec, ident, slot);
    LLINT_CHECK_EXCEPTION();
    LLINT_OP(1) = result;
    
    if (!LLINT_ALWAYS_ACCESS_SLOW
        && baseValue.isCell()
        && slot.isCacheable()
        && slot.slotBase() == baseValue
        && slot.isCacheableValue()) {
        
        JSCell* baseCell = baseValue.asCell();
        Structure* structure = baseCell->structure();
        
        if (!structure->isUncacheableDictionary()
            && !structure->typeInfo().prohibitsPropertyCaching()
            && !structure->typeInfo().newImpurePropertyFiresWatchpoints()) {
            ConcurrentJITLocker locker(codeBlock->m_lock);

            pc[4].u.structure.set(
                vm, codeBlock->ownerExecutable(), structure);
            if (isInlineOffset(slot.cachedOffset())) {
                pc[0].u.opcode = LLInt::getOpcode(op_get_by_id);
                pc[5].u.operand = offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + JSObject::offsetOfInlineStorage();
            } else {
                pc[0].u.opcode = LLInt::getOpcode(op_get_by_id_out_of_line);
                pc[5].u.operand = offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue);
            }
        }
    }

    if (!LLINT_ALWAYS_ACCESS_SLOW
        && isJSArray(baseValue)
        && ident == exec->propertyNames().length) {
        pc[0].u.opcode = LLInt::getOpcode(op_get_array_length);
        ArrayProfile* arrayProfile = codeBlock->getOrAddArrayProfile(pc - codeBlock->instructions().begin());
        arrayProfile->observeStructure(baseValue.asCell()->structure());
        pc[4].u.arrayProfile = arrayProfile;
    }

    pc[OPCODE_LENGTH(op_get_by_id) - 1].u.profile->m_buckets[0] = JSValue::encode(result);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_get_arguments_length)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    const Identifier& ident = codeBlock->identifier(pc[3].u.operand);
    JSValue baseValue = LLINT_OP(2).jsValue();
    PropertySlot slot(baseValue);
    LLINT_RETURN(baseValue.get(exec, ident, slot));
}

LLINT_SLOW_PATH_DECL(slow_path_put_by_id)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    const Identifier& ident = codeBlock->identifier(pc[2].u.operand);
    
    JSValue baseValue = LLINT_OP_C(1).jsValue();
    PutPropertySlot slot(baseValue, codeBlock->isStrictMode(), codeBlock->putByIdContext());
    if (pc[8].u.operand)
        asObject(baseValue)->putDirect(vm, ident, LLINT_OP_C(3).jsValue(), slot);
    else
        baseValue.put(exec, ident, LLINT_OP_C(3).jsValue(), slot);
    LLINT_CHECK_EXCEPTION();
    
    if (!LLINT_ALWAYS_ACCESS_SLOW
        && baseValue.isCell()
        && slot.isCacheablePut()) {
        
        JSCell* baseCell = baseValue.asCell();
        Structure* structure = baseCell->structure();
        
        if (!structure->isUncacheableDictionary()
            && !structure->typeInfo().prohibitsPropertyCaching()
            && baseCell == slot.base()) {
            
            if (slot.type() == PutPropertySlot::NewProperty) {
                GCSafeConcurrentJITLocker locker(codeBlock->m_lock, vm.heap);
            
                if (!structure->isDictionary() && structure->previousID()->outOfLineCapacity() == structure->outOfLineCapacity()) {
                    ASSERT(structure->previousID()->transitionWatchpointSetHasBeenInvalidated());
                    
                    // This is needed because some of the methods we call
                    // below may GC.
                    pc[0].u.opcode = LLInt::getOpcode(op_put_by_id);

                    if (normalizePrototypeChain(exec, structure) != InvalidPrototypeChain) {
                        ASSERT(structure->previousID()->isObject());
                        pc[4].u.structure.set(
                            vm, codeBlock->ownerExecutable(), structure->previousID());
                        if (isInlineOffset(slot.cachedOffset()))
                            pc[5].u.operand = offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + JSObject::offsetOfInlineStorage();
                        else
                            pc[5].u.operand = offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue);
                        pc[6].u.structure.set(
                            vm, codeBlock->ownerExecutable(), structure);
                        StructureChain* chain = structure->prototypeChain(exec);
                        ASSERT(chain);
                        pc[7].u.structureChain.set(
                            vm, codeBlock->ownerExecutable(), chain);
                    
                        if (pc[8].u.operand) {
                            if (isInlineOffset(slot.cachedOffset()))
                                pc[0].u.opcode = LLInt::getOpcode(op_put_by_id_transition_direct);
                            else
                                pc[0].u.opcode = LLInt::getOpcode(op_put_by_id_transition_direct_out_of_line);
                        } else {
                            if (isInlineOffset(slot.cachedOffset()))
                                pc[0].u.opcode = LLInt::getOpcode(op_put_by_id_transition_normal);
                            else
                                pc[0].u.opcode = LLInt::getOpcode(op_put_by_id_transition_normal_out_of_line);
                        }
                    }
                }
            } else {
                structure->didCachePropertyReplacement(vm, slot.cachedOffset());
                pc[4].u.structure.set(
                    vm, codeBlock->ownerExecutable(), structure);
                if (isInlineOffset(slot.cachedOffset())) {
                    pc[0].u.opcode = LLInt::getOpcode(op_put_by_id);
                    pc[5].u.operand = offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + JSObject::offsetOfInlineStorage();
                } else {
                    pc[0].u.opcode = LLInt::getOpcode(op_put_by_id_out_of_line);
                    pc[5].u.operand = offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue);
                }
            }
        }
    }
    
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_del_by_id)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    JSObject* baseObject = LLINT_OP_C(2).jsValue().toObject(exec);
    bool couldDelete = baseObject->methodTable()->deleteProperty(baseObject, exec, codeBlock->identifier(pc[3].u.operand));
    LLINT_CHECK_EXCEPTION();
    if (!couldDelete && codeBlock->isStrictMode())
        LLINT_THROW(createTypeError(exec, "Unable to delete property."));
    LLINT_RETURN(jsBoolean(couldDelete));
}

inline JSValue getByVal(ExecState* exec, JSValue baseValue, JSValue subscript)
{
    if (LIKELY(baseValue.isCell() && subscript.isString())) {
        VM& vm = exec->vm();
        Structure& structure = *baseValue.asCell()->structure(vm);
        if (JSCell::canUseFastGetOwnProperty(structure)) {
            if (AtomicStringImpl* existingAtomicString = asString(subscript)->toExistingAtomicString(exec)) {
                if (JSValue result = baseValue.asCell()->fastGetOwnProperty(vm, structure, existingAtomicString))
                    return result;
            }
        }
    }
    
    if (subscript.isUInt32()) {
        uint32_t i = subscript.asUInt32();
        if (isJSString(baseValue) && asString(baseValue)->canGetIndex(i))
            return asString(baseValue)->getIndex(exec, i);
        
        return baseValue.get(exec, i);
    }

    baseValue.requireObjectCoercible(exec);
    if (exec->hadException())
        return jsUndefined();
    auto property = subscript.toPropertyKey(exec);
    if (exec->hadException())
        return jsUndefined();
    return baseValue.get(exec, property);
}

LLINT_SLOW_PATH_DECL(slow_path_get_by_val)
{
    LLINT_BEGIN();
    LLINT_RETURN_PROFILED(op_get_by_val, getByVal(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(3).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_put_by_val)
{
    LLINT_BEGIN();
    
    JSValue baseValue = LLINT_OP_C(1).jsValue();
    JSValue subscript = LLINT_OP_C(2).jsValue();
    JSValue value = LLINT_OP_C(3).jsValue();
    
    if (LIKELY(subscript.isUInt32())) {
        uint32_t i = subscript.asUInt32();
        if (baseValue.isObject()) {
            JSObject* object = asObject(baseValue);
            if (object->canSetIndexQuickly(i))
                object->setIndexQuickly(vm, i, value);
            else
                object->methodTable()->putByIndex(object, exec, i, value, exec->codeBlock()->isStrictMode());
            LLINT_END();
        }
        baseValue.putByIndex(exec, i, value, exec->codeBlock()->isStrictMode());
        LLINT_END();
    }

    auto property = subscript.toPropertyKey(exec);
    LLINT_CHECK_EXCEPTION();
    PutPropertySlot slot(baseValue, exec->codeBlock()->isStrictMode());
    baseValue.put(exec, property, value, slot);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_put_by_val_direct)
{
    LLINT_BEGIN();
    
    JSValue baseValue = LLINT_OP_C(1).jsValue();
    JSValue subscript = LLINT_OP_C(2).jsValue();
    JSValue value = LLINT_OP_C(3).jsValue();
    RELEASE_ASSERT(baseValue.isObject());
    JSObject* baseObject = asObject(baseValue);
    if (LIKELY(subscript.isUInt32())) {
        uint32_t i = subscript.asUInt32();
        baseObject->putDirectIndex(exec, i, value);
    } else {
        auto property = subscript.toPropertyKey(exec);
        if (!exec->vm().exception()) { // Don't put to an object if toString threw an exception.
            PutPropertySlot slot(baseObject, exec->codeBlock()->isStrictMode());
            baseObject->putDirect(exec->vm(), property, value, slot);
        }
    }
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_del_by_val)
{
    LLINT_BEGIN();
    JSValue baseValue = LLINT_OP_C(2).jsValue();
    JSObject* baseObject = baseValue.toObject(exec);
    
    JSValue subscript = LLINT_OP_C(3).jsValue();
    
    bool couldDelete;
    
    uint32_t i;
    if (subscript.getUInt32(i))
        couldDelete = baseObject->methodTable()->deletePropertyByIndex(baseObject, exec, i);
    else {
        LLINT_CHECK_EXCEPTION();
        auto property = subscript.toPropertyKey(exec);
        LLINT_CHECK_EXCEPTION();
        couldDelete = baseObject->methodTable()->deleteProperty(baseObject, exec, property);
    }
    
    if (!couldDelete && exec->codeBlock()->isStrictMode())
        LLINT_THROW(createTypeError(exec, "Unable to delete property."));
    
    LLINT_RETURN(jsBoolean(couldDelete));
}

LLINT_SLOW_PATH_DECL(slow_path_put_by_index)
{
    LLINT_BEGIN();
    JSValue arrayValue = LLINT_OP_C(1).jsValue();
    ASSERT(isJSArray(arrayValue));
    asArray(arrayValue)->putDirectIndex(exec, pc[2].u.operand, LLINT_OP_C(3).jsValue());
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_put_getter_setter)
{
    LLINT_BEGIN();
    ASSERT(LLINT_OP(1).jsValue().isObject());
    JSObject* baseObj = asObject(LLINT_OP(1).jsValue());
    
    GetterSetter* accessor = GetterSetter::create(vm, exec->lexicalGlobalObject());
    LLINT_CHECK_EXCEPTION();
    
    JSValue getter = LLINT_OP(3).jsValue();
    JSValue setter = LLINT_OP(4).jsValue();
    ASSERT(getter.isObject() || getter.isUndefined());
    ASSERT(setter.isObject() || setter.isUndefined());
    ASSERT(getter.isObject() || setter.isObject());
    
    if (!getter.isUndefined())
        accessor->setGetter(vm, exec->lexicalGlobalObject(), asObject(getter));
    if (!setter.isUndefined())
        accessor->setSetter(vm, exec->lexicalGlobalObject(), asObject(setter));
    baseObj->putDirectAccessor(
        exec,
        exec->codeBlock()->identifier(pc[2].u.operand),
        accessor, Accessor);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_jtrue)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jtrue, LLINT_OP_C(1).jsValue().toBoolean(exec));
}

LLINT_SLOW_PATH_DECL(slow_path_jfalse)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jfalse, !LLINT_OP_C(1).jsValue().toBoolean(exec));
}

LLINT_SLOW_PATH_DECL(slow_path_jless)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jless, jsLess<true>(exec, LLINT_OP_C(1).jsValue(), LLINT_OP_C(2).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jnless)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jnless, !jsLess<true>(exec, LLINT_OP_C(1).jsValue(), LLINT_OP_C(2).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jgreater)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jgreater, jsLess<false>(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(1).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jngreater)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jngreater, !jsLess<false>(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(1).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jlesseq)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jlesseq, jsLessEq<true>(exec, LLINT_OP_C(1).jsValue(), LLINT_OP_C(2).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jnlesseq)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jnlesseq, !jsLessEq<true>(exec, LLINT_OP_C(1).jsValue(), LLINT_OP_C(2).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jgreatereq)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jgreatereq, jsLessEq<false>(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(1).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jngreatereq)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jngreatereq, !jsLessEq<false>(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(1).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_switch_imm)
{
    LLINT_BEGIN();
    JSValue scrutinee = LLINT_OP_C(3).jsValue();
    ASSERT(scrutinee.isDouble());
    double value = scrutinee.asDouble();
    int32_t intValue = static_cast<int32_t>(value);
    int defaultOffset = pc[2].u.operand;
    if (value == intValue) {
        CodeBlock* codeBlock = exec->codeBlock();
        pc += codeBlock->switchJumpTable(pc[1].u.operand).offsetForValue(intValue, defaultOffset);
    } else
        pc += defaultOffset;
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_switch_char)
{
    LLINT_BEGIN();
    JSValue scrutinee = LLINT_OP_C(3).jsValue();
    ASSERT(scrutinee.isString());
    JSString* string = asString(scrutinee);
    ASSERT(string->length() == 1);
    int defaultOffset = pc[2].u.operand;
    StringImpl* impl = string->value(exec).impl();
    CodeBlock* codeBlock = exec->codeBlock();
    pc += codeBlock->switchJumpTable(pc[1].u.operand).offsetForValue((*impl)[0], defaultOffset);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_switch_string)
{
    LLINT_BEGIN();
    JSValue scrutinee = LLINT_OP_C(3).jsValue();
    int defaultOffset = pc[2].u.operand;
    if (!scrutinee.isString())
        pc += defaultOffset;
    else {
        CodeBlock* codeBlock = exec->codeBlock();
        pc += codeBlock->stringSwitchJumpTable(pc[1].u.operand).offsetForValue(asString(scrutinee)->value(exec).impl(), defaultOffset);
    }
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_new_func)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    ASSERT(codeBlock->codeType() != FunctionCode || !codeBlock->needsActivation() || exec->hasActivation());
    JSScope* scope = exec->uncheckedR(pc[2].u.operand).Register::scope();
#if LLINT_SLOW_PATH_TRACING
    dataLogF("Creating function!\n");
#endif
    LLINT_RETURN(JSFunction::create(vm, codeBlock->functionDecl(pc[3].u.operand), scope));
}

LLINT_SLOW_PATH_DECL(slow_path_new_func_exp)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    JSScope* scope = exec->uncheckedR(pc[2].u.operand).Register::scope();
    FunctionExecutable* function = codeBlock->functionExpr(pc[3].u.operand);
    JSFunction* func = JSFunction::create(vm, function, scope);
    
    LLINT_RETURN(func);
}

static SlowPathReturnType handleHostCall(ExecState* execCallee, Instruction* pc, JSValue callee, CodeSpecializationKind kind)
{
    UNUSED_PARAM(pc);

#if LLINT_SLOW_PATH_TRACING
    dataLog("Performing host call.\n");
#endif
    
    ExecState* exec = execCallee->callerFrame();
    VM& vm = exec->vm();

    execCallee->setCodeBlock(0);
    execCallee->clearReturnPC();

    if (kind == CodeForCall) {
        CallData callData;
        CallType callType = getCallData(callee, callData);
    
        ASSERT(callType != CallTypeJS);
    
        if (callType == CallTypeHost) {
            NativeCallFrameTracer tracer(&vm, execCallee);
            execCallee->setCallee(asObject(callee));
            vm.hostCallReturnValue = JSValue::decode(callData.native.function(execCallee));
            
            LLINT_CALL_RETURN(execCallee, execCallee, LLInt::getCodePtr(getHostCallReturnValue));
        }
        
#if LLINT_SLOW_PATH_TRACING
        dataLog("Call callee is not a function: ", callee, "\n");
#endif

        ASSERT(callType == CallTypeNone);
        LLINT_CALL_THROW(exec, createNotAFunctionError(exec, callee));
    }

    ASSERT(kind == CodeForConstruct);
    
    ConstructData constructData;
    ConstructType constructType = getConstructData(callee, constructData);
    
    ASSERT(constructType != ConstructTypeJS);
    
    if (constructType == ConstructTypeHost) {
        NativeCallFrameTracer tracer(&vm, execCallee);
        execCallee->setCallee(asObject(callee));
        vm.hostCallReturnValue = JSValue::decode(constructData.native.function(execCallee));

        LLINT_CALL_RETURN(execCallee, execCallee, LLInt::getCodePtr(getHostCallReturnValue));
    }
    
#if LLINT_SLOW_PATH_TRACING
    dataLog("Constructor callee is not a function: ", callee, "\n");
#endif

    ASSERT(constructType == ConstructTypeNone);
    LLINT_CALL_THROW(exec, createNotAConstructorError(exec, callee));
}

inline SlowPathReturnType setUpCall(ExecState* execCallee, Instruction* pc, CodeSpecializationKind kind, JSValue calleeAsValue, LLIntCallLinkInfo* callLinkInfo = 0)
{
    ExecState* exec = execCallee->callerFrame();

#if LLINT_SLOW_PATH_TRACING
    dataLogF("Performing call with recorded PC = %p\n", exec->currentVPC());
#endif
    
    JSCell* calleeAsFunctionCell = getJSFunction(calleeAsValue);
    if (!calleeAsFunctionCell)
        return handleHostCall(execCallee, pc, calleeAsValue, kind);
    
    JSFunction* callee = jsCast<JSFunction*>(calleeAsFunctionCell);
    JSScope* scope = callee->scopeUnchecked();
    VM& vm = *scope->vm();
    ExecutableBase* executable = callee->executable();
    
    MacroAssemblerCodePtr codePtr;
    CodeBlock* codeBlock = 0;
    if (executable->isHostFunction())
        codePtr = executable->entrypointFor(vm, kind, MustCheckArity, RegisterPreservationNotRequired);
    else {
        FunctionExecutable* functionExecutable = static_cast<FunctionExecutable*>(executable);
        JSObject* error = functionExecutable->prepareForExecution(execCallee, callee, scope, kind);
        if (error)
            LLINT_CALL_THROW(exec, error);
        codeBlock = functionExecutable->codeBlockFor(kind);
        ASSERT(codeBlock);
        ArityCheckMode arity;
        if (execCallee->argumentCountIncludingThis() < static_cast<size_t>(codeBlock->numParameters()))
            arity = MustCheckArity;
        else
            arity = ArityCheckNotRequired;
        codePtr = functionExecutable->entrypointFor(vm, kind, arity, RegisterPreservationNotRequired);
    }
    
    ASSERT(!!codePtr);
    
    if (!LLINT_ALWAYS_ACCESS_SLOW && callLinkInfo) {
        CodeBlock* callerCodeBlock = exec->codeBlock();

        ConcurrentJITLocker locker(callerCodeBlock->m_lock);
        
        if (callLinkInfo->isOnList())
            callLinkInfo->remove();
        callLinkInfo->callee.set(vm, callerCodeBlock->ownerExecutable(), callee);
        callLinkInfo->lastSeenCallee.set(vm, callerCodeBlock->ownerExecutable(), callee);
        callLinkInfo->machineCodeTarget = codePtr;
        if (codeBlock)
            codeBlock->linkIncomingCall(exec, callLinkInfo);
    }

    LLINT_CALL_RETURN(exec, execCallee, codePtr.executableAddress());
}

inline SlowPathReturnType genericCall(ExecState* exec, Instruction* pc, CodeSpecializationKind kind)
{
    // This needs to:
    // - Set up a call frame.
    // - Figure out what to call and compile it if necessary.
    // - If possible, link the call's inline cache.
    // - Return a tuple of machine code address to call and the new call frame.
    
    JSValue calleeAsValue = LLINT_OP_C(2).jsValue();
    
    ExecState* execCallee = exec - pc[4].u.operand;
    
    execCallee->setArgumentCountIncludingThis(pc[3].u.operand);
    execCallee->uncheckedR(JSStack::Callee) = calleeAsValue;
    execCallee->setCallerFrame(exec);
    
    ASSERT(pc[5].u.callLinkInfo);
    return setUpCall(execCallee, pc, kind, calleeAsValue, pc[5].u.callLinkInfo);
}

LLINT_SLOW_PATH_DECL(slow_path_call)
{
    LLINT_BEGIN_NO_SET_PC();
    return genericCall(exec, pc, CodeForCall);
}

LLINT_SLOW_PATH_DECL(slow_path_construct)
{
    LLINT_BEGIN_NO_SET_PC();
    return genericCall(exec, pc, CodeForConstruct);
}

LLINT_SLOW_PATH_DECL(slow_path_size_frame_for_varargs)
{
    LLINT_BEGIN();
    // This needs to:
    // - Set up a call frame while respecting the variable arguments.
    
    unsigned numUsedStackSlots = -pc[5].u.operand;
    unsigned length = sizeFrameForVarargs(exec, &vm.interpreter->stack(),
        LLINT_OP_C(4).jsValue(), numUsedStackSlots, pc[6].u.operand);
    LLINT_CALL_CHECK_EXCEPTION(exec, exec);
    
    ExecState* execCallee = calleeFrameForVarargs(exec, numUsedStackSlots, length + 1);
    vm.varargsLength = length;
    vm.newCallFrameReturnValue = execCallee;

    LLINT_RETURN_CALLEE_FRAME(execCallee);
}

LLINT_SLOW_PATH_DECL(slow_path_call_varargs)
{
    LLINT_BEGIN_NO_SET_PC();
    // This needs to:
    // - Figure out what to call and compile it if necessary.
    // - Return a tuple of machine code address to call and the new call frame.
    
    JSValue calleeAsValue = LLINT_OP_C(2).jsValue();
    
    ExecState* execCallee = vm.newCallFrameReturnValue;

    setupVarargsFrameAndSetThis(exec, execCallee, LLINT_OP_C(3).jsValue(), LLINT_OP_C(4).jsValue(), pc[6].u.operand, vm.varargsLength);
    LLINT_CALL_CHECK_EXCEPTION(exec, exec);
    
    execCallee->uncheckedR(JSStack::Callee) = calleeAsValue;
    execCallee->setCallerFrame(exec);
    exec->setCurrentVPC(pc);
    
    return setUpCall(execCallee, pc, CodeForCall, calleeAsValue);
}
    
LLINT_SLOW_PATH_DECL(slow_path_construct_varargs)
{
    LLINT_BEGIN_NO_SET_PC();
    // This needs to:
    // - Figure out what to call and compile it if necessary.
    // - Return a tuple of machine code address to call and the new call frame.
    
    JSValue calleeAsValue = LLINT_OP_C(2).jsValue();
    
    ExecState* execCallee = vm.newCallFrameReturnValue;
    
    setupVarargsFrameAndSetThis(exec, execCallee, LLINT_OP_C(3).jsValue(), LLINT_OP_C(4).jsValue(), pc[6].u.operand, vm.varargsLength);
    LLINT_CALL_CHECK_EXCEPTION(exec, exec);
    
    execCallee->uncheckedR(JSStack::Callee) = calleeAsValue;
    execCallee->setCallerFrame(exec);
    exec->setCurrentVPC(pc);
    
    return setUpCall(execCallee, pc, CodeForConstruct, calleeAsValue);
}
    
LLINT_SLOW_PATH_DECL(slow_path_call_eval)
{
    LLINT_BEGIN_NO_SET_PC();
    JSValue calleeAsValue = LLINT_OP(2).jsValue();
    
    ExecState* execCallee = exec - pc[4].u.operand;
    
    execCallee->setArgumentCountIncludingThis(pc[3].u.operand);
    execCallee->setCallerFrame(exec);
    execCallee->uncheckedR(JSStack::Callee) = calleeAsValue;
    execCallee->setReturnPC(LLInt::getCodePtr(llint_generic_return_point));
    execCallee->setCodeBlock(0);
    exec->setCurrentVPC(pc);
    
    if (!isHostFunction(calleeAsValue, globalFuncEval))
        return setUpCall(execCallee, pc, CodeForCall, calleeAsValue);
    
    vm.hostCallReturnValue = eval(execCallee);
    LLINT_CALL_RETURN(exec, execCallee, LLInt::getCodePtr(getHostCallReturnValue));
}

LLINT_SLOW_PATH_DECL(slow_path_strcat)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsStringFromRegisterArray(exec, &LLINT_OP(2), pc[3].u.operand));
}

LLINT_SLOW_PATH_DECL(slow_path_to_primitive)
{
    LLINT_BEGIN();
    LLINT_RETURN(LLINT_OP_C(2).jsValue().toPrimitive(exec));
}

LLINT_SLOW_PATH_DECL(slow_path_push_with_scope)
{
    LLINT_BEGIN();
    JSValue v = LLINT_OP_C(2).jsValue();
    JSObject* o = v.toObject(exec);
    LLINT_CHECK_EXCEPTION();

    int scopeReg = pc[1].u.operand;
    JSScope* currentScope = exec->uncheckedR(scopeReg).Register::scope();
    exec->uncheckedR(scopeReg) = JSWithScope::create(exec, o, currentScope);
    
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_pop_scope)
{
    LLINT_BEGIN();
    int scopeReg = pc[1].u.operand;
    JSScope* scope = exec->uncheckedR(scopeReg).Register::scope();
    exec->uncheckedR(scopeReg) = scope->next();
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_push_name_scope)
{
    LLINT_BEGIN();
    int scopeReg = pc[1].u.operand;
    JSScope* currentScope = exec->uncheckedR(scopeReg).Register::scope();
    JSValue value = LLINT_OP_C(2).jsValue();
    SymbolTable* symbolTable = jsCast<SymbolTable*>(LLINT_OP_C(3).jsValue());
    JSNameScope::Type type = static_cast<JSNameScope::Type>(pc[4].u.operand);
    JSNameScope* scope = JSNameScope::create(vm, exec->lexicalGlobalObject(), currentScope, symbolTable, value, type);
    exec->uncheckedR(scopeReg) = scope;
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_throw)
{
    LLINT_BEGIN();
    LLINT_THROW(LLINT_OP_C(1).jsValue());
}

LLINT_SLOW_PATH_DECL(slow_path_throw_static_error)
{
    LLINT_BEGIN();
    JSValue errorMessageValue = LLINT_OP_C(1).jsValue();
    RELEASE_ASSERT(errorMessageValue.isString());
    String errorMessage = asString(errorMessageValue)->value(exec);
    if (pc[2].u.operand)
        LLINT_THROW(createReferenceError(exec, errorMessage));
    else
        LLINT_THROW(createTypeError(exec, errorMessage));
}

LLINT_SLOW_PATH_DECL(slow_path_handle_watchdog_timer)
{
    LLINT_BEGIN_NO_SET_PC();
    ASSERT(vm.watchdog);
    if (UNLIKELY(vm.watchdog->didFire(exec)))
        LLINT_THROW(createTerminatedExecutionException(&vm));
    LLINT_RETURN_TWO(0, exec);
}

LLINT_SLOW_PATH_DECL(slow_path_debug)
{
    LLINT_BEGIN();
    int debugHookID = pc[1].u.operand;
    vm.interpreter->debug(exec, static_cast<DebugHookID>(debugHookID));
    
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_profile_will_call)
{
    LLINT_BEGIN();
    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->willExecute(exec, LLINT_OP(1).jsValue());
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_profile_did_call)
{
    LLINT_BEGIN();
    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->didExecute(exec, LLINT_OP(1).jsValue());
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_handle_exception)
{
    LLINT_BEGIN_NO_SET_PC();
    ASSERT(vm.exception());
    genericUnwind(&vm, exec, vm.exception());
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(slow_path_resolve_scope)
{
    LLINT_BEGIN();
    const Identifier& ident = exec->codeBlock()->identifier(pc[3].u.operand);
    JSScope* scope = LLINT_OP(2).Register::scope();
    LLINT_RETURN(JSScope::resolve(exec, scope, ident));
}

LLINT_SLOW_PATH_DECL(slow_path_get_from_scope)
{
    LLINT_BEGIN();

    const Identifier& ident = exec->codeBlock()->identifier(pc[3].u.operand);
    JSObject* scope = jsCast<JSObject*>(LLINT_OP(2).jsValue());
    ResolveModeAndType modeAndType(pc[4].u.operand);

    PropertySlot slot(scope);
    if (!scope->getPropertySlot(exec, ident, slot)) {
        if (modeAndType.mode() == ThrowIfNotFound)
            LLINT_RETURN(exec->vm().throwException(exec, createUndefinedVariableError(exec, ident)));
        LLINT_RETURN(jsUndefined());
    }

    // Covers implicit globals. Since they don't exist until they first execute, we didn't know how to cache them at compile time.
    if (slot.isCacheableValue() && slot.slotBase() == scope && scope->structure()->propertyAccessesAreCacheable()) {
        if (modeAndType.type() == GlobalProperty || modeAndType.type() == GlobalPropertyWithVarInjectionChecks) {
            CodeBlock* codeBlock = exec->codeBlock();
            Structure* structure = scope->structure(vm);
            {
                ConcurrentJITLocker locker(codeBlock->m_lock);
                pc[5].u.structure.set(exec->vm(), codeBlock->ownerExecutable(), structure);
                pc[6].u.operand = slot.cachedOffset();
            }
            structure->startWatchingPropertyForReplacements(vm, slot.cachedOffset());
        }
    }

    LLINT_RETURN(slot.getValue(exec, ident));
}

LLINT_SLOW_PATH_DECL(slow_path_put_to_scope)
{
    LLINT_BEGIN();

    CodeBlock* codeBlock = exec->codeBlock();
    const Identifier& ident = codeBlock->identifier(pc[2].u.operand);
    JSObject* scope = jsCast<JSObject*>(LLINT_OP(1).jsValue());
    JSValue value = LLINT_OP_C(3).jsValue();
    ResolveModeAndType modeAndType = ResolveModeAndType(pc[4].u.operand);
    if (modeAndType.type() == LocalClosureVar) {
        JSLexicalEnvironment* environment = jsCast<JSLexicalEnvironment*>(scope);
        environment->variableAt(ScopeOffset(pc[6].u.operand)).set(vm, environment, value);
        if (VariableWatchpointSet* set = pc[5].u.watchpointSet)
            set->notifyWrite(vm, value, "Executed op_put_scope<LocalClosureVar>");
        LLINT_END();
    }

    if (modeAndType.mode() == ThrowIfNotFound && !scope->hasProperty(exec, ident))
        LLINT_THROW(createUndefinedVariableError(exec, ident));

    PutPropertySlot slot(scope, codeBlock->isStrictMode());
    scope->methodTable()->put(scope, exec, ident, value, slot);
    
    CommonSlowPaths::tryCachePutToScopeGlobal(exec, codeBlock, pc, scope, modeAndType, slot);

    LLINT_END();
}

extern "C" SlowPathReturnType llint_throw_stack_overflow_error(VM* vm, ProtoCallFrame* protoFrame)
{
    ExecState* exec = vm->topCallFrame;
    if (!exec)
        exec = protoFrame->callee()->globalObject()->globalExec();
    throwStackOverflowError(exec);
    return encodeResult(0, 0);
}

#if !ENABLE(JIT)
extern "C" SlowPathReturnType llint_stack_check_at_vm_entry(VM* vm, Register* newTopOfStack)
{
    bool success = vm->interpreter->stack().ensureCapacityFor(newTopOfStack);
    return encodeResult(reinterpret_cast<void*>(success), 0);
}
#endif

extern "C" void llint_write_barrier_slow(ExecState* exec, JSCell* cell)
{
    VM& vm = exec->vm();
    vm.heap.writeBarrier(cell);
}

extern "C" NO_RETURN_DUE_TO_CRASH void llint_crash()
{
    CRASH();
}

} } // namespace JSC::LLInt
