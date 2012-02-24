/*
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
