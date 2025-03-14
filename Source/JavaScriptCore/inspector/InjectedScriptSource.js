/*
 * Copyright (C) 2007, 2014 Apple Inc.  All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

//# sourceURL=__WebInspectorInjectedScript__

(function (InjectedScriptHost, inspectedGlobalObject, injectedScriptId) {

// Protect against Object overwritten by the user code.
var Object = {}.constructor;

function toString(obj)
{
    return String(obj);
}

function toStringDescription(obj)
{
    if (obj === 0 && 1 / obj < 0)
        return "-0";

    return toString(obj);
}

function isUInt32(obj)
{
    if (typeof obj === "number")
        return obj >>> 0 === obj && (obj > 0 || 1 / obj > 0);
    return "" + (obj >>> 0) === obj;
}

function isSymbol(obj)
{
    return typeof obj === "symbol";
}

var InjectedScript = function()
{
    this._lastBoundObjectId = 1;
    this._idToWrappedObject = {};
    this._idToObjectGroupName = {};
    this._objectGroups = {};
    this._modules = {};
    this._nextSavedResultIndex = 1;
    this._savedResults = [];
}

InjectedScript.primitiveTypes = {
    undefined: true,
    boolean: true,
    number: true,
    string: true,
}

InjectedScript.CollectionMode = {
    OwnProperties: 1 << 0,          // own properties.
    NativeGetterProperties: 1 << 1, // native getter properties in the prototype chain.
    AllProperties: 1 << 2,          // all properties in the prototype chain.
}

InjectedScript.prototype = {
    isPrimitiveValue: function(object)
    {
        // FIXME(33716): typeof document.all is always 'undefined'.
        return InjectedScript.primitiveTypes[typeof object] && !this._isHTMLAllCollection(object);
    },

    wrapObject: function(object, groupName, canAccessInspectedGlobalObject, generatePreview)
    {
        if (canAccessInspectedGlobalObject)
            return this._wrapObject(object, groupName, false, generatePreview);
        return this._fallbackWrapper(object);
    },

    setExceptionValue: function(value)
    {
        this._exceptionValue = value;
    },

    clearExceptionValue: function()
    {
        delete this._exceptionValue;
    },

    _fallbackWrapper: function(object)
    {
        var result = {};
        result.type = typeof object;
        if (this.isPrimitiveValue(object))
            result.value = object;
        else
            result.description = toString(object);
        return result;
    },

    wrapTable: function(canAccessInspectedGlobalObject, table, columns)
    {
        if (!canAccessInspectedGlobalObject)
            return this._fallbackWrapper(table);

        // FIXME: Currently columns are ignored. Instead, the frontend filters all
        // properties based on the provided column names and in the provided order.
        // Should we filter here too?

        var columnNames = null;
        if (typeof columns === "string")
            columns = [columns];

        if (InjectedScriptHost.subtype(columns) === "array") {
            columnNames = [];
            for (var i = 0; i < columns.length; ++i)
                columnNames.push(toString(columns[i]));
        }

        return this._wrapObject(table, "console", false, true, columnNames);
    },

    inspectObject: function(object)
    {
        if (this._commandLineAPIImpl)
            this._commandLineAPIImpl.inspect(object);
    },

    _wrapObject: function(object, objectGroupName, forceValueType, generatePreview, columnNames)
    {
        try {
            return new InjectedScript.RemoteObject(object, objectGroupName, forceValueType, generatePreview, columnNames);
        } catch (e) {
            try {
                var description = injectedScript._describe(e);
            } catch (ex) {
                var description = "<failed to convert exception to string>";
            }
            return new InjectedScript.RemoteObject(description);
        }
    },

    _bind: function(object, objectGroupName)
    {
        var id = this._lastBoundObjectId++;
        this._idToWrappedObject[id] = object;
        var objectId = "{\"injectedScriptId\":" + injectedScriptId + ",\"id\":" + id + "}";
        if (objectGroupName) {
            var group = this._objectGroups[objectGroupName];
            if (!group) {
                group = [];
                this._objectGroups[objectGroupName] = group;
            }
            group.push(id);
            this._idToObjectGroupName[id] = objectGroupName;
        }
        return objectId;
    },

    _parseObjectId: function(objectId)
    {
        return InjectedScriptHost.evaluate("(" + objectId + ")");
    },

    releaseObjectGroup: function(objectGroupName)
    {
        if (objectGroupName === "console") {
            delete this._lastResult;
            this._nextSavedResultIndex = 1;
            this._savedResults = [];
        }

        var group = this._objectGroups[objectGroupName];
        if (!group)
            return;

        for (var i = 0; i < group.length; i++)
            this._releaseObject(group[i]);

        delete this._objectGroups[objectGroupName];
    },

    dispatch: function(methodName, args)
    {
        var argsArray = InjectedScriptHost.evaluate("(" + args + ")");
        var result = this[methodName].apply(this, argsArray);
        if (typeof result === "undefined") {
            if (inspectedGlobalObject.console)
                inspectedGlobalObject.console.error("Web Inspector error: InjectedScript.%s returns undefined", methodName);
            result = null;
        }
        return result;
    },

    _getProperties: function(objectId, collectionMode, generatePreview)
    {
        var parsedObjectId = this._parseObjectId(objectId);
        var object = this._objectForId(parsedObjectId);
        var objectGroupName = this._idToObjectGroupName[parsedObjectId.id];

        if (!this._isDefined(object))
            return false;

        if (isSymbol(object))
            return false;

        var descriptors = this._propertyDescriptors(object, collectionMode);

        // Go over properties, wrap object values.
        for (var i = 0; i < descriptors.length; ++i) {
            var descriptor = descriptors[i];
            if ("get" in descriptor)
                descriptor.get = this._wrapObject(descriptor.get, objectGroupName);
            if ("set" in descriptor)
                descriptor.set = this._wrapObject(descriptor.set, objectGroupName);
            if ("value" in descriptor)
                descriptor.value = this._wrapObject(descriptor.value, objectGroupName, false, generatePreview);
            if (!("configurable" in descriptor))
                descriptor.configurable = false;
            if (!("enumerable" in descriptor))
                descriptor.enumerable = false;
        }

        return descriptors;
    },

    getProperties: function(objectId, ownProperties, generatePreview)
    {
        var collectionMode = ownProperties ? InjectedScript.CollectionMode.OwnProperties : InjectedScript.CollectionMode.AllProperties;
        return this._getProperties(objectId, collectionMode, generatePreview);
    },

    getDisplayableProperties: function(objectId, generatePreview)
    {
        var collectionMode = InjectedScript.CollectionMode.OwnProperties | InjectedScript.CollectionMode.NativeGetterProperties;
        return this._getProperties(objectId, collectionMode, generatePreview);
    },

    getInternalProperties: function(objectId, generatePreview)
    {
        var parsedObjectId = this._parseObjectId(objectId);
        var object = this._objectForId(parsedObjectId);
        var objectGroupName = this._idToObjectGroupName[parsedObjectId.id];

        if (!this._isDefined(object))
            return false;

        if (isSymbol(object))
            return false;

        var descriptors = this._internalPropertyDescriptors(object);
        if (!descriptors)
            return [];

        // Go over properties, wrap object values.
        for (var i = 0; i < descriptors.length; ++i) {
            var descriptor = descriptors[i];
            if ("value" in descriptor)
                descriptor.value = this._wrapObject(descriptor.value, objectGroupName, false, generatePreview);
        }

        return descriptors;
    },

    getCollectionEntries: function(objectId, objectGroupName, startIndex, numberToFetch)
    {
        var parsedObjectId = this._parseObjectId(objectId);
        var object = this._objectForId(parsedObjectId);
        var objectGroupName = objectGroupName || this._idToObjectGroupName[parsedObjectId.id];

        if (!this._isDefined(object))
            return;

        if (typeof object !== "object")
            return;

        var entries = this._entries(object, InjectedScriptHost.subtype(object), startIndex, numberToFetch);
        return entries.map(function(entry) {
            entry.value = injectedScript._wrapObject(entry.value, objectGroupName, false, true);
            if ("key" in entry)
                entry.key = injectedScript._wrapObject(entry.key, objectGroupName, false, true);
            return entry;
        });
    },

    saveResult: function(callArgumentJSON)
    {
        this._savedResultIndex = 0;

        try {
            var callArgument = InjectedScriptHost.evaluate("(" + callArgumentJSON + ")");
            var value = this._resolveCallArgument(callArgument);
            this._saveResult(value);
        } catch (e) {}

        return this._savedResultIndex;
    },

    getFunctionDetails: function(functionId)
    {
        var parsedFunctionId = this._parseObjectId(functionId);
        var func = this._objectForId(parsedFunctionId);
        if (typeof func !== "function")
            return "Cannot resolve function by id.";
        var details = InjectedScriptHost.functionDetails(func);
        if (!details)
            return "Cannot resolve function details.";
        if ("rawScopes" in details) {
            var objectGroupName = this._idToObjectGroupName[parsedFunctionId.id];
            var rawScopes = details.rawScopes;
            var scopes = [];
            delete details.rawScopes;
            for (var i = 0; i < rawScopes.length; i++)
                scopes.push(InjectedScript.CallFrameProxy._createScopeJson(rawScopes[i].type, rawScopes[i].object, objectGroupName));
            details.scopeChain = scopes;
        }
        return details;
    },

    releaseObject: function(objectId)
    {
        var parsedObjectId = this._parseObjectId(objectId);
        this._releaseObject(parsedObjectId.id);
    },

    _releaseObject: function(id)
    {
        delete this._idToWrappedObject[id];
        delete this._idToObjectGroupName[id];
    },

    evaluate: function(expression, objectGroup, injectCommandLineAPI, returnByValue, generatePreview, saveResult)
    {
        return this._evaluateAndWrap(InjectedScriptHost.evaluate, InjectedScriptHost, expression, objectGroup, false, injectCommandLineAPI, returnByValue, generatePreview, saveResult);
    },

    callFunctionOn: function(objectId, expression, args, returnByValue, generatePreview)
    {
        var parsedObjectId = this._parseObjectId(objectId);
        var object = this._objectForId(parsedObjectId);
        if (!this._isDefined(object))
            return "Could not find object with given id";

        if (args) {
            var resolvedArgs = [];
            var callArgs = InjectedScriptHost.evaluate(args);
            for (var i = 0; i < callArgs.length; ++i) {
                try {
                    resolvedArgs[i] = this._resolveCallArgument(callArgs[i]);
                } catch (e) {
                    return String(e);
                }
            }
        }

        try {
            var objectGroup = this._idToObjectGroupName[parsedObjectId.id];
            var func = InjectedScriptHost.evaluate("(" + expression + ")");
            if (typeof func !== "function")
                return "Given expression does not evaluate to a function";

            return {
                wasThrown: false,
                result: this._wrapObject(func.apply(object, resolvedArgs), objectGroup, returnByValue, generatePreview)
            };
        } catch (e) {
            return this._createThrownValue(e, objectGroup);
        }
    },

    _resolveCallArgument: function(callArgumentJSON)
    {
        if ("value" in callArgumentJSON)
            return callArgumentJSON.value;

        var objectId = callArgumentJSON.objectId;
        if (objectId) {
            var parsedArgId = this._parseObjectId(objectId);
            if (!parsedArgId || parsedArgId["injectedScriptId"] !== injectedScriptId)
                throw "Arguments should belong to the same JavaScript world as the target object.";

            var resolvedArg = this._objectForId(parsedArgId);
            if (!this._isDefined(resolvedArg))
                throw "Could not find object with given id";

            return resolvedArg;
        }

        return undefined;
    },

    _evaluateAndWrap: function(evalFunction, object, expression, objectGroup, isEvalOnCallFrame, injectCommandLineAPI, returnByValue, generatePreview, saveResult)
    {
        try {
            this._savedResultIndex = 0;

            var returnObject = {
                wasThrown: false,
                result: this._wrapObject(this._evaluateOn(evalFunction, object, objectGroup, expression, isEvalOnCallFrame, injectCommandLineAPI, saveResult), objectGroup, returnByValue, generatePreview)
            };

            if (saveResult && this._savedResultIndex)
                returnObject.savedResultIndex = this._savedResultIndex;

            return returnObject;
        } catch (e) {
            return this._createThrownValue(e, objectGroup);
        }
    },

    _createThrownValue: function(value, objectGroup)
    {
        var remoteObject = this._wrapObject(value, objectGroup);
        try {
            remoteObject.description = toStringDescription(value);
        } catch (e) {}
        return {
            wasThrown: true,
            result: remoteObject
        };
    },

    _evaluateOn: function(evalFunction, object, objectGroup, expression, isEvalOnCallFrame, injectCommandLineAPI, saveResult)
    {
        var commandLineAPI = null;
        if (injectCommandLineAPI) {
            if (this.CommandLineAPI)
                commandLineAPI = new this.CommandLineAPI(this._commandLineAPIImpl, isEvalOnCallFrame ? object : null);
            else
                commandLineAPI = new BasicCommandLineAPI;
        }

        if (isEvalOnCallFrame) {
            // We can only use this approach if the evaluate function is the true 'eval'. That allows us to use it with
            // the 'eval' identifier when calling it. Using 'eval' grants access to the local scope of the closure we
            // create that provides the command line APIs.

            var parameters = [InjectedScriptHost.evaluate, expression];
            var expressionFunctionBody = "" +
                "var global = Function('return this')() || (1, eval)('this');" +
                "var __originalEval = global.eval; global.eval = __eval;" +
                "try { return eval(__currentExpression); }" +
                "finally { global.eval = __originalEval; }";

            if (commandLineAPI) {
                // To avoid using a 'with' statement (which fails in strict mode and requires injecting the API object)
                // we instead create a closure where we evaluate the expression. The command line APIs are passed as
                // parameters to the closure so they are in scope but not injected. This allows the code evaluated in
                // the console to stay in strict mode (if is was already set), or to get strict mode by prefixing
                // expressions with 'use strict';.

                var parameterNames = Object.getOwnPropertyNames(commandLineAPI);
                for (var i = 0; i < parameterNames.length; ++i)
                    parameters.push(commandLineAPI[parameterNames[i]]);

                var expressionFunctionString = "(function(__eval, __currentExpression, " + parameterNames.join(", ") + ") { " + expressionFunctionBody + " })";
            } else {
                // Use a closure in this case too to keep the same behavior of 'var' being captured by the closure instead
                // of leaking out into the calling scope.
                var expressionFunctionString = "(function(__eval, __currentExpression) { " + expressionFunctionBody + " })";
            }

            // Bind 'this' to the function expression using another closure instead of Function.prototype.bind. This ensures things will work if the page replaces bind.
            var boundExpressionFunctionString = "(function(__function, __thisObject) { return function() { return __function.apply(__thisObject, arguments) }; })(" + expressionFunctionString + ", this)";
            var expressionFunction = evalFunction.call(object, boundExpressionFunctionString);
            var result = expressionFunction.apply(null, parameters);

            if (objectGroup === "console" && saveResult)
                this._saveResult(result);

            return result;
        }

        // When not evaluating on a call frame we use a 'with' statement to allow var and function statements to leak
        // into the global scope. This allow them to stick around between evaluations.

        try {
            if (commandLineAPI) {
                if (inspectedGlobalObject.console)
                    inspectedGlobalObject.console.__commandLineAPI = commandLineAPI;
                else
                    inspectedGlobalObject.__commandLineAPI = commandLineAPI;
                expression = "with ((this && (this.console ? this.console.__commandLineAPI : this.__commandLineAPI)) || {}) { " + expression + "\n}";
            }

            var result = evalFunction.call(inspectedGlobalObject, expression);

            if (objectGroup === "console" && saveResult)
                this._saveResult(result);

            return result;
        } finally {
            if (commandLineAPI) {
                if (inspectedGlobalObject.console)
                    delete inspectedGlobalObject.console.__commandLineAPI;
                else
                    delete inspectedGlobalObject.__commandLineAPI;
            }
        }
    },

    wrapCallFrames: function(callFrame)
    {
        if (!callFrame)
            return false;

        var result = [];
        var depth = 0;
        do {
            result.push(new InjectedScript.CallFrameProxy(depth++, callFrame));
            callFrame = callFrame.caller;
        } while (callFrame);
        return result;
    },

    evaluateOnCallFrame: function(topCallFrame, callFrameId, expression, objectGroup, injectCommandLineAPI, returnByValue, generatePreview, saveResult)
    {
        var callFrame = this._callFrameForId(topCallFrame, callFrameId);
        if (!callFrame)
            return "Could not find call frame with given id";
        return this._evaluateAndWrap(callFrame.evaluate, callFrame, expression, objectGroup, true, injectCommandLineAPI, returnByValue, generatePreview, saveResult);
    },

    _callFrameForId: function(topCallFrame, callFrameId)
    {
        var parsedCallFrameId = InjectedScriptHost.evaluate("(" + callFrameId + ")");
        var ordinal = parsedCallFrameId["ordinal"];
        var callFrame = topCallFrame;
        while (--ordinal >= 0 && callFrame)
            callFrame = callFrame.caller;
        return callFrame;
    },

    _objectForId: function(objectId)
    {
        return this._idToWrappedObject[objectId.id];
    },

    findObjectById: function(objectId)
    {
        var parsedObjectId = this._parseObjectId(objectId);
        return this._objectForId(parsedObjectId);
    },

    module: function(name)
    {
        return this._modules[name];
    },

    injectModule: function(name, source, host)
    {
        delete this._modules[name];

        var moduleFunction = InjectedScriptHost.evaluate("(" + source + ")");
        if (typeof moduleFunction !== "function") {
            if (inspectedGlobalObject.console)
                inspectedGlobalObject.console.error("Web Inspector error: A function was expected for module %s evaluation", name);
            return null;
        }

        var module = moduleFunction.call(inspectedGlobalObject, InjectedScriptHost, inspectedGlobalObject, injectedScriptId, this, host);
        this._modules[name] = module;
        return module;
    },

    _internalPropertyDescriptors: function(object, completeDescriptor)
    {
        var internalProperties = InjectedScriptHost.getInternalProperties(object);
        if (!internalProperties)
            return null;

        var descriptors = [];
        for (var i = 0; i < internalProperties.length; i++) {
            var property = internalProperties[i];
            var descriptor = {name: property.name, value: property.value};
            if (completeDescriptor) {
                descriptor.writable = false;
                descriptor.configurable = false;
                descriptor.enumerable = false;
                descriptor.isOwn = true;
            }
            descriptors.push(descriptor);
        }
        return descriptors;
    },

    _propertyDescriptors: function(object, collectionMode)
    {
        var descriptors = [];
        var nameProcessed = {};
        nameProcessed["__proto__"] = null;

        function createFakeValueDescriptor(name, descriptor, isOwnProperty, possibleNativeBindingGetter)
        {
            try {
                var descriptor = {name, value: object[name], writable: descriptor.writable || false, configurable: descriptor.configurable || false, enumerable: descriptor.enumerable || false};
                if (possibleNativeBindingGetter)
                    descriptor.nativeGetter = true;
                return descriptor;
            } catch (e) {
                var errorDescriptor = {name, value: e, wasThrown: true};
                if (isOwnProperty)
                    errorDescriptor.isOwn = true;
                return errorDescriptor;
            }
        }

        function processDescriptor(descriptor, isOwnProperty, possibleNativeBindingGetter)
        {
            // All properties.
            if (collectionMode & InjectedScript.CollectionMode.AllProperties) {
                descriptors.push(descriptor);
                return;
            }

            // Own properties.
            if (collectionMode & InjectedScript.CollectionMode.OwnProperties && isOwnProperty) {
                descriptors.push(descriptor);
                return;
            }

            // Native Getter properties.
            if (collectionMode & InjectedScript.CollectionMode.NativeGetterProperties) {
                // FIXME: <https://webkit.org/b/140575> Web Inspector: Native Bindings Descriptors are Incomplete
                // if (descriptor.hasOwnProperty("get") && descriptor.get && isNativeFunction(descriptor.get)) { ... }

                if (possibleNativeBindingGetter) {
                    // Possible getter property in the prototype chain.
                    descriptors.push(descriptor);
                    return;
                }
            }
        }

        function processPropertyNames(o, names, isOwnProperty)
        {
            for (var i = 0; i < names.length; ++i) {
                var name = names[i];
                if (nameProcessed[name] || name === "__proto__")
                    continue;

                nameProcessed[name] = true;

                var descriptor = Object.getOwnPropertyDescriptor(o, name);
                if (!descriptor) {
                    // FIXME: Bad descriptor. Can we get here?
                    // Fall back to very restrictive settings.
                    var fakeDescriptor = createFakeValueDescriptor(name, {writable: false, configurable: false, enumerable: false}, isOwnProperty);
                    processDescriptor(fakeDescriptor, isOwnProperty);
                    continue;
                }

                if (descriptor.hasOwnProperty("get") && descriptor.hasOwnProperty("set") && !descriptor.get && !descriptor.set) {
                    // FIXME: <https://webkit.org/b/140575> Web Inspector: Native Bindings Descriptors are Incomplete
                    // Developers may create such a descriptors, so we should be resilient:
                    // var x = {}; Object.defineProperty(x, "p", {get:undefined}); Object.getOwnPropertyDescriptor(x, "p")
                    var fakeDescriptor = createFakeValueDescriptor(name, descriptor, isOwnProperty, true);
                    processDescriptor(fakeDescriptor, isOwnProperty, true);
                    continue;
                }

                descriptor.name = name;
                if (isOwnProperty)
                    descriptor.isOwn = true;
                processDescriptor(descriptor, isOwnProperty);
            }
        }

        // Iterate prototype chain.
        for (var o = object; this._isDefined(o); o = o.__proto__) {
            var isOwnProperty = o === object;
            processPropertyNames(o, Object.getOwnPropertyNames(o), isOwnProperty);
            if (collectionMode === InjectedScript.CollectionMode.OwnProperties)
                break;
        }

        // Always include __proto__ at the end.
        try {
            if (object.__proto__)
                descriptors.push({name: "__proto__", value: object.__proto__, writable: true, configurable: true, enumerable: false, isOwn: true});
        } catch (e) {}

        return descriptors;
    },

    _isDefined: function(object)
    {
        return !!object || this._isHTMLAllCollection(object);
    },

    _isHTMLAllCollection: function(object)
    {
        // document.all is reported as undefined, but we still want to process it.
        return (typeof object === "undefined") && InjectedScriptHost.isHTMLAllCollection(object);
    },

    _subtype: function(obj)
    {
        if (obj === null)
            return "null";

        if (this.isPrimitiveValue(obj) || isSymbol(obj))
            return null;

        if (this._isHTMLAllCollection(obj))
            return "array";

        var preciseType = InjectedScriptHost.subtype(obj);
        if (preciseType)
            return preciseType;

        // FireBug's array detection.
        try {
            if (typeof obj.splice === "function" && isFinite(obj.length))
                return "array";
            if (Object.prototype.toString.call(obj) === "[object Arguments]" && isFinite(obj.length)) // arguments.
                return "array";
        } catch (e) {
        }

        return null;
    },

    _describe: function(obj)
    {
        if (this.isPrimitiveValue(obj))
            return null;

        if (isSymbol(obj))
            return toString(obj);

        var subtype = this._subtype(obj);

        if (subtype === "regexp")
            return toString(obj);

        if (subtype === "date")
            return toString(obj);

        if (subtype === "error")
            return toString(obj);

        if (subtype === "node") {
            var description = obj.nodeName.toLowerCase();
            switch (obj.nodeType) {
            case 1 /* Node.ELEMENT_NODE */:
                description += obj.id ? "#" + obj.id : "";
                var className = obj.className;
                description += (className && typeof className === "string") ? "." + className.trim().replace(/\s+/g, ".") : "";
                break;
            case 10 /*Node.DOCUMENT_TYPE_NODE */:
                description = "<!DOCTYPE " + description + ">";
                break;
            }
            return description;
        }

        var className = InjectedScriptHost.internalConstructorName(obj);
        if (subtype === "array")
            return className;

        if (subtype === "class")
            return obj.name;

        // NodeList in JSC is a function, check for array prior to this.
        if (typeof obj === "function")
            return toString(obj);

        // If Object, try for a better name from the constructor.
        if (className === "Object") {
            var constructorName = obj.constructor && obj.constructor.name;
            if (constructorName)
                return constructorName;
        }

        return className;
    },

    _getSetEntries: function(object, skip, numberToFetch)
    {
        var entries = [];

        for (var value of object) {
            if (skip > 0) {
                skip--;
                continue;
            }

            entries.push({value});

            if (numberToFetch && entries.length === numberToFetch)
                break;
        }

        return entries;
    },

    _getMapEntries: function(object, skip, numberToFetch)
    {
        var entries = [];

        for (var [key, value] of object) {
            if (skip > 0) {
                skip--;
                continue;
            }

            entries.push({key, value});

            if (numberToFetch && entries.length === numberToFetch)
                break;
        }

        return entries;
    },

    _getWeakMapEntries: function(object, numberToFetch)
    {
        return InjectedScriptHost.weakMapEntries(object, numberToFetch);
    },

    _getIteratorEntries: function(object, numberToFetch)
    {
        return InjectedScriptHost.iteratorEntries(object, numberToFetch);
    },

    _entries: function(object, subtype, startIndex, numberToFetch)
    {
        if (subtype === "set")
            return this._getSetEntries(object, startIndex, numberToFetch);
        if (subtype === "map")
            return this._getMapEntries(object, startIndex, numberToFetch);
        if (subtype === "weakmap")
            return this._getWeakMapEntries(object, numberToFetch);
        if (subtype === "iterator")
            return this._getIteratorEntries(object, numberToFetch);

        throw "unexpected type";
    },

    _saveResult: function(result)
    {
        this._lastResult = result;

        if (result === undefined || result === null)
            return;

        var existingIndex = this._savedResults.indexOf(result);
        if (existingIndex !== -1) {
            this._savedResultIndex = existingIndex;
            return;
        }

        this._savedResultIndex = this._nextSavedResultIndex;
        this._savedResults[this._nextSavedResultIndex++] = result;

        // $n is limited from $1-$99. $0 is special.
        if (this._nextSavedResultIndex >= 100)
            this._nextSavedResultIndex = 1;
    },

    _savedResult: function(index)
    {
        return this._savedResults[index];
    }
}

