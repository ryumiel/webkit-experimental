description("Test to ensure correct behaviour of Object.getOwnPropertyNames");

function argumentsObject() { return arguments; };

var expectedPropertyNamesSet = {
    "{}": "[]",
    "{a:null}": "['a']",
    "{a:null, b:null}": "['a', 'b']",
    "{b:null, a:null}": "['a', 'b']",
    "{__proto__:{a:null}}": "[]",
    "{__proto__:[1,2,3]}": "[]",
    "Object.create({}, { 'a': { 'value': 1, 'enumerable': false } })": "['a']",
    "Object.create([1,2,3], { 'a': { 'value': 1, 'enumerable': false } })": "['a']",
// Function objects
    "new Function()": "['arguments', 'caller', 'length', 'name', 'prototype']",
    "(function(){var x=new Function();x.__proto__=[1,2,3];return x;})()": "['arguments', 'caller', 'length', 'name', 'prototype']",
// String objects
    "new String('')": "['length']",
    "new String('a')": "['0', 'length']",
    "new String('abc')": "['0', '1', '2', 'length']",
    "(function(){var x=new String('');x.__proto__=[1,2,3];return x;})()": "['length']",
// Array objects
    "[]": "['length']",
    "[null]": "['0', 'length']",
    "[null,null]": "['0','1', 'length']",
    "[null,null,,,,null]": "['0','1','5', 'length']",
    "(function(){var x=[];x.__proto__=[1,2,3];return x;})()": "['length']",
// Date objects
    "new Date()": "[]",
    "(function(){var x=new Date();x.__proto__=[1,2,3];return x;})()": "[]",
// RegExp objects
    "new RegExp('foo')": "['global', 'ignoreCase', 'lastIndex', 'multiline', 'source']",
    "(function(){var x=new RegExp();x.__proto__=[1,2,3];return x;})()": "['global', 'ignoreCase', 'lastIndex', 'multiline', 'source']",
// Arguments objects
     "argumentsObject()": "['callee', 'length']",
     "argumentsObject(1)": "['0', 'callee', 'length']",
     "argumentsObject(1,2,3)": "['0', '1', '2', 'callee', 'length']",
    "(function(){arguments.__proto__=[1,2,3];return arguments;})()": "['callee', 'length']",
// Built-in ECMA functions
    "parseInt": "['length', 'name']",
    "parseFloat": "['length', 'name']",
    "isNaN": "['length', 'name']",
    "isFinite": "['length', 'name']",
    "escape": "['length', 'name']",
    "unescape": "['length', 'name']",
    "decodeURI": "['length', 'name']",
    "decodeURIComponent": "['length', 'name']",
    "encodeURI": "['length', 'name']",
    "encodeURIComponent": "['length', 'name']",
// Built-in ECMA objects
    "Object": "['create', 'defineProperties', 'defineProperty', 'freeze', 'getOwnPropertyDescriptor', 'getOwnPropertyNames', 'getPrototypeOf', 'isExtensible', 'isFrozen', 'isSealed', 'keys', 'length', 'name', 'preventExtensions', 'prototype', 'seal']",
    "Object.prototype": "['__defineGetter__', '__defineSetter__', '__lookupGetter__', '__lookupSetter__', '__proto__', 'constructor', 'hasOwnProperty', 'isPrototypeOf', 'propertyIsEnumerable', 'toLocaleString', 'toString', 'valueOf']",
    "Function": "['length', 'name', 'prototype']",
    "Function.prototype": "['apply', 'bind', 'call', 'constructor', 'length', 'name', 'toString']",
    "Array": "['from', 'isArray', 'length', 'name', 'of', 'prototype']",
    "Array.prototype": "['concat', 'constructor', 'entries', 'every', 'fill', 'filter', 'find', 'findIndex', 'forEach', 'includes', 'indexOf', 'join', 'keys', 'lastIndexOf', 'length', 'map', 'pop', 'push', 'reduce', 'reduceRight', 'reverse', 'shift', 'slice', 'some', 'sort', 'splice', 'toLocaleString', 'toString', 'unshift']",
    "String": "['fromCharCode', 'length', 'name', 'prototype']",
    "String.prototype": "['anchor', 'big', 'blink', 'bold', 'charAt', 'charCodeAt', 'concat', 'constructor', 'endsWith', 'fixed', 'fontcolor', 'fontsize', 'includes', 'indexOf', 'italics', 'lastIndexOf', 'length', 'link', 'localeCompare', 'match', 'repeat', 'replace', 'search', 'slice', 'small', 'split', 'startsWith', 'strike', 'sub', 'substr', 'substring', 'sup', 'toLocaleLowerCase', 'toLocaleUpperCase', 'toLowerCase', 'toString', 'toUpperCase', 'trim', 'trimLeft', 'trimRight', 'valueOf']",
    "Boolean": "['length', 'name', 'prototype']",
    "Boolean.prototype": "['constructor', 'toString', 'valueOf']",
    "Number": "['EPSILON', 'MAX_SAFE_INTEGER', 'MAX_VALUE', 'MIN_SAFE_INTEGER', 'MIN_VALUE', 'NEGATIVE_INFINITY', 'NaN', 'POSITIVE_INFINITY', 'isFinite', 'isInteger', 'isNaN', 'isSafeInteger', 'length', 'name', 'parseFloat', 'parseInt', 'prototype']",
    "Number.prototype": "['clz', 'constructor', 'toExponential', 'toFixed', 'toLocaleString', 'toPrecision', 'toString', 'valueOf']",
    "Date": "['UTC', 'length', 'name', 'now', 'parse', 'prototype']",
    "Date.prototype": "['constructor', 'getDate', 'getDay', 'getFullYear', 'getHours', 'getMilliseconds', 'getMinutes', 'getMonth', 'getSeconds', 'getTime', 'getTimezoneOffset', 'getUTCDate', 'getUTCDay', 'getUTCFullYear', 'getUTCHours', 'getUTCMilliseconds', 'getUTCMinutes', 'getUTCMonth', 'getUTCSeconds', 'getYear', 'setDate', 'setFullYear', 'setHours', 'setMilliseconds', 'setMinutes', 'setMonth', 'setSeconds', 'setTime', 'setUTCDate', 'setUTCFullYear', 'setUTCHours', 'setUTCMilliseconds', 'setUTCMinutes', 'setUTCMonth', 'setUTCSeconds', 'setYear', 'toDateString', 'toGMTString', 'toISOString', 'toJSON', 'toLocaleDateString', 'toLocaleString', 'toLocaleTimeString', 'toString', 'toTimeString', 'toUTCString', 'valueOf']",
    "RegExp": "['$&', \"$'\", '$*', '$+', '$1', '$2', '$3', '$4', '$5', '$6', '$7', '$8', '$9', '$_', '$`', 'input', 'lastMatch', 'lastParen', 'leftContext', 'length', 'multiline', 'name', 'prototype', 'rightContext']",
    "RegExp.prototype": "['compile', 'constructor', 'exec', 'global', 'ignoreCase', 'lastIndex', 'multiline', 'source', 'test', 'toString']",
    "Error": "['length', 'name', 'prototype']",
    "Error.prototype": "['constructor', 'message', 'name', 'toString']",
    "Math": "['E','LN10','LN2','LOG10E','LOG2E','PI','SQRT1_2','SQRT2','abs','acos','acosh','asin','asinh','atan','atan2','atanh','cbrt','ceil','cos','cosh','exp','expm1','floor','fround','hypot','imul','log','log10','log1p','log2','max','min','pow','random','round','sign','sin','sinh','sqrt','tan','tanh','trunc']",
    "JSON": "['parse', 'stringify']"
};

function getSortedOwnPropertyNames(obj)
{
    return Object.getOwnPropertyNames(obj).sort();
}

for (var expr in expectedPropertyNamesSet)
    shouldBe("getSortedOwnPropertyNames(" + expr + ")", expectedPropertyNamesSet[expr]);

// Global Object
// Only check for ECMA properties here
var globalPropertyNames = Object.getOwnPropertyNames(this);
var expectedGlobalPropertyNames = [
    "NaN",
    "Infinity",
    "undefined",
    "parseInt",
    "parseFloat",
    "isNaN",
    "isFinite",
    "escape",
    "unescape",
    "decodeURI",
    "decodeURIComponent",
    "encodeURI",
    "encodeURIComponent",
    "Object",
    "Function",
    "Array",
    "String",
    "Boolean",
    "Number",
    "Date",
    "RegExp",
    "Error",
    "Math",
    "JSON"
];

for (var i = 0; i < expectedGlobalPropertyNames.length; ++i)
    shouldBeTrue("globalPropertyNames.indexOf('" + expectedGlobalPropertyNames[i] + "') != -1");
