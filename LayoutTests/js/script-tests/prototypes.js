description(
'Test prototypes of various objects and the various means to access them.'
);

shouldBe("('').__proto__", "String.prototype");
shouldBe("(0).__proto__", "Number.prototype");
shouldBe("([]).__proto__", "Array.prototype");
shouldBe("({}).__proto__", "Object.prototype");
shouldBe("(new Date).__proto__", "Date.prototype");
shouldBe("(new Number).__proto__", "Number.prototype");
shouldBe("(new Object).__proto__", "Object.prototype");
shouldBe("(new String).__proto__", "String.prototype");
shouldBe("Array.prototype.__proto__", "Object.prototype");
shouldBe("Date.prototype.__proto__", "Object.prototype");
shouldBe("Number.prototype.__proto__", "Object.prototype");
shouldBe("Object.prototype.__proto__", "null");
shouldBe("String.prototype.__proto__", "Object.prototype");
shouldBe("Array.__proto__", "Object.__proto__");
shouldBe("Date.__proto__", "Object.__proto__");
shouldBe("Number.__proto__", "Object.__proto__");
shouldBe("String.__proto__", "Object.__proto__");

shouldBe("Object.getPrototypeOf('')", "String.prototype");
shouldBe("Object.getPrototypeOf(0)", "Number.prototype");
shouldBe("Object.getPrototypeOf([])", "Array.prototype");
shouldBe("Object.getPrototypeOf({})", "Object.prototype");
shouldBe("Object.getPrototypeOf(new Date)", "Date.prototype");
shouldBe("Object.getPrototypeOf(new Number)", "Number.prototype");
shouldBe("Object.getPrototypeOf(new Object)", "Object.prototype");
shouldBe("Object.getPrototypeOf(new String)", "String.prototype");
shouldBe("Object.getPrototypeOf(Array.prototype)", "Object.prototype");
shouldBe("Object.getPrototypeOf(Date.prototype)", "Object.prototype");
shouldBe("Object.getPrototypeOf(Number.prototype)", "Object.prototype");
shouldBe("Object.getPrototypeOf(Object.prototype)", "null");
shouldBe("Object.getPrototypeOf(String.prototype)", "Object.prototype");
shouldBe("Object.getPrototypeOf(Array)", "Object.__proto__");
shouldBe("Object.getPrototypeOf(Date)", "Object.__proto__");
shouldBe("Object.getPrototypeOf(Number)", "Object.__proto__");
shouldBe("Object.getPrototypeOf(String)", "Object.__proto__");

shouldBeFalse("String.prototype.isPrototypeOf('')");
shouldBeFalse("Number.prototype.isPrototypeOf(0)");
shouldBeTrue("Array.prototype.isPrototypeOf([])");
shouldBeTrue("Object.prototype.isPrototypeOf({})");
shouldBeTrue("Date.prototype.isPrototypeOf(new Date)");
shouldBeTrue("Number.prototype.isPrototypeOf(new Number)");
shouldBeTrue("Object.prototype.isPrototypeOf(new Object)");
shouldBeTrue("String.prototype.isPrototypeOf(new String)");
shouldBeTrue("Object.prototype.isPrototypeOf(Array.prototype)");
shouldBeTrue("Object.prototype.isPrototypeOf(Date.prototype)");
shouldBeTrue("Object.prototype.isPrototypeOf(Number.prototype)");
shouldBeTrue("Object.prototype.isPrototypeOf(String.prototype)");
shouldBeTrue("Object.__proto__.isPrototypeOf(Array)");
shouldBeTrue("Object.__proto__.isPrototypeOf(Date)");
shouldBeTrue("Object.__proto__.isPrototypeOf(Number)");
shouldBeTrue("Object.__proto__.isPrototypeOf(String)");

shouldBeTrue("var wasSet = false; var o = { }; o.__defineGetter__(\"__proto__\", function() { wasSet = true }); o.__proto__; wasSet;");
shouldBeTrue("var wasSet = false; var o = { }; o.__defineSetter__(\"__proto__\", function() { wasSet = true }); o.__proto__ = {}; wasSet;");
shouldBeTrue("var wasSet = false; var o = { }; Object.defineProperty(o, \"__proto__\", { \"get\": function() { wasSet = true } }); o.__proto__; wasSet;");
shouldBeFalse("var wasSet = false; var o = { }; Object.defineProperty(o, \"__proto__\", { \"__proto__\": function(x) { wasSet = true } }); o.__proto__ = {}; wasSet;");

// Deleting Object.prototype.__proto__ removes the ability to set the object's prototype.
shouldBeTrue("var o = {}; o.__proto__ = { x:true }; o.x");
shouldBeFalse("var o = {}; o.__proto__ = { x:true }; o.hasOwnProperty('__proto__')");
delete Object.prototype.__proto__;
shouldBeUndefined("var o = {}; o.__proto__ = { x:true }; o.x");
shouldBeTrue("var o = {}; o.__proto__ = { x:true }; o.hasOwnProperty('__proto__')");
