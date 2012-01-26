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
"use strict";
//var unzipWrapBindings = require('./deviceinfo.node'); //ucomment to make the mod dynamic
var deviceInfoBindings = process.binding('deviceinfo');

var deviceInfoPropertyList = {
    "ro.product.manufacturer": "manu",
    "ro.product.device": "pdev",
    "ro.product.model": "mdl",
    "ro.product.name": "pname",
    "ro.hardware": "hwd",
    "ro.build.display.id": "bld",
    "ro.carrier": "cr",
    "ro.product.locale.language": "lang",
    "ro.product.locale.region": "reg",
    "ro.product.board": "pbrd",
    "ro.product.brand": "pbnd",
    "ro.board.platform": "brdplat"
};

var environmentPropertyList = {
    "DIRECTORY_MUSIC": "Music",
    "DIRECTORY_ALARMS": "Alarms",
    "DIRECTORY_DCIM": "DCIM",
    "DIRECTORY_DOWNLOADS": "Download",
    "DIRECTORY_MOVIES": "Movies",
    "DIRECTORY_NOTIFICATIONS": "Notifications",
    "DIRECTORY_PICTURES": "Pictures",
    "DIRECTORY_PODCASTS": "Podcasts",
    "DIRECTORY_RINGTONES": "Ringtones"
};

exports.getSystemProperty = function (propertyName) {
    var deviceInfoObj = new deviceInfoBindings.createDeviceInfo(process);
    var value = deviceInfoObj.getSystemProp(propertyName);
    return value;
};

exports.getEnvironmentProperty = function (propertyName, isPrivate) {
    if (environmentPropertyList[propertyName]) {
        var deviceInfoObj = new deviceInfoBindings.createDeviceInfo(process);
        var value = deviceInfoObj.getEnvironmentProp(environmentPropertyList[propertyName]);
        return value;
    } else {
        throw "Not a valid Environment Property";
    }
};

exports.getDeviceInfo = function () {
    var deviceInfo = {};
    for (var i in deviceInfoPropertyList) {
        var deviceInfoObj = new deviceInfoBindings.createDeviceInfo(process);
        var value = deviceInfoObj.getSystemProp(i);
        if (!value) {
            value = '';
        }
        deviceInfo[deviceInfoPropertyList[i]] = value;
    }
    console.info("getDeviceInfo -> " + JSON.stringify(deviceInfo));
    return deviceInfo;
};
