/*
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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

// 'validate' module
//
// Use these functions to promote consistent handling of arguments in public
// APIs.  Where applicable, we normalize values as specified in WebIDL and
// throw exceptions in the cases specified in WebIDL.
//
// Function          WebIDL Type
// -------------     -----------------
// asBoolean()       boolean
// asNumber()        double
// asString()        DOMString
// asDate()          Date
// asFunction()      callback types
// asObject()        dictionary types
//
// Functions named `asValidXXX` perform additional checks for convenience.
//
// Not all of these functions perform type conversion, but all return a
// value so clients can follow this convention uniformly:
//
//    arg1 = validate.asTYPE(arg1);
//


var toString = Object.prototype.toString;

function error(name, extra) {
    // WebIDL ==> throw new TypeError('Invalid Argument Value Provided');
    var e = new Error('Invalid Argument Value Provided');
    e.name = 'INVALID_VALUES_ERR';
    throw e;
}


// Call this to throw INVALID_VALUES_ERR
//
exports.error = error;


exports.asBoolean = function (v) {
    return ! ! v;
};


exports.asNumber = function (v) {
    return Number(v);
};


exports.asString = function (v) {
    return String(v);
};


exports.asFunction = function (v) {
    if (typeof v !== 'function') {
        error();
    }
    return v;
};


exports.asObject = function (v) {
    if (v === null || typeof v !== 'object') {
        error();
    }
    return v;
};


// WebIDL "Date" conversion requires caller to pass a JavaScript Date.
// <http://www.w3.org/TR/2012/WD-WebIDL-20120207/#es-Date>
//
exports.asDate = function (v) {
    if (toString.call(v) !== '[object Date]') {
        error();
    }
    return v;
};


// Note: this assumes that NaN, +Inf and -Inf are invalid, which
// is not strictly the case for IDL 'floats'.
//
exports.asValidNumber = function (v) {
    v = Number(v);
    if (isNaN(v) || v === Infinity || v === -Infinity) {
        error();
    }
    return v;
};


exports.asValidDate = function (v) {
    v = exports.asDate(v);
    if (isNaN(Number(v))) {
        error();
    }
    return v;
};


// This function simplifies handling the common case where a value may be
// either null or some other type.  If the value is null (or undefined)
// the return value is defaultValue (or null if defaultValue is not
// specified).
//
// Usage:
//     arg = validate.asNullOr(validate.asDate, arg, [defaultValue]);
//
exports.asNullOr = function(fn, v, defaultValue) {
    if (v === null || v === undefined) {
        // normalize null & undefined to null
        return (arguments.length > 2 ? defaultValue : null);
    }
    return fn(v);
};

