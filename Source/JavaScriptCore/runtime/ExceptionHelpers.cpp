/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#include "config.h"
#include "ExceptionHelpers.h"

#include "CodeBlock.h"
#include "CallFrame.h"
#include "ErrorHandlingScope.h"
#include "JSGlobalObjectFunctions.h"
#include "JSNotAnObject.h"
#include "Interpreter.h"
#include "Nodes.h"
#include "JSCInlines.h"
#include "RuntimeType.h"
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringView.h>

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(TerminatedExecutionError);

const ClassInfo TerminatedExecutionError::s_info = { "TerminatedExecutionError", &Base::s_info, 0, CREATE_METHOD_TABLE(TerminatedExecutionError) };

JSValue TerminatedExecutionError::defaultValue(const JSObject*, ExecState* exec, PreferredPrimitiveType hint)
{
    if (hint == PreferString)
        return jsNontrivialString(exec, String(ASCIILiteral("JavaScript execution terminated.")));
    return JSValue(PNaN);
}

JSObject* createTerminatedExecutionException(VM* vm)
{
    return TerminatedExecutionError::create(*vm);
}

bool isTerminatedExecutionException(JSObject* object)
{
    return object->inherits(TerminatedExecutionError::info());
}

bool isTerminatedExecutionException(JSValue value)
{
    return value.inherits(TerminatedExecutionError::info());
}


JSObject* createStackOverflowError(ExecState* exec)
{
    return createRangeError(exec, ASCIILiteral("Maximum call stack size exceeded."));
}

JSObject* createStackOverflowError(JSGlobalObject* globalObject)
{
    return createRangeError(globalObject, ASCIILiteral("Maximum call stack size exceeded."));
}

JSObject* createUndefinedVariableError(ExecState* exec, const Identifier& ident)
{
    if (ident.impl()->isSymbol()) {
        String message(makeString("Can't find private variable: @", exec->propertyNames().getPublicName(ident).string()));
        return createReferenceError(exec, message);
    }
    String message(makeString("Can't find variable: ", ident.string()));
    return createReferenceError(exec, message);
}
    
JSString* errorDescriptionForValue(ExecState* exec, JSValue v)
{
    VM& vm = exec->vm();
    if (v.isNull())
        return vm.smallStrings.nullString();
    if (v.isUndefined())
        return vm.smallStrings.undefinedString();
    if (v.isInt32())
        return jsString(&vm, vm.numericStrings.add(v.asInt32()));
    if (v.isDouble())
        return jsString(&vm, vm.numericStrings.add(v.asDouble()));
    if (v.isTrue())
        return vm.smallStrings.trueString();
    if (v.isFalse())
        return vm.smallStrings.falseString();
    if (v.isString())
        return jsString(&vm, makeString('"',  asString(v)->value(exec), '"'));
    if (v.isObject()) {
        CallData callData;
        JSObject* object = asObject(v);
        if (object->methodTable()->getCallData(object, callData) != CallTypeNone)
            return vm.smallStrings.functionString();
        return jsString(exec, JSObject::calculatedClassName(object));
    }
    
    // The JSValue should never be empty, so this point in the code should never be reached.
    ASSERT_NOT_REACHED();
    return vm.smallStrings.emptyString();
}
    
static String defaultApproximateSourceError(const String& originalMessage, const String& sourceText)
{
    return makeString(originalMessage, " (near '...", sourceText, "...')");
}

static String defaultSourceAppender(const String& originalMessage, const String& sourceText, RuntimeType, ErrorInstance::SourceTextWhereErrorOccurred occurrence)
{
    if (occurrence == ErrorInstance::FoundApproximateSource)
        return defaultApproximateSourceError(originalMessage, sourceText);

    ASSERT(occurrence == ErrorInstance::FoundExactSource);
    return makeString(originalMessage, " (evaluating '", sourceText, "')");
}

static String functionCallBase(const String& sourceText)
{ 
    // This function retrieves the 'foo.bar' substring from 'foo.bar(baz)'.
    unsigned idx = sourceText.length() - 1;
    if (sourceText[idx] != ')') {
        // For function calls that have many new lines in between their open parenthesis
        // and their closing parenthesis, the text range passed into the message appender 
        // will not inlcude the text in between these parentheses, it will just be the desired
        // text that precedes the parentheses.
        return sourceText;
    }

    unsigned parenStack = 1;
    bool isInMultiLineComment = false;
    idx -= 1;
    // Note that we're scanning text right to left instead of the more common left to right, 
    // so syntax detection is backwards.
    while (parenStack > 0) {
        UChar curChar = sourceText[idx];
        if (isInMultiLineComment)  {
            if (curChar == '*' && sourceText[idx - 1] == '/') {
                isInMultiLineComment = false;
                idx -= 1;
            }
        } else if (curChar == '(')
            parenStack -= 1;
        else if (curChar == ')')
            parenStack += 1;
        else if (curChar == '/' && sourceText[idx - 1] == '*') {
            isInMultiLineComment = true;
            idx -= 1;
        }

        idx -= 1;
    }

    return sourceText.left(idx + 1);
}

