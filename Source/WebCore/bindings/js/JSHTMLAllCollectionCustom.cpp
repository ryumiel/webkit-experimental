/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
#include "JSHTMLAllCollection.h"

#include "HTMLAllCollection.h"
#include "JSNode.h"
#include "JSNodeList.h"
#include "StaticNodeList.h"
#include <runtime/IdentifierInlines.h>

using namespace JSC;

namespace WebCore {

static JSValue namedItems(ExecState* exec, JSHTMLAllCollection* collection, PropertyName propertyName)
{
    Vector<Ref<Element>> namedItems = collection->impl().namedItems(propertyNameToAtomicString(propertyName));

    if (namedItems.isEmpty())
        return jsUndefined();
    if (namedItems.size() == 1)
        return toJS(exec, collection->globalObject(), namedItems[0].ptr());

    // FIXME: HTML5 specification says this should be a HTMLCollection.
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/common-dom-interfaces.html#htmlallcollection
    return toJS(exec, collection->globalObject(), StaticElementList::adopt(namedItems).get());
}

// HTMLAllCollections are strange objects, they support both get and call.
static EncodedJSValue JSC_HOST_CALL callHTMLAllCollection(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return JSValue::encode(jsUndefined());

    // Do not use thisObj here. It can be the JSHTMLDocument, in the document.forms(i) case.
    JSHTMLAllCollection* jsCollection = jsCast<JSHTMLAllCollection*>(exec->callee());
    HTMLAllCollection& collection = jsCollection->impl();

    // Also, do we need the TypeError test here ?

    if (exec->argumentCount() == 1) {
        // Support for document.all(<index>) etc.
        String string = exec->argument(0).toString(exec)->value(exec);
        unsigned index = toUInt32FromStringImpl(string.impl());
        if (index != PropertyName::NotAnIndex)
            return JSValue::encode(toJS(exec, jsCollection->globalObject(), collection.item(index)));

        // Support for document.images('<name>') etc.
        return JSValue::encode(namedItems(exec, jsCollection, Identifier::fromString(exec, string)));
    }

    // The second arg, if set, is the index of the item we want
    String string = exec->argument(0).toString(exec)->value(exec);
    unsigned index = toUInt32FromStringImpl(exec->argument(1).toWTFString(exec).impl());
    if (index != PropertyName::NotAnIndex) {
        if (auto* item = collection.namedItemWithIndex(string, index))
            return JSValue::encode(toJS(exec, jsCollection->globalObject(), item));
    }

    return JSValue::encode(jsUndefined());
}

CallType JSHTMLAllCollection::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callHTMLAllCollection;
    return CallTypeHost;
}

bool JSHTMLAllCollection::canGetItemsForName(ExecState*, HTMLAllCollection* collection, PropertyName propertyName)
{
    return collection->hasNamedItem(propertyNameToAtomicString(propertyName));
}

EncodedJSValue JSHTMLAllCollection::nameGetter(ExecState* exec, JSObject* slotBase, EncodedJSValue, PropertyName propertyName)
{
    JSHTMLAllCollection* thisObj = jsCast<JSHTMLAllCollection*>(slotBase);
    return JSValue::encode(namedItems(exec, thisObj, propertyName));
}

JSValue JSHTMLAllCollection::item(ExecState* exec)
{
    uint32_t index = toUInt32FromStringImpl(exec->argument(0).toString(exec)->value(exec).impl());
    if (index != PropertyName::NotAnIndex)
        return toJS(exec, globalObject(), impl().item(index));
    return namedItems(exec, this, Identifier::fromString(exec, exec->argument(0).toString(exec)->value(exec)));
}

JSValue JSHTMLAllCollection::namedItem(ExecState* exec)
{
    JSValue value = namedItems(exec, this, Identifier::fromString(exec, exec->argument(0).toString(exec)->value(exec)));
    return value.isUndefined() ? jsNull() : value;
}

} // namespace WebCore
