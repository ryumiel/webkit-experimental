/*
 * Copyright (C) 2008, 2013, 2015 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Interpreter_h
#define Interpreter_h

#include "ArgList.h"
#include "JSCJSValue.h"
#include "JSCell.h"
#include "JSFunction.h"
#include "JSObject.h"
#include "JSStack.h"
#include "LLIntData.h"
#include "Opcode.h"
#include "SourceProvider.h"
#include "StackAlignment.h"

#include <wtf/HashMap.h>
#include <wtf/text/StringBuilder.h>

namespace JSC {

    class CodeBlock;
    class EvalExecutable;
    class ExecutableBase;
    class FunctionExecutable;
    class VM;
    class JSGlobalObject;
    class LLIntOffsetsExtractor;
    class ProgramExecutable;
    class Register;
    class JSScope;
    class SamplingTool;
    struct CallFrameClosure;
    struct HandlerInfo;
    struct Instruction;
    struct ProtoCallFrame;

    enum DebugHookID {
        WillExecuteProgram,
        DidExecuteProgram,
        DidEnterCallFrame,
        DidReachBreakpoint,
        WillLeaveCallFrame,
        WillExecuteStatement
    };

    enum StackFrameCodeType {
        StackFrameGlobalCode,
        StackFrameEvalCode,
        StackFrameFunctionCode,
        StackFrameNativeCode
    };

    struct StackFrame {
        Strong<JSObject> callee;
        StackFrameCodeType codeType;
        Strong<ScriptExecutable> executable;
        Strong<UnlinkedCodeBlock> codeBlock;
        RefPtr<SourceProvider> code;
        int lineOffset;
        unsigned firstLineColumnOffset;
        unsigned characterOffset;
        unsigned bytecodeOffset;
        String sourceURL;
        JS_EXPORT_PRIVATE String toString(CallFrame*);
        String friendlySourceURL() const
        {
            String traceLine;

            switch (codeType) {
            case StackFrameEvalCode:
            case StackFrameFunctionCode:
            case StackFrameGlobalCode:
                if (!sourceURL.isEmpty())
                    traceLine = sourceURL.impl();
                break;
            case StackFrameNativeCode:
                traceLine = "[native code]";
                break;
            }
            return traceLine.isNull() ? emptyString() : traceLine;
        }
        String friendlyFunctionName(CallFrame* callFrame) const
        {
            String traceLine;
            JSObject* stackFrameCallee = callee.get();

            switch (codeType) {
            case StackFrameEvalCode:
                traceLine = "eval code";
                break;
            case StackFrameNativeCode:
                if (callee)
                    traceLine = getCalculatedDisplayName(callFrame, stackFrameCallee).impl();
                break;
            case StackFrameFunctionCode:
                traceLine = getCalculatedDisplayName(callFrame, stackFrameCallee).impl();
                break;
            case StackFrameGlobalCode:
                traceLine = "global code";
                break;
            }
            return traceLine.isNull() ? emptyString() : traceLine;
        }
        JS_EXPORT_PRIVATE void computeLineAndColumn(unsigned& line, unsigned& column);

    private:
        void expressionInfo(int& divot, int& startOffset, int& endOffset, unsigned& line, unsigned& column);
    };

    class ClearExceptionScope {
    public:
        ClearExceptionScope(VM* vm): m_vm(vm)
        {
            vm->getExceptionInfo(oldException, oldExceptionStack);
            vm->clearException();
        }
        ~ClearExceptionScope()
        {
            m_vm->setExceptionInfo(oldException, oldExceptionStack);
        }
    private:
        JSC::JSValue oldException;
        RefCountedArray<JSC::StackFrame> oldExceptionStack;
        VM* m_vm;
    };
    
    class TopCallFrameSetter {
    public:
        TopCallFrameSetter(VM& currentVM, CallFrame* callFrame)
            : vm(currentVM)
            , oldCallFrame(currentVM.topCallFrame) 
        {
            currentVM.topCallFrame = callFrame;
        }
        
        ~TopCallFrameSetter() 
        {
            vm.topCallFrame = oldCallFrame;
        }
    private:
        VM& vm;
        CallFrame* oldCallFrame;
    };
    
    class NativeCallFrameTracer {
    public:
        ALWAYS_INLINE NativeCallFrameTracer(VM* vm, CallFrame* callFrame)
        {
            ASSERT(vm);
            ASSERT(callFrame);
            vm->topCallFrame = callFrame;
        }
    };

    class NativeCallFrameTracerWithRestore {
    public:
        ALWAYS_INLINE NativeCallFrameTracerWithRestore(VM* vm, VMEntryFrame* vmEntryFrame, CallFrame* callFrame)
            : m_vm(vm)
        {
            ASSERT(vm);
            ASSERT(callFrame);
            m_savedTopVMEntryFrame = vm->topVMEntryFrame;
            m_savedTopCallFrame = vm->topCallFrame;
            vm->topVMEntryFrame = vmEntryFrame;
            vm->topCallFrame = callFrame;
        }

        ALWAYS_INLINE ~NativeCallFrameTracerWithRestore()
        {
            m_vm->topVMEntryFrame = m_savedTopVMEntryFrame;
            m_vm->topCallFrame = m_savedTopCallFrame;
        }

    private:
        VM* m_vm;
        VMEntryFrame* m_savedTopVMEntryFrame;
        CallFrame* m_savedTopCallFrame;
    };

    class Interpreter {
        WTF_MAKE_FAST_ALLOCATED;
        friend class CachedCall;
        friend class LLIntOffsetsExtractor;
        friend class JIT;
        friend class VM;

    public:
        Interpreter(VM &);
        ~Interpreter();
        
        void initialize(bool canUseJIT);

        JSStack& stack() { return m_stack; }
        
        Opcode getOpcode(OpcodeID id)
        {
            ASSERT(m_initialized);
#if ENABLE(COMPUTED_GOTO_OPCODES)
            return m_opcodeTable[id];
#else
            return id;
#endif
        }

        OpcodeID getOpcodeID(Opcode opcode)
        {
            ASSERT(m_initialized);
#if ENABLE(COMPUTED_GOTO_OPCODES)
            ASSERT(isOpcode(opcode));
            return m_opcodeIDTable.get(opcode);
#else
            return opcode;
#endif
        }
        
        bool isOpcode(Opcode);

        JSValue execute(ProgramExecutable*, CallFrame*, JSObject* thisObj);
        JSValue executeCall(CallFrame*, JSObject* function, CallType, const CallData&, JSValue thisValue, const ArgList&);
        JSObject* executeConstruct(CallFrame*, JSObject* function, ConstructType, const ConstructData&, const ArgList&);
        JSValue execute(EvalExecutable*, CallFrame*, JSValue thisValue, JSScope*);

        void getArgumentsData(CallFrame*, JSFunction*&, ptrdiff_t& firstParameterIndex, Register*& argv, int& argc);
        
        SamplingTool* sampler() { return m_sampler.get(); }

        NEVER_INLINE HandlerInfo* unwind(VMEntryFrame*&, CallFrame*&, JSValue&);
        NEVER_INLINE void debug(CallFrame*, DebugHookID);
        JSString* stackTraceAsString(ExecState*, Vector<StackFrame>);

        static EncodedJSValue JSC_HOST_CALL constructWithErrorConstructor(ExecState*);
        static EncodedJSValue JSC_HOST_CALL callErrorConstructor(ExecState*);
        static EncodedJSValue JSC_HOST_CALL constructWithNativeErrorConstructor(ExecState*);
        static EncodedJSValue JSC_HOST_CALL callNativeErrorConstructor(ExecState*);

        void dumpSampleData(ExecState* exec);
        void startSampling();
        void stopSampling();

        JS_EXPORT_PRIVATE void dumpCallFrame(CallFrame*);

    private:
        enum ExecutionFlag { Normal, InitializeAndReturn };

        CallFrameClosure prepareForRepeatCall(FunctionExecutable*, CallFrame*, ProtoCallFrame*, JSFunction*, int argumentCountIncludingThis, JSScope*, JSValue*);

        JSValue execute(CallFrameClosure&);

        void getStackTrace(Vector<StackFrame>& results, size_t maxStackSize = std::numeric_limits<size_t>::max());

        void dumpRegisters(CallFrame*);
        
        bool isCallBytecode(Opcode opcode) { return opcode == getOpcode(op_call) || opcode == getOpcode(op_construct) || opcode == getOpcode(op_call_eval); }

        void enableSampler();
        int m_sampleEntryDepth;
        std::unique_ptr<SamplingTool> m_sampler;

        VM& m_vm;
        JSStack m_stack;
        int m_errorHandlingModeReentry;
        
#if ENABLE(COMPUTED_GOTO_OPCODES)
        Opcode* m_opcodeTable; // Maps OpcodeID => Opcode for compiling
        HashMap<Opcode, OpcodeID> m_opcodeIDTable; // Maps Opcode => OpcodeID for decompiling
#endif

#if !ASSERT_DISABLED
        bool m_initialized;
#endif
    };

    JSValue eval(CallFrame*);

    inline CallFrame* calleeFrameForVarargs(CallFrame* callFrame, unsigned numUsedStackSlots, unsigned argumentCountIncludingThis)
    {
        unsigned paddedCalleeFrameOffset = WTF::roundUpToMultipleOf(
            stackAlignmentRegisters(),
            numUsedStackSlots + argumentCountIncludingThis + JSStack::CallFrameHeaderSize);
        return CallFrame::create(callFrame->registers() - paddedCalleeFrameOffset);
    }

    unsigned sizeOfVarargs(CallFrame* exec, JSValue arguments, uint32_t firstVarArgOffset);
    static const unsigned maxArguments = 0x10000;
    unsigned sizeFrameForVarargs(CallFrame* exec, JSStack*, JSValue arguments, unsigned numUsedStackSlots, uint32_t firstVarArgOffset);
    void loadVarargs(CallFrame* execCaller, VirtualRegister firstElementDest, JSValue source, uint32_t offset, uint32_t length);
    void setupVarargsFrame(CallFrame* execCaller, CallFrame* execCallee, JSValue arguments, uint32_t firstVarArgOffset, uint32_t length);
    void setupVarargsFrameAndSetThis(CallFrame* execCaller, CallFrame* execCallee, JSValue thisValue, JSValue arguments, uint32_t firstVarArgOffset, uint32_t length);
    
} // namespace JSC

#endif // Interpreter_h