var injectedScript = new InjectedScript;


InjectedScript.RemoteObject = function(object, objectGroupName, forceValueType, generatePreview, columnNames)
{
    this.type = typeof object;

    if (this.type === "undefined" && injectedScript._isHTMLAllCollection(object))
        this.type = "object";

    if (injectedScript.isPrimitiveValue(object) || object === null || forceValueType) {
        // We don't send undefined values over JSON.
        if (this.type !== "undefined")
            this.value = object;

        // Null object is object with 'null' subtype.
        if (object === null)
            this.subtype = "null";

        // Provide user-friendly number values.
        if (this.type === "number")
            this.description = toStringDescription(object);
        return;
    }

    this.objectId = injectedScript._bind(object, objectGroupName);

    var subtype = injectedScript._subtype(object);
    if (subtype)
        this.subtype = subtype;

    this.className = InjectedScriptHost.internalConstructorName(object);
    this.description = injectedScript._describe(object);

    if (subtype === "array")
        this.size = typeof object.length === "number" ? object.length : 0;
    else if (subtype === "set" || subtype === "map")
        this.size = object.size;
    else if (subtype === "weakmap")
        this.size = InjectedScriptHost.weakMapSize(object);
    else if (subtype === "class")
        this.classPrototype = injectedScript._wrapObject(object.prototype, objectGroupName);

    if (generatePreview && this.type === "object")
        this.preview = this._generatePreview(object, undefined, columnNames);
}

