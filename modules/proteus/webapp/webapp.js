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

var validate = require('validate');
var wrapper = process.evalInHostContext("(function (cb, args) { return cb.apply(null, args); })");

// evaluates in the host context, to be invoked for synchronous api in the host context
// e.g window.webkitURL.createObjectURL(..) would be invoked as
// webapp.eval("window.webkitURL.createObjectURL()"
// if you need to pass arguments from the current lexical scope
// var fn = webapp.eval("(function(arg0) { window.webkitURL.createObjectURL(arg0); })");
// fn(arg); // where arg is available in current scope
// note window prefix is redundant, you could as well do
// webapp.eval("webkitURL.createObjectURL()")
exports.evalInHost = function (str) {
    return process.evalInHostContext(validate.asString(str));
};

exports.callback = function (cb) {
    cb = validate.asFunction(cb);

    // Create a function wrapper in the host context and invoke the wrapper
    // This ensures that the function "fn" gets invoked with the host context as the
    // current/calling/entered context
    var args = arguments;
    process.nextTick( function () { process.callInHostContext(wrapper, cb, [].slice.call(args, 1));} );
};

exports.wrapCallback = function (cb) {
    cb = validate.asFunction(cb);

    // Create a function wrapper in the host context and invoke the wrapper
    // This ensures that the function "fn" gets invoked with the host context as the
    // current/calling/entered context
    return function () {
        // prepend cb to the arguments lists and invoke webapp.callback
        var args = [].slice.call(arguments);
        process.nextTick( function () { process.callInHostContext(wrapper, cb, args);} );
    };

};

exports.newFunc = function (func) {
    if (typeof func === 'string') {
        func = process.evalInHostContext(func);
    }

    func = validate.asFunction(func);

    // Create a function wrapper in the host context and invoke the wrapper
    // This ensures that the function "fn" gets invoked with the host context as the
    // current/calling/entered context
    return function () {
        // prepend cb to the arguments lists and invoke webapp.callback
        var args = [].slice.call(arguments);
        return process.callInHostContext(wrapper, func, args);
    };
};
