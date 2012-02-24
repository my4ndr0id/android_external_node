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

// secureproxy.js

"use strict";

// Same as 'wrapObject' (see below) but without caching.
//
function createProxy(obj)
{
    var proxy = {};

    // Add a wrapper for the 'name' property to 'proxy'.
    // 'type' should be one of: "method", "ro", "rw".
    function wrap(name, type) {
        var fn;
        var desc = {
            enumerable: true,
            configurable: false
        };

        if (type === "method") {
            fn = obj[name];
            desc.value = function () {
                return fn.apply(obj, arguments);
            };
            desc.writable =  false;
        } else {
            desc.get = function () {
                return obj[name];
            };
            if (type === "rw")  {
                desc.set = function (newValue) {
                    obj[name] = newValue;
                };
            }
        }
        Object.defineProperty(proxy, name, desc);
    }

    function wrapAll(properties, typ) {
        for (var ndx in properties) {
            wrap(properties[ndx], typ);
        }
    }

    wrapAll(obj.exportedMethods, "method");
    wrapAll(obj.exportedROProps, "ro");
    wrapAll(obj.exportedRWProps, "rw");

    return proxy;
}


// Create a proxy object that wraps obj.
//
// A proxy object allows access to certain "exported" members and does not
// otherwise allow access to the wrapped object.  Exported members are
// given by:
//
//   obj.exportedMethods = array of method names (if defined).
//   obj.exportedROProps = array of read-only property names (if defined).
//   obj.exportedRWProps = array of read-write property names (if defined).
//
// Exported methods will be callable via the proxy, and exported value
// properties are readable (and optionally writable) via the proxy.  Proxied
// methods are guaranteed that their 'this' parameter will be properly
// set when called via the proxy.
//
// createProxy() stores the created proxy in obj.secureProxy.  If called
// multiple times on the same object it will return the same proxy.
//
function wrapObject(obj)
{
    return obj.secureProxy || (obj.secureProxy = createProxy(obj));
}


exports.wrapObject = wrapObject;