static String notAFunctionSourceAppender(const String& originalMessage, const String& sourceText, RuntimeType type, ErrorInstance::SourceTextWhereErrorOccurred occurrence)
{
    ASSERT(type != TypeFunction);

    if (occurrence == ErrorInstance::FoundApproximateSource)
        return defaultApproximateSourceError(originalMessage, sourceText);

    ASSERT(occurrence == ErrorInstance::FoundExactSource);
    auto notAFunctionIndex = originalMessage.reverseFind("is not a function");
    RELEASE_ASSERT(notAFunctionIndex != notFound);
    StringView displayValue;
    if (originalMessage.is8Bit()) 
        displayValue = StringView(originalMessage.characters8(), notAFunctionIndex - 1);
    else
        displayValue = StringView(originalMessage.characters16(), notAFunctionIndex - 1);

    String base = functionCallBase(sourceText);
    StringBuilder builder;
    builder.append(base);
    builder.appendLiteral(" is not a function. (In '");
    builder.append(sourceText);
    builder.appendLiteral("', '");
    builder.append(base);
    builder.appendLiteral("' is ");
    if (type == TypeObject)
        builder.appendLiteral("an instance of ");
    builder.append(displayValue);
    builder.appendLiteral(")");

    return builder.toString();
}

static String invalidParameterInSourceAppender(const String& originalMessage, const String& sourceText, RuntimeType type, ErrorInstance::SourceTextWhereErrorOccurred occurrence)
{
    ASSERT_UNUSED(type, type != TypeObject);

    if (occurrence == ErrorInstance::FoundApproximateSource)
        return defaultApproximateSourceError(originalMessage, sourceText);

    ASSERT(occurrence == ErrorInstance::FoundExactSource);
    auto inIndex = sourceText.reverseFind("in");
    RELEASE_ASSERT(inIndex != notFound);
    if (sourceText.find("in") != inIndex)
        return makeString(originalMessage, " (evaluating '", sourceText, "')");

    static const unsigned inLength = 2;
    String rightHandSide = sourceText.substring(inIndex + inLength).simplifyWhiteSpace();
    return makeString(rightHandSide, " is not an Object. (evaluating '", sourceText, "')");
}

static String invalidParameterInstanceofSourceAppender(const String& originalMessage, const String& sourceText, RuntimeType, ErrorInstance::SourceTextWhereErrorOccurred occurrence)
{
    if (occurrence == ErrorInstance::FoundApproximateSource)
        return defaultApproximateSourceError(originalMessage, sourceText);

    ASSERT(occurrence == ErrorInstance::FoundExactSource);
    auto instanceofIndex = sourceText.reverseFind("instanceof");
    RELEASE_ASSERT(instanceofIndex != notFound);
    if (sourceText.find("instanceof") != instanceofIndex)
        return makeString(originalMessage, " (evaluating '", sourceText, "')");

    static const unsigned instanceofLength = 10;
    String rightHandSide = sourceText.substring(instanceofIndex + instanceofLength).simplifyWhiteSpace();
    return makeString(rightHandSide, " is not a function. (evaluating '", sourceText, "')");
}

JSObject* createError(ExecState* exec, ErrorFactory errorFactory, JSValue value, const String& message, ErrorInstance::SourceAppender appender)
{
    String errorMessage = makeString(errorDescriptionForValue(exec, value)->value(exec), ' ', message);
    JSObject* exception = errorFactory(exec, errorMessage);
    ASSERT(exception->isErrorInstance());
    static_cast<ErrorInstance*>(exception)->setSourceAppender(appender);
    static_cast<ErrorInstance*>(exception)->setRuntimeTypeForCause(runtimeTypeForValue(value));
    return exception;
}

JSObject* createInvalidFunctionApplyParameterError(ExecState* exec, JSValue value)
{
    JSObject* exception = createTypeError(exec, makeString("second argument to Function.prototype.apply must be an Array-like object"));
    ASSERT(exception->isErrorInstance());
    static_cast<ErrorInstance*>(exception)->setSourceAppender(defaultSourceAppender);
    static_cast<ErrorInstance*>(exception)->setRuntimeTypeForCause(runtimeTypeForValue(value));
    return exception;
}

JSObject* createInvalidInParameterError(ExecState* exec, JSValue value)
{
    return createError(exec, createTypeError, value, makeString("is not an Object."), invalidParameterInSourceAppender);
}

JSObject* createInvalidInstanceofParameterError(ExecState* exec, JSValue value)
{
    return createError(exec, createTypeError, value, makeString("is not a function."), invalidParameterInstanceofSourceAppender);
}

JSObject* createNotAConstructorError(ExecState* exec, JSValue value)
{
    return createError(exec, createTypeError, value, ASCIILiteral("is not a constructor"), defaultSourceAppender);
}

JSObject* createNotAFunctionError(ExecState* exec, JSValue value)
{
    return createError(exec, createTypeError, value, ASCIILiteral("is not a function"), notAFunctionSourceAppender);
}

JSObject* createNotAnObjectError(ExecState* exec, JSValue value)
{
    return createError(exec, createTypeError, value, ASCIILiteral("is not an object"), defaultSourceAppender);
}

JSObject* createErrorForInvalidGlobalAssignment(ExecState* exec, const String& propertyName)
{
    return createReferenceError(exec, makeString("Strict mode forbids implicit creation of global property '", propertyName, '\''));
}

JSObject* createOutOfMemoryError(JSGlobalObject* globalObject)
{
    return createError(globalObject, ASCIILiteral("Out of memory"));
}

JSObject* throwOutOfMemoryError(ExecState* exec)
{
    return exec->vm().throwException(exec, createOutOfMemoryError(exec->lexicalGlobalObject()));
}

JSObject* throwStackOverflowError(ExecState* exec)
{
    VM& vm = exec->vm();
    ErrorHandlingScope errorScope(vm);
    return vm.throwException(exec, createStackOverflowError(exec));
}

JSObject* throwTerminatedExecutionException(ExecState* exec)
{
    VM& vm = exec->vm();
    ErrorHandlingScope errorScope(vm);
    return vm.throwException(exec, createTerminatedExecutionException(&vm));
}

} // namespace JSC