InjectedScript.RemoteObject.prototype = {
    _emptyPreview: function()
    {
        var preview = {
            type: this.type,
            description: this.description || toString(this.value),
            lossless: true,
        };

        if (this.subtype) {
            preview.subtype = this.subtype;
            if (this.subtype !== "null") {
                preview.overflow = false;
                preview.properties = [];
            }
        }

        if ("size" in this)
            preview.size = this.size;

        return preview;
    },

    _createObjectPreviewForValue: function(value)
    {
        var remoteObject = new InjectedScript.RemoteObject(value, undefined, false, true, undefined);
        if (remoteObject.objectId)
            injectedScript.releaseObject(remoteObject.objectId);
        if (remoteObject.classPrototype && remoteObject.classPrototype.objectId)
            injectedScript.releaseObject(remoteObject.classPrototype.objectId);

        return remoteObject.preview || remoteObject._emptyPreview();
    },

    _generatePreview: function(object, firstLevelKeys, secondLevelKeys)
    {
        var preview = this._emptyPreview();

        // Primitives just have a value.
        if (this.type !== "object")
            return;

        var isTableRowsRequest = secondLevelKeys === null || secondLevelKeys;
        var firstLevelKeysCount = firstLevelKeys ? firstLevelKeys.length : 0;

        var propertiesThreshold = {
            properties: isTableRowsRequest ? 1000 : Math.max(5, firstLevelKeysCount),
            indexes: isTableRowsRequest ? 1000 : Math.max(100, firstLevelKeysCount)
        };

        try {
            // Maps, Sets, and Iterators have entries.
            if (this.subtype === "map" || this.subtype === "set" || this.subtype === "weakmap" || this.subtype === "iterator")
                this._appendEntryPreviews(object, preview);

            preview.properties = [];

            // Internal Properties.
            var internalPropertyDescriptors = injectedScript._internalPropertyDescriptors(object, true);
            if (internalPropertyDescriptors) {
                this._appendPropertyPreviews(preview, internalPropertyDescriptors, true, propertiesThreshold, firstLevelKeys, secondLevelKeys);
                if (propertiesThreshold.indexes < 0 || propertiesThreshold.properties < 0)
                    return preview;
            }

            if (preview.entries)
                return preview;

            // Properties.
            var descriptors = injectedScript._propertyDescriptors(object, InjectedScript.CollectionMode.AllProperties);
            this._appendPropertyPreviews(preview, descriptors, false, propertiesThreshold, firstLevelKeys, secondLevelKeys);
            if (propertiesThreshold.indexes < 0 || propertiesThreshold.properties < 0)
                return preview;
        } catch (e) {
            preview.lossless = false;
        }

        return preview;
    },

    _appendPropertyPreviews: function(preview, descriptors, internal, propertiesThreshold, firstLevelKeys, secondLevelKeys)
    {
        for (var descriptor of descriptors) {
            // Seen enough.
            if (propertiesThreshold.indexes < 0 || propertiesThreshold.properties < 0)
                break;

            // Error in descriptor.
            if (descriptor.wasThrown) {
                preview.lossless = false;
                continue;
            }

            // Do not show "__proto__" in preview.
            var name = descriptor.name;
            if (name === "__proto__")
                continue;

            // Do not show "length" on array like objects in preview.
            if (this.subtype === "array" && name === "length")
                continue;

            // Do not show non-enumerable non-own properties. Special case to allow array indexes that may be on the prototype.
            if (!descriptor.enumerable && !descriptor.isOwn && !(this.subtype === "array" && isUInt32(name)))
                continue;

            // If we have a filter, only show properties in the filter.
            if (firstLevelKeys && firstLevelKeys.indexOf(name) === -1)
                continue;

            // Getter/setter.
            if (!("value" in descriptor)) {
                preview.lossless = false;
                this._appendPropertyPreview(preview, internal, {name, type: "accessor"}, propertiesThreshold);
                continue;
            }

            // Null value.
            var value = descriptor.value;
            if (value === null) {
                this._appendPropertyPreview(preview, internal, {name, type: "object", subtype: "null", value: "null"}, propertiesThreshold);
                continue;
            }

            // Ignore non-enumerable functions.
            var type = typeof value;
            if (!descriptor.enumerable && type === "function")
                continue;

            // Fix type of document.all.
            if (type === "undefined" && injectedScript._isHTMLAllCollection(value))
                type = "object";

            // Primitive.
            const maxLength = 100;
            if (InjectedScript.primitiveTypes[type]) {
                if (type === "string" && value.length > maxLength) {
                    value = this._abbreviateString(value, maxLength, true);
                    preview.lossless = false;
                }
                this._appendPropertyPreview(preview, internal, {name, type, value: toStringDescription(value)}, propertiesThreshold);
                continue;
            }

            // Symbol.
            if (isSymbol(value)) {
                var symbolString = toString(value);
                if (symbolString.length > maxLength) {
                    symbolString = this._abbreviateString(symbolString, maxLength, true);
                    preview.lossless = false;
                }
                this._appendPropertyPreview(preview, internal, {name, type, value: symbolString}, propertiesThreshold);
                return;
            }

            // Object.
            var property = {name, type};
            var subtype = injectedScript._subtype(value);
            if (subtype)
                property.subtype = subtype;

            // Second level.
            if (secondLevelKeys === null || secondLevelKeys) {
                var subPreview = this._generatePreview(value, secondLevelKeys || undefined, undefined);
                property.valuePreview = subPreview;
                if (!subPreview.lossless)
                    preview.lossless = false;
                if (subPreview.overflow)
                    preview.overflow = true;
            } else if (this._isPreviewableObject(value)) {
                var subPreview = this._createObjectPreviewForValue(value);
                property.valuePreview = subPreview;
                if (!subPreview.lossless)
                    preview.lossless = false;
                if (subPreview.overflow)
                    preview.overflow = true;
            } else {
                var description = "";
                if (type !== "function" || subtype === "class")
                    description = this._abbreviateString(injectedScript._describe(value), maxLength, subtype === "regexp");
                property.value = description;
                preview.lossless = false;
            }

            this._appendPropertyPreview(preview, internal, property, propertiesThreshold);
        }
    },

    _appendPropertyPreview: function(preview, internal, property, propertiesThreshold)
    {
        if (toString(property.name >>> 0) === property.name)
            propertiesThreshold.indexes--;
        else
            propertiesThreshold.properties--;

        if (propertiesThreshold.indexes < 0 || propertiesThreshold.properties < 0) {
            preview.overflow = true;
            preview.lossless = false;
            return;
        }

        if (internal)
            property.internal = true;

        preview.properties.push(property);
    },

    _appendEntryPreviews: function(object, preview)
    {
        // Fetch 6, but only return 5, so we can tell if we overflowed.
        var entries = injectedScript._entries(object, this.subtype, 0, 6);
        if (!entries)
            return;

        if (entries.length > 5) {
            entries.pop();
            preview.overflow = true;
            preview.lossless = false;
        }

        preview.entries = entries.map(function(entry) {
            entry.value = this._createObjectPreviewForValue(entry.value);
            if ("key" in entry)
                entry.key = this._createObjectPreviewForValue(entry.key);
            return entry;
        }, this);
    },

    _isPreviewableObject: function(object)
    {
        return this._isPreviewableObjectInternal(object, new Set, 1);
    },

    _isPreviewableObjectInternal: function(object, knownObjects, depth)
    {
        // Deep object.
        if (depth > 3)
            return false;

        // Primitive.
        if (injectedScript.isPrimitiveValue(object) || isSymbol(object))
            return true;

        // Null.
        if (object === null)
            return true;

        // Cyclic objects.
        if (knownObjects.has(object))
            return false;

        ++depth;
        knownObjects.add(object);

        // Arrays are simple if they have 5 or less simple objects.
        var subtype = injectedScript._subtype(object);
        if (subtype === "array") {
            var length = object.length;
            if (length > 5)
                return false;
            for (var i = 0; i < length; ++i) {
                if (!this._isPreviewableObjectInternal(object[i], knownObjects, depth))
                    return false;
            }
            return true;
        }

        // Not a basic object.
        if (object.__proto__ && object.__proto__.__proto__)
            return false;

        // Objects are simple if they have 3 or less simple properties.
        var ownPropertyNames = Object.getOwnPropertyNames(object);
        if (ownPropertyNames.length > 3)
            return false;
        for (var propertyName of ownPropertyNames) {
            if (!this._isPreviewableObjectInternal(object[propertyName], knownObjects, depth))
                return false;
        }

        return true;
    },

    _abbreviateString: function(string, maxLength, middle)
    {
        if (string.length <= maxLength)
            return string;

        if (middle) {
            var leftHalf = maxLength >> 1;
            var rightHalf = maxLength - leftHalf - 1;
            return string.substr(0, leftHalf) + "\u2026" + string.substr(string.length - rightHalf, rightHalf);
        }

        return string.substr(0, maxLength) + "\u2026";
    }
}

