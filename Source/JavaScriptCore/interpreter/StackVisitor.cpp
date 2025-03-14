/*
 * Copyright (C) 2013, 2015 Apple Inc. All rights reserved.
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
#include "StackVisitor.h"

#include "CallFrameInlines.h"
#include "ClonedArguments.h"
#include "Executable.h"
#include "Interpreter.h"
#include "JSCInlines.h"
#include <wtf/DataLog.h>

namespace JSC {

StackVisitor::StackVisitor(CallFrame* startFrame)
{
    m_frame.m_index = 0;
    CallFrame* topFrame;
    if (startFrame) {
        m_frame.m_VMEntryFrame = startFrame->vm().topVMEntryFrame;
        topFrame = startFrame->vm().topCallFrame;
    } else {
        m_frame.m_VMEntryFrame = 0;
        topFrame = 0;
    }
    m_frame.m_callerIsVMEntryFrame = false;
    readFrame(topFrame);

    // Find the frame the caller wants to start unwinding from.
    while (m_frame.callFrame() && m_frame.callFrame() != startFrame)
        gotoNextFrame();
}

void StackVisitor::gotoNextFrame()
{
#if ENABLE(DFG_JIT)
    if (m_frame.isInlinedFrame()) {
        InlineCallFrame* inlineCallFrame = m_frame.inlineCallFrame();
        CodeOrigin* callerCodeOrigin = &inlineCallFrame->caller;
        readInlinedFrame(m_frame.callFrame(), callerCodeOrigin);
        return;
    }
#endif // ENABLE(DFG_JIT)
    m_frame.m_VMEntryFrame = m_frame.m_CallerVMEntryFrame;
    readFrame(m_frame.callerFrame());
}

void StackVisitor::readFrame(CallFrame* callFrame)
{
    if (!callFrame) {
        m_frame.setToEnd();
        return;
    }

#if !ENABLE(DFG_JIT)
    readNonInlinedFrame(callFrame);

#else // !ENABLE(DFG_JIT)
    // If the frame doesn't have a code block, then it's not a DFG frame.
    // Hence, we're not at an inlined frame.
    CodeBlock* codeBlock = callFrame->codeBlock();
    if (!codeBlock) {
        readNonInlinedFrame(callFrame);
        return;
    }

    // If the code block does not have any code origins, then there's no
    // inlining. Hence, we're not at an inlined frame.
    if (!codeBlock->hasCodeOrigins()) {
        readNonInlinedFrame(callFrame);
        return;
    }

    unsigned index = callFrame->locationAsCodeOriginIndex();
    ASSERT(codeBlock->canGetCodeOrigin(index));
    if (!codeBlock->canGetCodeOrigin(index)) {
        // See assertion above. In release builds, we try to protect ourselves
        // from crashing even though stack walking will be goofed up.
        m_frame.setToEnd();
        return;
    }

    CodeOrigin codeOrigin = codeBlock->codeOrigin(index);
    if (!codeOrigin.inlineCallFrame) {
        readNonInlinedFrame(callFrame, &codeOrigin);
        return;
    }

    readInlinedFrame(callFrame, &codeOrigin);
#endif // !ENABLE(DFG_JIT)
}

void StackVisitor::readNonInlinedFrame(CallFrame* callFrame, CodeOrigin* codeOrigin)
{
    m_frame.m_callFrame = callFrame;
    m_frame.m_argumentCountIncludingThis = callFrame->argumentCountIncludingThis();
    m_frame.m_CallerVMEntryFrame = m_frame.m_VMEntryFrame;
    m_frame.m_callerFrame = callFrame->callerFrame(m_frame.m_CallerVMEntryFrame);
    m_frame.m_callerIsVMEntryFrame = m_frame.m_CallerVMEntryFrame != m_frame.m_VMEntryFrame;
    m_frame.m_callee = callFrame->callee();
    m_frame.m_codeBlock = callFrame->codeBlock();
    m_frame.m_bytecodeOffset = !m_frame.codeBlock() ? 0
        : codeOrigin ? codeOrigin->bytecodeIndex
        : callFrame->locationAsBytecodeOffset();
#if ENABLE(DFG_JIT)
    m_frame.m_inlineCallFrame = 0;
#endif
}

#if ENABLE(DFG_JIT)
static int inlinedFrameOffset(CodeOrigin* codeOrigin)
{
    InlineCallFrame* inlineCallFrame = codeOrigin->inlineCallFrame;
    int frameOffset = inlineCallFrame ? inlineCallFrame->stackOffset : 0;
    return frameOffset;
}

void StackVisitor::readInlinedFrame(CallFrame* callFrame, CodeOrigin* codeOrigin)
{
    ASSERT(codeOrigin);

    int frameOffset = inlinedFrameOffset(codeOrigin);
    bool isInlined = !!frameOffset;
    if (isInlined) {
        InlineCallFrame* inlineCallFrame = codeOrigin->inlineCallFrame;

        m_frame.m_callFrame = callFrame;
        m_frame.m_inlineCallFrame = inlineCallFrame;
        if (inlineCallFrame->argumentCountRegister.isValid())
            m_frame.m_argumentCountIncludingThis = callFrame->r(inlineCallFrame->argumentCountRegister.offset()).unboxedInt32();
        else
            m_frame.m_argumentCountIncludingThis = inlineCallFrame->arguments.size();
        m_frame.m_codeBlock = inlineCallFrame->baselineCodeBlock();
        m_frame.m_bytecodeOffset = codeOrigin->bytecodeIndex;

        JSFunction* callee = inlineCallFrame->calleeForCallFrame(callFrame);
        m_frame.m_callee = callee;
        ASSERT(m_frame.callee());

        // The callerFrame just needs to be non-null to indicate that we
        // haven't reached the last frame yet. Setting it to the root
        // frame (i.e. the callFrame that this inlined frame is called from)
        // would work just fine.
        m_frame.m_callerFrame = callFrame;
        return;
    }

    readNonInlinedFrame(callFrame, codeOrigin);
}
#endif // ENABLE(DFG_JIT)

StackVisitor::Frame::CodeType StackVisitor::Frame::codeType() const
{
    if (!isJSFrame())
        return CodeType::Native;

    switch (codeBlock()->codeType()) {
    case EvalCode:
        return CodeType::Eval;
    case FunctionCode:
        return CodeType::Function;
    case GlobalCode:
        return CodeType::Global;
    }
    RELEASE_ASSERT_NOT_REACHED();
    return CodeType::Global;
}

String StackVisitor::Frame::functionName()
{
    String traceLine;
    JSObject* callee = this->callee();

    switch (codeType()) {
    case CodeType::Eval:
        traceLine = ASCIILiteral("eval code");
        break;
    case CodeType::Native:
        if (callee)
            traceLine = getCalculatedDisplayName(callFrame(), callee).impl();
        break;
    case CodeType::Function:
        traceLine = getCalculatedDisplayName(callFrame(), callee).impl();
        break;
    case CodeType::Global:
        traceLine = ASCIILiteral("global code");
        break;
    }
    return traceLine.isNull() ? emptyString() : traceLine;
}

String StackVisitor::Frame::sourceURL()
{
    String traceLine;

    switch (codeType()) {
    case CodeType::Eval:
    case CodeType::Function:
    case CodeType::Global: {
        String sourceURL = codeBlock()->ownerExecutable()->sourceURL();
        if (!sourceURL.isEmpty())
            traceLine = sourceURL.impl();
        break;
    }
    case CodeType::Native:
        traceLine = ASCIILiteral("[native code]");
        break;
    }
    return traceLine.isNull() ? emptyString() : traceLine;
}

String StackVisitor::Frame::toString()
{
    StringBuilder traceBuild;
    String functionName = this->functionName();
    String sourceURL = this->sourceURL();
    traceBuild.append(functionName);
    if (!sourceURL.isEmpty()) {
        if (!functionName.isEmpty())
            traceBuild.append('@');
        traceBuild.append(sourceURL);
        if (isJSFrame()) {
            unsigned line = 0;
            unsigned column = 0;
            computeLineAndColumn(line, column);
            traceBuild.append(':');
            traceBuild.appendNumber(line);
            traceBuild.append(':');
            traceBuild.appendNumber(column);
        }
    }
    return traceBuild.toString().impl();
}

ClonedArguments* StackVisitor::Frame::createArguments()
{
    ASSERT(m_callFrame);
    CallFrame* physicalFrame = m_callFrame;
    ClonedArguments* arguments;
    ArgumentsMode mode;
    if (Options::enableFunctionDotArguments())
        mode = ArgumentsMode::Cloned;
    else
        mode = ArgumentsMode::FakeValues;
#if ENABLE(DFG_JIT)
    if (isInlinedFrame()) {
        ASSERT(m_inlineCallFrame);
        arguments = ClonedArguments::createWithInlineFrame(physicalFrame, physicalFrame, m_inlineCallFrame, mode);
    } else 
#endif
        arguments = ClonedArguments::createWithMachineFrame(physicalFrame, physicalFrame, mode);
    return arguments;
}

void StackVisitor::Frame::computeLineAndColumn(unsigned& line, unsigned& column)
{
    CodeBlock* codeBlock = this->codeBlock();
    if (!codeBlock) {
        line = 0;
        column = 0;
        return;
    }

    int divot = 0;
    int unusedStartOffset = 0;
    int unusedEndOffset = 0;
    unsigned divotLine = 0;
    unsigned divotColumn = 0;
    retrieveExpressionInfo(divot, unusedStartOffset, unusedEndOffset, divotLine, divotColumn);

    line = divotLine + codeBlock->ownerExecutable()->firstLine();
    column = divotColumn + (divotLine ? 1 : codeBlock->firstLineColumnOffset());

    if (codeBlock->ownerExecutable()->hasOverrideLineNumber())
        line = codeBlock->ownerExecutable()->overrideLineNumber();
}

void StackVisitor::Frame::retrieveExpressionInfo(int& divot, int& startOffset, int& endOffset, unsigned& line, unsigned& column)
{
    CodeBlock* codeBlock = this->codeBlock();
    codeBlock->unlinkedCodeBlock()->expressionRangeForBytecodeOffset(bytecodeOffset(), divot, startOffset, endOffset, line, column);
    divot += codeBlock->sourceOffset();
}

void StackVisitor::Frame::setToEnd()
{
    m_callFrame = 0;
#if ENABLE(DFG_JIT)
    m_inlineCallFrame = 0;
#endif
}

#ifndef NDEBUG

static const char* jitTypeName(JITCode::JITType jitType)
{
    switch (jitType) {
    case JITCode::None: return "None";
    case JITCode::HostCallThunk: return "HostCallThunk";
    case JITCode::InterpreterThunk: return "InterpreterThunk";
    case JITCode::BaselineJIT: return "BaselineJIT";
    case JITCode::DFGJIT: return "DFGJIT";
    case JITCode::FTLJIT: return "FTLJIT";
    }
    return "<unknown>";
}

static void printIndents(int levels)
{
    while (levels--)
        dataLogFString("   ");
}

static void printif(int indentLevels, const char* format, ...)
{
    va_list argList;
    va_start(argList, format);

    if (indentLevels)
        printIndents(indentLevels);

#if COMPILER(CLANG) || COMPILER(GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wmissing-format-attribute"
#endif

    WTF::dataLogFV(format, argList);

#if COMPILER(CLANG) || COMPILER(GCC)
#pragma GCC diagnostic pop
#endif

    va_end(argList);
}

void StackVisitor::Frame::print(int indentLevel)
{
    int i = indentLevel;

    if (!this->callFrame()) {
        printif(i, "frame 0x0\n");
        return;
    }

    CodeBlock* codeBlock = this->codeBlock();
    printif(i, "frame %p {\n", this->callFrame());

    CallFrame* callFrame = m_callFrame;
    CallFrame* callerFrame = this->callerFrame();
    void* returnPC = callFrame->hasReturnPC() ? callFrame->returnPC().value() : nullptr;

    printif(i, "   name '%s'\n", functionName().utf8().data());
    printif(i, "   sourceURL '%s'\n", sourceURL().utf8().data());

#if ENABLE(DFG_JIT)
    printif(i, "   isInlinedFrame %d\n", isInlinedFrame());
    if (isInlinedFrame())
        printif(i, "   InlineCallFrame %p\n", m_inlineCallFrame);
#endif

    printif(i, "   callee %p\n", callee());
    printif(i, "   returnPC %p\n", returnPC);
    printif(i, "   callerFrame %p\n", callerFrame);
    unsigned locationRawBits = callFrame->locationAsRawBits();
    printif(i, "   rawLocationBits %u 0x%x\n", locationRawBits, locationRawBits);
    printif(i, "   codeBlock %p\n", codeBlock);
    if (codeBlock) {
        JITCode::JITType jitType = codeBlock->jitType();
        if (callFrame->hasLocationAsBytecodeOffset()) {
            unsigned bytecodeOffset = callFrame->locationAsBytecodeOffset();
            printif(i, "      bytecodeOffset %u %p / %zu\n", bytecodeOffset, reinterpret_cast<void*>(bytecodeOffset), codeBlock->instructions().size());
#if ENABLE(DFG_JIT)
        } else {
            unsigned codeOriginIndex = callFrame->locationAsCodeOriginIndex();
            printif(i, "      codeOriginIdex %u %p / %zu\n", codeOriginIndex, reinterpret_cast<void*>(codeOriginIndex), codeBlock->codeOrigins().size());
#endif
        }
        unsigned line = 0;
        unsigned column = 0;
        computeLineAndColumn(line, column);
        printif(i, "      line %d\n", line);
        printif(i, "      column %d\n", column);
        printif(i, "      jitType %d <%s> isOptimizingJIT %d\n", jitType, jitTypeName(jitType), JITCode::isOptimizingJIT(jitType));
#if ENABLE(DFG_JIT)
        printif(i, "      hasCodeOrigins %d\n", codeBlock->hasCodeOrigins());
        if (codeBlock->hasCodeOrigins()) {
            JITCode* jitCode = codeBlock->jitCode().get();
            printif(i, "         jitCode %p start %p end %p\n", jitCode, jitCode->start(), jitCode->end());
        }
#endif
    }
    printif(i, "}\n");
}

#endif // NDEBUG

} // namespace JSC

#ifndef NDEBUG
using JSC::StackVisitor;

// For debugging use
JS_EXPORT_PRIVATE void debugPrintCallFrame(JSC::CallFrame*);
JS_EXPORT_PRIVATE void debugPrintStack(JSC::CallFrame* topCallFrame);

class DebugPrintFrameFunctor {
public:
    enum Action {
        PrintOne,
        PrintAll
    };

    DebugPrintFrameFunctor(Action action)
        : m_action(action)
    {
    }

    StackVisitor::Status operator()(StackVisitor& visitor)
    {
        visitor->print(2);
        return m_action == PrintAll ? StackVisitor::Continue : StackVisitor::Done;
    }

private:
    Action m_action;
};

void debugPrintCallFrame(JSC::CallFrame* callFrame)
{
    if (!callFrame)
        return;
    DebugPrintFrameFunctor functor(DebugPrintFrameFunctor::PrintOne);
    callFrame->iterate(functor);
}

void debugPrintStack(JSC::CallFrame* topCallFrame)
{
    if (!topCallFrame)
        return;
    DebugPrintFrameFunctor functor(DebugPrintFrameFunctor::PrintAll);
    topCallFrame->iterate(functor);
}

#endif // !NDEBUG
