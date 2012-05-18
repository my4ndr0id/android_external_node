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
"use strict";
var https = require("https"),
  url = require("url"),
  validate = require('validate'),
  path = require("path"),
  fs = require("fs"),
  util = require("util"),
  packageExtractor = require("proteusPackageExtractor"),
  proteusConfig = require("proteusConfig"),
  net = require('net');

// Constants
var PROTEUS_PATH = process.downloadPath + '/';
var TEMP_PATH = process.downloadPath + '/temp/';
var PERM = 448;
var UPDATE_FILE = process.downloadPath + '/lastUpdate.log';

var localSetting = {
  "serverURL": "https://DAPIProd.quicinc.com",
  "serverPort": "443",
  "clientConnTimeout": 15000, // 15 seconds
  "clientUpdatePeriod": 1209600000, //2 weeks
  "ca": ["-----BEGIN CERTIFICATE-----\r\nMIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB\r\nyjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\r\nExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\r\nU2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\r\nZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\r\naG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL\r\nMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\r\nZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln\r\nbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp\r\nU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y\r\naXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1\r\nnmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex\r\nt0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz\r\nSdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG\r\nBO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+\r\nrCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/\r\nNIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E\r\nBAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH\r\nBgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy\r\naXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv\r\nMzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE\r\np6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y\r\n5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK\r\nWE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ\r\n4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N\r\nhnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq\r\n-----END CERTIFICATE-----\r\n"]
};

var modulesDB = {};

// Method for rm -r
var rmdirRSync = function (dirRPath) {
    function deleteFile(filePath) {
        fs.unlinkSync(filePath);
    }

    function deleteDirectory(dirPath) {
        var files, file, filePath, i;
        files = fs.readdirSync(dirPath);
        for (i in files) {
            filePath = dirPath + "/" + files[i];
            file = fs.statSync(filePath);
            if (file.isFile() || file.isSymbolicLink()) {
                deleteFile(filePath);
            } else if (file.isDirectory()) {
                deleteDirectory(filePath);
            }
        }
        // delete the directory
        fs.rmdirSync(dirPath);
    }
    if (path.existsSync(dirRPath)) {
        deleteDirectory(dirRPath);
    }
};

//  Method to create dir recursively
var mkdirsRSync = function (dirPath, mode) {
    var dirs = dirPath.split("/");
    var subpath = '';
    while (dirs.length) {
        subpath += dirs.shift() + '/';
        try {
            fs.mkdirSync(subpath, mode);
        } catch (e) { }
    }
};

var getKeys = function (obj) {
    var names = [];
    if (!isEmptyObject(obj)) {
        for (var prop in obj) {
            names.push(prop);
        }
    }
    return names;
};

// method to get configuration
var getProperty = function (propertyName, useConfigOnly) {
    propertyName = validate.asString(propertyName);
    var proteusConfigObj = new proteusConfig();
    var configFileValue;
    var configObj = proteusConfigObj.getConfig();
    if (configObj && typeof configObj.modLoader === 'object') {
        configFileValue = configObj.modLoader[propertyName];
        if (useConfigOnly)
          return configFileValue;
    }
    return configFileValue ? configFileValue : localSetting[propertyName];
};

var getModuleProperties = function (moduleName) {
    var prop = {};
    try {
        // try the public
        var publicModuleConfigPath = PROTEUS_PATH + moduleName + '/package.json';
        if (path.existsSync(publicModuleConfigPath)) {
            var jsonBuffer = JSON.parse(fs.readFileSync(publicModuleConfigPath));
            if (!isEmptyObject(jsonBuffer)) {
              prop.version = jsonBuffer.version;
              prop.dependencies = getKeys(jsonBuffer.dependencies);
              console.info("getModuleProperties : Module : " + moduleName + " Version : " + prop.version + " Dependencies : " + prop.dependencies);
            }
        }
    } catch (ex) {
        console.error("getModuleProperties :" + ex);
    }
    return prop;
};

// Check if its a empty object
var isEmptyObject = function (obj) {
    if (obj) {
        for (var name in obj) {
            return false;
        }
    }
    return true;
};

// get all the downloadedModules
var createDB = function () {
    try {
        if (isEmptyObject(modulesDB)) {
            var files = fs.readdirSync(PROTEUS_PATH);
            for (var i in files) {
                var file = fs.statSync(PROTEUS_PATH + files[i]);
                if (file.isDirectory() === true) {
                    var prop = getModuleProperties(files[i]);
                    if (prop.version) {
                      modulesDB[files[i]] = prop;
                    }
                }
            }
        }
    } catch (err) {
      console.error("createDB : Error :" + err);
    }
};

var getDownloadedModules = function () {
    return getKeys(modulesDB);
};

var addModule = function (moduleName) {
    var moduleProp = getModuleProperties(moduleName);
    if (moduleProp.version)
        modulesDB[moduleName] = moduleProp;
};

var deleteModule = function (moduleName) {
    if(!isEmptyObject(modulesDB[moduleName])) {
      delete modulesDB[moduleName];
      rmdirRSync(PROTEUS_PATH + moduleName);
    }
};

var getModuleVersion = function (moduleName) {
    return isEmptyObject(modulesDB[moduleName]) ? undefined : modulesDB[moduleName].version ;
};

var getModuleDependencies = function (moduleName) {
    return isEmptyObject(modulesDB[moduleName]) ? undefined : modulesDB[moduleName].dependencies;
};

var createError = function (err, msg) {
    var e = new Error(msg);
    e.name = err;
    return e;
};

