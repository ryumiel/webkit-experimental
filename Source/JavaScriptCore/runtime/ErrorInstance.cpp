/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "ErrorInstance.h"

#include "JSScope.h"
#include "JSCInlines.h"

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(ErrorInstance);

const ClassInfo ErrorInstance::s_info = { "Error", &JSNonFinalObject::s_info, 0, CREATE_METHOD_TABLE(ErrorInstance) };

ErrorInstance::ErrorInstance(VM& vm, Structure* structure)
    : JSNonFinalObject(vm, structure)
{
}

void ErrorInstance::finishCreation(VM& vm, const String& message, Vector<StackFrame> stackTrace)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    if (!message.isNull())
        putDirect(vm, vm.propertyNames->message, jsString(&vm, message), DontEnum);
    
    if (!stackTrace.isEmpty())
        putDirect(vm, vm.propertyNames->stack, vm.interpreter->stackTraceAsString(vm.topCallFrame, stackTrace), DontEnum);
}
    
} // namespace JSC
