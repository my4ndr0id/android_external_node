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
var validate = require('validate'),
    modutil = require('modloaderutil'),
    util = require('util'),
    webapp = require("webapp"),
    querystring = require("querystring"),
    deviceInfo = require("proteusDeviceInfo");

var downloadModule = function (moduleName, successCB, failureCB) {
    try {
        var encodedQueryStr = encodeURIComponent(querystring.stringify(deviceInfo.getDeviceInfo()) + '&Module=' + moduleName);
        var requestUrl = modutil.getProperty("serverURL") + '/getModule?' + encodedQueryStr; // www.qualcomm-xyz.com/getModule?AV=4.0&PV=1.0.0&Module=xyz
        modutil.networkRequest(requestUrl, modutil.getProperty("clientConnTimeout"), function (data) {
            modutil.writeModuleToFS(data, moduleName, successCB, failureCB);
        }, failureCB);
    } catch (ex) {
        console.error("downloadModule : "+ ex);
        failureCB(modutil.createError("IO_ERR", "Decode URI error"));
    }
};

var getLatestVersions = function (modules, callback) {
    try {
        var encodedQueryStr = encodeURIComponent(querystring.stringify(deviceInfo.getDeviceInfo()) + '&Modules=' + modules.join(','));
        var requestUrl = modutil.getProperty("serverURL") + '/getVersions?' + encodedQueryStr;
        modutil.networkRequest(requestUrl, 5000, function (modules, statusCode) {
            callback(true, modules);
        }, function (result) {
            callback(false);
        });
    } catch (ex) {
        console.error("GetVersion Failed : " + ex);
        callback(false);
    }
};

var checkUpdatesComplete = function (callback) {
    process.setModuleUpdates(2);
    process.releaseLock();
    callback();
};

var getDecimal = function (str){
    return isNaN(str) ? 0 : parseInt(str, 10);
};

var needsUpdate = function (callback) {
    var lastCheck = modutil.readUpdateStatus();
    var mods = modutil.getDownloadedModules();
    if (mods.length <= 0 ) {
        checkUpdatesComplete(callback);
        return false;
    }
    if (!lastCheck || (getDecimal(Date.now().toString())) > (getDecimal(lastCheck) + modutil.getProperty("clientUpdatePeriod"))) {
        return true;
    }
    else {
        //Updates check not required either no modules or update period not reached
        checkUpdatesComplete(callback);
        return false;
    }
};

var checkUpdates = function (callback) {
    var mods = modutil.getDownloadedModules();
    if (needsUpdate(callback)) {
        getLatestVersions(mods, function (result, versions) {
            if (result) {
                var jsonObj = JSON.parse(versions);
                var moduleVersions = jsonObj.versionList;
                for (var n = 0; n < mods.length; ++n) {
                    if (moduleVersions[n] !== null && modutil.versionIsNewer(moduleVersions[n], modutil.getModuleVersion(mods[n]))) {
                        console.info("Module : " + mods[n] + " Local Version : " + modutil.getModuleVersion(mods[n])+ " Server Version : " + moduleVersions[n]);
                        modutil.deleteModule(mods[n]);
                    }
                }
                // update the timestamp for versioncheck
                modutil.writeUpdateStatus(Date.now());
            }
            checkUpdatesComplete(callback);
        });
    }
};

// gives an array of modules that needs to be downloaded
var filterDownloadedMods = function (modlist) {
    if (modlist) return modlist.filter(function (element) {
        return !modutil.isPkgAvailable(element);
    });
};

var workOnDependencies = function (lookup, downloadedModules, continueCb, failedDownload) {
    var curr = lookup.shift();
    var newLookup = lookup.concat(modutil.getModuleDependencies(curr));
    newLookup = filterDownloadedMods(lookup);
    continueCb(newLookup, downloadedModules);
};

var workOnModule = function (lookup, downloadedModules, continueCb, errorCb) {
    process.acquireLock(function () {
        console.info("Start Download : " + lookup[0] );
        downloadModule(lookup[0], function () {
            console.info("Download Complete : " + lookup[0] );
            process.releaseLock();
            modutil.addModule(lookup[0]);
            downloadedModules.push(lookup[0]);
            var module = lookup.shift();
            var deps = modutil.getModuleDependencies(module);
            if (deps && deps.length > 0) {
                lookup.unshift(deps.toString());
            }
            var newLookup = filterDownloadedMods(lookup);
            continueCb(newLookup, downloadedModules);
        }, function (err) {
            console.error("Download failed : " + lookup + " downloadedModules : " + downloadedModules + " error : " + err);
            process.releaseLock();
            errorCb(lookup, downloadedModules, err);
        });
    });
};

function getPackage(pkgName, sucessCb, errorCb) {
    var lookedUpMods = [pkgName];
    var fetchedMods = [];
    var failedDownload = function (lookup, downloadedModules, err) {
        console.error("Failed " + lookup + "downloadedModules" + downloadedModules +" error : " + err);
        // clean up downloaded Modules
        downloadedModules.forEach(modutil.deleteModule);
        errorCb(err);
    };
    var checkOrDownload = function (checkMods, downloadedMods) {
        console.info("in checkOrDownload : " + checkMods + " downloadedMods : "+ downloadedMods);
        if (checkMods && checkMods.length > 0) {
          if (modutil.isPkgAvailable(checkMods[0])) {
              workOnDependencies(checkMods, downloadedMods, checkOrDownload, failedDownload);
          } else {
              workOnModule(checkMods, downloadedMods, checkOrDownload, failedDownload);
          }
        } else {
            console.info("Fully loaded : " + pkgName + " Downloaded Mods : " + downloadedMods);
            sucessCb();
        }
      };
    checkOrDownload(lookedUpMods, fetchedMods);
}

var loadPackage = function (pkgName, successCB, errorCB) {
    try {
        errorCB = validate.asFunction(errorCB);
        successCB = validate.asFunction(successCB);
        pkgName = validate.asString(pkgName);
        var loadModule = function () {
            getPackage(pkgName, successCB, errorCB);
        };
        modutil.createDB(); // build the db of modules
        if (process.getModuleUpdates() === 0) {
            process.setModuleUpdates(1); //set the flag to inprogress state
            process.acquireLock(checkUpdates, loadModule);
        } else {
            loadModule();
        }
    } catch (err) {
        if (typeof errorCB === 'function') {
          return webapp.callback(errorCB, err);
        }
    }
};
exports.loadPackage = loadPackage;
exports.checkUpdates = checkUpdates;
exports.modutil = modutil; // exposed just for tests
