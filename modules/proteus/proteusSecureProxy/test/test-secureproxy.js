// ----------------------------------------------------------------
// test for secureproxy.js
// ----------------------------------------------------------------

var sp = require('secureproxy');
var assert = require('assert');
var expect = assert.strictEqual;

//--------------------------------
// PrivateClass
//--------------------------------

var PrivateClass = function () {
    this.priv = 1;
    this.roVar = 2;
    this.rwVar = 3;
};

PrivateClass.prototype.f1 = function () {
    return 1;
};

PrivateClass.prototype.f2 = function (a, b, c) {
    return a * b - c + this.priv;
};

PrivateClass.prototype.fpriv = function () {
    return 2;
};

PrivateClass.prototype.exportedMethods = ['f1', 'f2'];
PrivateClass.prototype.exportedROProps = ['roVar'];
PrivateClass.prototype.exportedRWProps = ['rwVar'];

//--------------------------------
// instantiate and access via proxy...
//--------------------------------

var o = new PrivateClass;
var p = sp.wrapObject(o);

// ASSERT: calling wrapObject again should return the SAME object
expect(sp.wrapObject(o), p);

// ASSERT: public members should appear in p
expect(typeof p.f1, 'function');
expect(typeof p.f2, 'function');

// ASSERT: function wrapping: check argument passing and return values
expect(1, p.f1());
expect(3, p.f2(2,3,4));

// ASSERT: RO property reflects changes to wrapped object
o.roVar = 7;            // original object reference can get/set
expect(p.roVar, 7);     // proxy gets

// ASSERT: RO property cannot be modified
var caught=false, e;
try { p.roVar = 9; } catch (e) { caught = true; }
expect(caught, true);
expect(o.roVar, 7);

// ASSERT: RW property can be modified
p.rwVar = 23;
expect(o.rwVar, 23);

// ASSERT: private members of o should not appear in p
expect(p.fpriv,  undefined);
expect(p.priv, undefined);

// ASSERT: changing exported method on proxy should not should not affect o
p.f1 = null;
expect(typeof o.f1, 'function');
p.a = 1;
expect(o.a, undefined);

// ASSERT: constructor or prototype of wrapped object should not be accessible
assert.notEqual(p.constructor, o.constructor);
assert.notEqual(Object.getPrototypeOf(p), Object.getPrototypeOf(o));