InjectedScript.CallFrameProxy = function(ordinal, callFrame)
{
    this.callFrameId = "{\"ordinal\":" + ordinal + ",\"injectedScriptId\":" + injectedScriptId + "}";
    this.functionName = (callFrame.type === "function" ? callFrame.functionName : "");
    this.location = {scriptId: String(callFrame.sourceID), lineNumber: callFrame.line, columnNumber: callFrame.column};
    this.scopeChain = this._wrapScopeChain(callFrame);
    this.this = injectedScript._wrapObject(callFrame.thisObject, "backtrace");
}

InjectedScript.CallFrameProxy.prototype = {
    _wrapScopeChain: function(callFrame)
    {
        var scopeChain = callFrame.scopeChain;
        var scopeChainProxy = [];
        for (var i = 0; i < scopeChain.length; i++)
            scopeChainProxy[i] = InjectedScript.CallFrameProxy._createScopeJson(callFrame.scopeType(i), scopeChain[i], "backtrace");
        return scopeChainProxy;
    }
}

InjectedScript.CallFrameProxy._scopeTypeNames = {
    0: "global", // GLOBAL_SCOPE
    1: "local", // LOCAL_SCOPE
    2: "with", // WITH_SCOPE
    3: "closure", // CLOSURE_SCOPE
    4: "catch", // CATCH_SCOPE
    5: "functionName", // FUNCTION_NAME_SCOPE
}

InjectedScript.CallFrameProxy._createScopeJson = function(scopeTypeCode, scopeObject, groupId)
{
    return {
        object: injectedScript._wrapObject(scopeObject, groupId),
        type: InjectedScript.CallFrameProxy._scopeTypeNames[scopeTypeCode]
    };
}


function slice(array, index)
{
    var result = [];
    for (var i = index || 0; i < array.length; ++i)
        result.push(array[i]);
    return result;
}

function bind(func, thisObject, var_args)
{
    var args = slice(arguments, 2);
    return function(var_args) {
        return func.apply(thisObject, args.concat(slice(arguments)));
    }
}

function BasicCommandLineAPI()
{
    this.$_ = injectedScript._lastResult;
    this.$exception = injectedScript._exceptionValue;

    // $1-$99
    for (var i = 1; i <= injectedScript._savedResults.length; ++i) {
        var member = "$" + i;
        if (member in inspectedGlobalObject)
            continue;
        this.__defineGetter__("$" + i, bind(injectedScript._savedResult, injectedScript, i));
    }
}

return injectedScript;
})