var networkRequest = function (requestUrl, timeOut, successCB, failureCB) {
    var parsedURL = url.parse(requestUrl);
    var request = null;
    var clearTimeoutfn = null;
    var reqTimeoutObj = null;
    var abort = false;
    var options = {
        host: parsedURL.host,
        port: getProperty("serverPort"),
        path: parsedURL.pathname + parsedURL.search,
        method: 'GET'
    };
    var configCA = getProperty("ca", true);
    options.ca = configCA ? (configCA.length === 0 ? undefined : configCA) : localSetting['ca'];
    var abortNetworkReq = function () {
        if (request) {
            console.error("Abort network request ");
            clearTimeoutfn();
            request.abort();
            request = null;
            abort = true;
        }
    };
    var timeoutHandler = function () {
        reqTimeoutObj = null;
        console.error("Server Taking too long");
        abortNetworkReq();
        process.removeListener('exit', abortNetworkReq);
        failureCB(createError("NETWORK_ERR", "Network Request timeout"));
    };
    var failure = function(err){
        clearTimeoutfn();
        process.removeListener('exit', abortNetworkReq);
        failureCB(err);
    };
    request = https.request(options, function (response) {
        if (!response.client.authorized) {
            failure(createError("NETWORK_ERR", "SSL error"));
            return;
        }
        switch (response.statusCode) {
            case 200:
                break;
            case 302:
                clearTimeoutfn();
                reqTimeoutObj = setTimeout(timeoutHandler, timeOut );
                var redirectedRemote = response.headers.location;
                networkRequest(redirectedRemote, successCB, failureCB);
                return;
            case 404:
                failure(createError("NOT_FOUND_ERR", "Module Not Found"));
                break;
            default:
                console.error("networkRequest Error : " + response.statusCode);
                failure(createError("NETWORK_ERR", "Invalid Request "));
                return;
        }
        var chunk = '';
        response.on('data', function (data) {
            chunk += data.toString('binary', 0, data.length);
        });
        response.on('end', function () {
            if (response.statusCode === 200) {
                clearTimeoutfn();
                process.removeListener('exit', abortNetworkReq);
                if (abort === false) {
                    successCB(chunk, response.statusCode);
                }
            }
        });
    });
    request.end();
    clearTimeoutfn = function () {
        if (reqTimeoutObj) {
            clearTimeout(reqTimeoutObj);
            reqTimeoutObj = null;
        }
    };
    reqTimeoutObj = setTimeout(timeoutHandler, timeOut);
    request.on('error', function (e) {
        console.error("networkRequest Error : " + e);
        failure(createError("NETWORK_ERR", "Network Request Error"));
    });
    // if the request is being cancelled cleanup
    process.on('exit', abortNetworkReq);
};

//
var writeModuleToFS = function (data, moduleName, successCB, failureCB) {
    try {
        var downloadPath = TEMP_PATH + moduleName + ".crx";
        if (path.existsSync(TEMP_PATH) === false) {
            fs.mkdirSync(TEMP_PATH, PERM);
        }
        deleteFile(downloadPath);
        fs.writeFileSync(downloadPath, data, 'binary');
    } catch(ex) {
        console.error("writeModuleToFS : " +ex);
        failureCB(createError("IO_ERR", "Cannot write to file"));
    }
    packageExtractor.extract(downloadPath, moduleName, successCB, failureCB);
};

// Compare versions
var versionIsNewer = function (verA, verB) {
    var a = /([^\.]*)\.?(.*)/.exec(verA);
    var b = /([^\.]*)\.?(.*)/.exec(verB);
    var an = Number(a[1]);
    var bn = Number(b[1]);
    return (an > bn ? true : an < bn ? false : b[2] && versionIsNewer(a[2], b[2]));
};

// Check id a module is available on the device
var isPkgAvailable = function (pkgName) {
    var avail = false;
    // Check file system always
    var prop = getModuleProperties(pkgName);
    if (prop.version) {
        modulesDB[pkgName] = prop;
        avail = true;
    }
    return avail;
};

// Function to read the last updated time
var readUpdateStatus = function () {
    if (path.existsSync(UPDATE_FILE)) {
        var time = fs.readFileSync(UPDATE_FILE, 'utf8');
        if (time && !(isNaN(time))) {
            return time;
        } else {
            deleteFile(UPDATE_FILE);
        }
    }
    return 0;
};

var deleteFile = function(filePath) {
    return path.existsSync(filePath) ? fs.unlinkSync(filePath) : 0;
};

// Function to write the update time stamp to file
var writeUpdateStatus = function (time) {
    deleteFile(UPDATE_FILE) ;// delete the log file file.
    fs.writeFileSync(UPDATE_FILE, time.toString());
};

if (path.existsSync(PROTEUS_PATH) === false) {
    mkdirsRSync(PROTEUS_PATH, PERM);
}
exports.readUpdateStatus = readUpdateStatus;
exports.writeUpdateStatus = writeUpdateStatus;
exports.isPkgAvailable = isPkgAvailable;
exports.versionIsNewer = versionIsNewer;
exports.networkRequest = networkRequest;
exports.getKeys = getKeys;
exports.createDB = createDB;
exports.getDownloadedModules = getDownloadedModules;
exports.getModuleDependencies = getModuleDependencies;
exports.getModuleVersion = getModuleVersion;
exports.getProperty = getProperty;
exports.writeModuleToFS = writeModuleToFS;
exports.rmdirRSync = rmdirRSync;
exports.mkdirsRSync = mkdirsRSync;
exports.createError = createError;
exports.createDB = createDB;
exports.addModule = addModule;
exports.deleteModule = deleteModule;
exports.getModuleProperties = getModuleProperties;
exports.isEmptyObject = isEmptyObject;
exports.deleteFile = deleteFile;
