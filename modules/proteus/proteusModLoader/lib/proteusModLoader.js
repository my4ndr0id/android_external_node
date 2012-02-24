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
    path = require("path"),
    fs = require("fs"),
    sys = require("sys"),
    packageExtractor = require("proteusPackageExtractor"),
    deviceInfo = require("proteusDeviceInfo");

module.exports = proteusModManager;

// Constants
var PROTEUS_PATH = process.downloadPath + '/';
var TEMP_PATH = process.downloadPath + '/temp/';
var UPDATE_FILE = process.downloadPath + '/lastUpdate.log';
var PERM = 448;

function proteusModManager() {
    var request = null;
    var localSetting = {
        "serverURL": "https://DAPIProd.quicinc.com",
        "serverPort": "443",
        "clientConnTimeout": 5000,
        "clientUpdatePeriod": 1209600000,
        "clientRetryTime": 500,
        "clientCoreModulesUpgrade": []
    };
    var consts = {
        NETWORK_ERR: "TYPE_NETWORK_ERR",
        NETWORK_ERR_MSG: "Network error",
        TYPE_MISMATCH_ERR: "TYPE_MISMATCH_ERR",
        TYPE_MISMATCH_ERR_MSG: "Type mismatch error - unexpected parameter",
        INVALID_VALUES_ERR: "INVALID_VALUES_ERR",
        INVALID_VALUES_ERR_MSG: "Invalid argument value provided",
        NOT_FOUND_ERR: "NOT_FOUND_ERR",
        MODULE_NOT_FOUND_ERR_MSG: "Module not found",
        UNKNOWN_ERR: "UNKNOWN_ERR",
        UNKNOWN_ERR_MSG: "Unknown error has occurred"
    };
    var proteusConfig = require("proteusConfig");

    function getProperty(propertyName) {
        var value = localSetting[propertyName];
        var proteusConfigObj = new proteusConfig();
        var configObj = proteusConfigObj.getConfig();
        if (configObj && typeof configObj.modLoader === 'object') {
            var proteusModLoaderConfig = configObj.modLoader;
            var configFileValue = proteusModLoaderConfig[propertyName];
            if (configFileValue) {
                value = configFileValue;
            }
        }
        console.info("Config [" + propertyName + "] => [" + value + "]");
        return value;
    }

    function getDeviceInfo() {
        var info = "";
        var deviceInfoObj = deviceInfo.getDeviceInfo();
        for (var i in deviceInfoObj) {
            info += i + "=" + deviceInfoObj[i] + "&";
        }
        console.info("Device Info : " + info);
        return info;
    }
    var rmdirRSync = fs.rmdirRSync;

    function getDownloadedModules() {
        try {
            var files = fs.readdirSync(PROTEUS_PATH);
            var modulesOnFS = [];
            for (var i in files) {
                var filePath = PROTEUS_PATH + files[i];
                var file = fs.statSync(filePath);
                if (file.isDirectory() === true) {
                    modulesOnFS.push(files[i]);
                }
            }
            console.info("getDownloadedModules : Modules  :" + modulesOnFS);
            return modulesOnFS;
        } catch (err) {
            console.error("getDownloadedModules : Error :" + err);
        }
    }

    function getCoreModules(modules) {
        try {
            var coreModules = getProperty("clientCoreModulesUpgrade");
            for (var i in coreModules) {
                modules.push(coreModules[i]);
            }
            return modules;
        } catch (err) {
            console.error("getCoreModules : Error :" + err);
        }
    }

    function getModuleVersion(module) {
        try {
            // try the public
            var publicModuleConfigPath = PROTEUS_PATH + module + '/package.json';
            if (path.existsSync(publicModuleConfigPath)) {
                var bufferJSON = JSON.parse(fs.readFileSync(publicModuleConfigPath));
                console.info("getModuleVersion : public module :" + module + " version :" + bufferJSON.version);
                return bufferJSON.version;
            } else {
                // Might be a core moudle
                return "0.0.1";
            }
        } catch (err) {
            console.error("getModuleVersion : Error :" + err);
        }
    }

    function getPackage(pkg, callback) {
        console.info(" In getPackage :" + pkg);
        var pkgsDownloadedList = []; // list of pkgs that were downloaded
        var pkgsList = []; // list of pkgs to process

        function isPkgAvailable(pkgName) {
            var pkgConfigPath = PROTEUS_PATH + pkgName + '/package.json';
            if (path.existsSync(pkgConfigPath)) {
                console.info(pkgName + " is Available Locally");
                return true;
            } else {
                console.info(pkgName + " is Not Available Locally");
                return false;
            }
        }
        // get all the Dependencies for the given package

        function getDependencies(pkgName) {
            var pkgConfigPath = PROTEUS_PATH + pkgName + '/package.json';
            var jsonBuffer = JSON.parse(fs.readFileSync(pkgConfigPath));
            var pkgDependencies = [];
            if (jsonBuffer.dependencies) {
                console.info(pkgName + " dependencies :" + JSON.stringify(jsonBuffer.dependencies));
                for (var key in jsonBuffer.dependencies) {
                    pkgDependencies.push(key);
                }
            }
            return pkgDependencies;
        }

        function downloadHandler(callback) {
            console.info("downloadHandler : " + pkgsList);
            if (pkgsList.length > 0) {
                // download the pkg
                process.acquireLock(function() {
                    // check once more if the item was downloaded due to another instance while this was waiting
                    if (isPkgAvailable(pkgsList[0])) {
                        console.info(pkgsList[0] + " is Available due to another instance");
                        // add more
                        var newDependencies = getDependencies(pkgsList[0]);
                        process.releaseLock();
                        callback(true);
                    } else {
                        download(pkgsList[0], function(path, statusCode) {
                            console.info(pkgsList[0] + " Download Success : Path :" + path + " statuscode :" + statusCode);
                            // add more
                            var newDependencies = getDependencies(pkgsList[0]);
                            // pop the first element add it to the list of pkgs downloaded
                            pkgsDownloadedList.push(pkgsList.shift());
                            pkgsList = pkgsList.concat(newDependencies);
                            callback(true);
                            process.releaseLock();
                        }, function(error) {
                            console.error(pkgsList[0] + " Download Failed : " + error);
                            process.releaseLock();
                            callback(false, error);
                        });
                    }
                });
            }
        }

        function checkPkgs(result, error) {
            if (result) {
                if (pkgsList.length > 0) {
                    if (isPkgAvailable(pkgsList[0])) {
                        var dependencies = getDependencies(pkgsList[0]);
                        // skip the current pkg
                        pkgsList.shift();
                        if (dependencies.length > 0) {
                            pkgsList = pkgsList.concat(dependencies);
                        }
                        checkPkgs(true);
                    } else {
                        console.info(" In checkPkgs : start download " + pkgsList[0]);
                        downloadHandler(checkPkgs);
                    }
                } else {
                    console.info("get Package :" + pkg + " Success");
                    callback(true);
                }
            } else {
                console.info("get Package :" + pkg + " Failed");
                console.info("detele downloaded pkgs : " + pkgsDownloadedList);
                // do cleanup of downloaded pkgs
                for (var n = 0; n < pkgsDownloadedList.length; ++n) {
                    try {
                        rmdirRSync(PROTEUS_PATH + pkgsDownloadedList[n]);
                    } catch (ex) {
                        console.error("checkPkgs:: rmdirRSync :" + PROTEUS_PATH + pkgsDownloadedList[n] + " Failed");
                    }
                }
                callback(false, error);
            }
        }
        pkgsList.push(pkg);
        checkPkgs(true);
    }

    function download(moduleName, successCB, failureCB) {
        if (typeof failureCB !== 'function') {
            failureCB = function(err) {
                console.error("error in download: " + err);
            };
        }
        if (typeof moduleName !== 'string' || typeof successCB != 'function') {
            console.error("moduleName: " + moduleName + " successCB: " + successCB);
            return failureCB(createError(consts.TYPE_MISMATCH_ERR, consts.TYPE_MISMATCH_ERR_MSG));
        }
        if (path.existsSync(TEMP_PATH) === false) {
            fs.mkdirSync(TEMP_PATH, PERM);
        }

        function installPackage(filePath, successCB, failureCB) {
            console.info("ProteusModLoader::installPackage ::  crx file  " + filePath);
            var installPath;
            installPath = PROTEUS_PATH + moduleName;
            packageExtractor.packageExtractor.extract(filePath, moduleName, function() {
                successCB(installPath);
            }, function(error) {
                failureCB(error);
            });
        }
        //downloads a module from given server

        function downloadModule(requestUrl, numRedirect, successCB, failureCB) {
            var downloadPath = TEMP_PATH + moduleName + ".crx";
            var requestCancelFn = null, clearTimeoutfn = null, fileStreamCancelFn = null;
            var reqTimeoutObj = null;
            var abort = false;
            var connected = false;
            var downloadModFile = null;
            try {
                var contentLength = 0;
                var bytesDownloaded = 0;
                if (path.existsSync(downloadPath)) {
                    fs.unlink(downloadPath);
                }
                //console.info("Download Module :: requestUrl : ", requestUrl);
                console.info("ProteusModLoader::downloadModule Module: " + moduleName + " to Directory : " + downloadPath);
                var parsedURL = url.parse(requestUrl);
                var options = {
                    host: parsedURL.host,
                    port: getProperty("serverPort"),
                    path: parsedURL.pathname + parsedURL.search,
                    method: 'GET'
                };
                // function to cleanup if request is being cancelled
                requestCancelFn = function() {
                    if (request) {
                        console.info("Abort module  download:" + moduleName);
                        request.abort();
                        clearTimeoutfn();
                        if (fileStreamCancelFn)
                            fileStreamCancelFn();
                        request = null;
                        abort = true;
                    }
                };
                console.info("<<Time : Start connection Download" + moduleName + ">>" + Date.now());
                request = https.request(options, function(response) {
                console.info("<<Time : Connected To server for : " + moduleName + ">>" + Date.now());
                connected = true;
                    try {
                        switch (response.statusCode) {
                        case 200:
                            contentLength = response.headers['content-length'];
                            break;
                        case 302:
                            var redirectedRemote = response.headers.location;
                            downloadModule(redirectedRemote, numRedirect + 1, successCB, failureCB);
                            return;
                        case 404:
                            console.error("Module " + moduleName + " Not Found");
                            // unregister function for node destruction.
                            clearTimeoutfn();
                            process.removeListener('exit', requestCancelFn);
                            failureCB(createError(consts.NOT_FOUND_ERR, consts.MODULE_NOT_FOUND_ERR_MSG));
                            //request.abort();
                            break;
                        default:
                            //request.abort();
                            // unregister function for node destruction.
                            clearTimeoutfn();
                            process.removeListener('exit', requestCancelFn);
                            failureCB(createError(consts.NETWORK_ERR, consts.NETWORK_ERR_MSG));
                            return;
                        }
                        // function to cleanup stream on exit
                        fileStreamCancelFn = function() {
                                if ((connected === true) && downloadModFile) {
                                    console.info("Abort file stream write:");
                                    downloadModFile.destroy();
                                    downloadModFile = null;
                                }
                            };
                        response.on('data', function(data) {
                            if ((data.length > 0) && (response.statusCode == 200)) {
                                // Open only if the file is not opened already
                                if (downloadModFile === null) {
                                    downloadModFile = fs.createWriteStream(downloadPath, {
                                        'flags': 'a'
                                    });
                                }
                                downloadModFile.write(data);
                                bytesDownloaded += data.length;
                                var percent = parseInt((bytesDownloaded / contentLength) * 100, 10);
                                console.info("Module " + moduleName + " Progress: " + percent);
                                downloadModFile.addListener('close', function() {
                                    if (downloadModFile && (abort === false)) {
                                        if (downloadModFile.bytesWritten != contentLength) {
                                            console.error(" Incomplete download bytesDownloaded: " + downloadModFile.bytesWritten + "contentLength" + contentLength);
                                            failureCB(createError(consts.NETWORK_ERR, consts.NETWORK_ERR_MSG));
                                        } else {
                                            installPackage(downloadPath, function(installPath) {
                                                console.info("Installed module :" + moduleName);
                                                successCB(installPath, response.statusCode);
                                            }, function(ex) {
                                                console.error("installPackage Got error: " + ex);
                                                failureCB(ex);
                                            });
                                        }
                                        process.removeListener('exit', requestCancelFn);
                                    }
                                    downloadModFile = null;
                                });
                            }
                        });
                        response.on('end', function() {
                            if (response.statusCode == 200) {
                                // unregister function for node destruction.
                                clearTimeoutfn();
                                console.info("<<Time : Download Complete: " + moduleName + ">>" + Date.now());
                                downloadModFile.end();
                            }
                        });
                        response.on('close', function(err) {
                            // unregister function for node destruction.
                            clearTimeoutfn();
                            // delete file if its already present
                            if (path.existsSync(downloadPath)) {
                                fs.unlinkSync(downloadPath);
                            }
                            if (response.statusCode == 200) {
                                downloadModFile.end();
                            }
                        });
                    } catch (ex) {
                        console.error("Response Got error: " + ex.message);
                        // delete file if its already present
                        if (path.existsSync(downloadPath)) {
                            try {
                                fs.unlinkSync(downloadPath);
                            } catch (ex) {
                                console.error("unlinkSync Got error: " + ex.message);
                            }
                            // unregister function for node destruction.
                            clearTimeoutfn();
                            process.removeListener('exit', requestCancelFn);
                            failureCB(createError(consts.NETWORK_ERR, consts.NETWORK_ERR_MSG));
                        }
                    }
                });
                request.end();

                clearTimeoutfn = function() {
                    if (reqTimeoutObj) {
                        clearTimeout(reqTimeoutObj);
                        reqTimeoutObj = null;
                    }
                };
                // cleanup the reques tif the request is taking too long
                reqTimeoutObj = setTimeout(function() {
                      console.error("Server Taking too long request cancelled isConnected :" + connected);
                      console.info("<<Time : Request Timeout : " + moduleName + ">>" + Date.now());
                      requestCancelFn();
                      process.removeListener('exit', requestCancelFn);
                      // might not be yet defined if response is not recieved
                      reqTimeoutObj = null;
                      failureCB(createError(consts.NETWORK_ERR, consts.NETWORK_ERR_MSG));
                }, getProperty("clientConnTimeout"));

                request.on('error', function(e) {
                    // delete file if its already present
                    if (path.existsSync(downloadPath)) {
                        fs.unlinkSync(downloadPath);
                    }
                    // unregister function for node destruction.
                    clearTimeoutfn();
                    process.removeListener('exit', requestCancelFn);
                    console.error("Request Got error: " + e.message);
                    failureCB(createError(consts.NETWORK_ERR, consts.NETWORK_ERR_MSG));
                });
                process.on('exit', requestCancelFn);
            } catch (ex) {
                console.error("Download Got error: " + ex.message);
                // delete file if its already present
                if (path.existsSync(downloadPath)) {
                    try {
                        fs.unlinkSync(downloadPath);
                    } catch (ex) {
                        console.error("unlinkSync Got error: " + ex.message);
                    }
                    // unregister function for node destruction.
                    clearTimeoutfn();
                    process.removeListener('exit', requestCancelFn);
                    failureCB(createError(consts.UNKNOWN_ERR, consts.UNKNOWN_ERR_MSG));
                }
            }
        }

        function downloadNow() {
            var encodedQueryStr = encodeURIComponent(getDeviceInfo() + 'Module=' + moduleName);
            var requestUrl = getProperty("serverURL") + '/getModule?' + encodedQueryStr; // www.qualcomm-xyz.com/getModule?AV=4.0&PV=1.0.0&Module=xyz
            downloadModule(requestUrl, 0, successCB, failureCB);
        }
        downloadNow();
    }

    function getVersions(requestUrl, successCB, failureCB) {
        console.info("Get Versions");
        var abortVersionCheckFn = null, clearTimeoutfn = null;
        var reqTimeoutObj = null;
        var abort = false;
        try {
            var contentLength = 0;
            var bytesDownloaded = 0;
            var parsedURL = url.parse(requestUrl);
            var options = {
                host: parsedURL.host,
                port: getProperty("serverPort"),
                path: parsedURL.pathname + parsedURL.search,
                method: 'GET'
            };
            console.info("Get version :: requestUrl : split " + sys.inspect(options));
            abortVersionCheckFn = function() {
                if (request && process.getModuleUpdates() < 2) {
                    console.info("Abort querying Version from server ");
                    clearTimeoutfn();
                    request.abort();
                    request = null;
                    abort = true;
                }
            };
            request = https.request(options, function(response) {
                try {
                    var versionResponse = '';
                    switch (response.statusCode) {
                    case 200:
                        contentLength = response.headers['content-length'];
                        break;
                    case 302:
                        var redirectedRemote = response.headers.location;
                        getVersions(redirectedRemote, successCB, failureCB);
                        return;
                    case 404:
                        console.error("unable to get latest versions");
                        // unregister function for node destruction.
                        clearTimeoutfn();
                        process.removeListener('exit', abortVersionCheckFn);
                        request.abort();
                        failureCB("unable to get latest version", response.statusCode);
                        break;
                    default:
                        // unregister function for node destruction.
                        clearTimeoutfn();
                        process.removeListener('exit', abortVersionCheckFn);
                        request.abort();
                        failureCB("Error ", response.statusCode);
                        return;
                    }
                    response.on('data', function(data) {
                        console.info("got response" + data);
                        if ((data.length > 0) && (response.statusCode == 200)) {
                            versionResponse += data;
                        }
                    });
                    response.on('end', function() {
                        console.info("getVersion in response on end ");
                        if (response.statusCode == 200) {
                            // unregister function for node destruction.
                            clearTimeoutfn();
                            process.removeListener('exit', abortVersionCheckFn);
                            if (abort === false) {
                                successCB(versionResponse, response.statusCode);
                            }
                        }
                    });
                    response.on('close', function(err) {
                        console.error("Response close Got error: " + err.message);
                        // unregister function for node destruction.
                        process.removeListener('exit', abortVersionCheckFn);
                        if (abort === false) {
                            failureCB(err, response.statusCode);
                        }
                    });
                } catch (ex) {
                    console.error("Got error: " + ex.message);
                    // unregister function for node destruction.
                    process.removeListener('exit', abortVersionCheckFn);
                    failureCB(ex, 0);
                }
            });
            request.end();
            clearTimeoutfn = function() {
                if (reqTimeoutObj) {
                    clearTimeout(reqTimeoutObj);
                    reqTimeoutObj = null;
                }
            };
            reqTimeoutObj = setTimeout(function() {
                reqTimeoutObj = null;
                console.error("Server Taking too long");
                abortVersionCheckFn();
                process.removeListener('exit', abortVersionCheckFn);
                failureCB("unable to get latest version ", 0);
            }, getProperty("clientConnTimeout"));

            request.on('error', function(e) {
                console.error("GetVersion Request Got error: " + e.message);
                // unregister function for node destruction.
                clearTimeoutfn();
                process.removeListener('exit', abortVersionCheckFn);
                failureCB(e.message, 0);
            });
            // if the request is being cancelled cleanup
            process.on('exit', abortVersionCheckFn);
        } catch (ex) {
            console.error("Got error: " + ex.message);
            // unregister function for node destruction.
            clearTimeoutfn();
            process.removeListener('exit', abortVersionCheckFn);
            failureCB(ex, 0);
        }
    }


    function getLatestVersions(modules, callback) {
        var encodedQueryStr = encodeURIComponent(getDeviceInfo() + 'Modules=' + modules.join(','));
        var requestUrl = getProperty("serverURL") + '/getVersions?' + encodedQueryStr;
        //console.info("GetVersion : requestUrl : " + requestUrl);
        try {
            getVersions(requestUrl, function(modules, statusCode) {
                console.info("GetVersion Success : modules" + modules + " statuscode :" + statusCode);
                callback(true, modules);
            }, function(result, statusCode) {
                console.error("GetVersion Failed : " + result + statusCode);
                callback(false);
            });
        } catch (ex) {
            console.error("GetVersion Failed : " + ex);
            callback(false);
        }
    }

    function readUpdateStatus() {
        var filePath = UPDATE_FILE;
        try {
            if (path.existsSync(filePath)) {
                var time = fs.readFileSync(filePath, 'utf8');
                if (!time) {
                    console.error("Error Reading Update Time");
                    return 0;
                } else {
                    console.info("Module Update file was read : Last Update Time ! :" + time);
                    return time;
                }
            } else {
                console.error("Module Update file not Found ");
            }
        } catch (ex) {
            console.error("Error Reading Update Time " + ex);
            if (path.existsSync(filePath)) {
                // delete the log file file.
                fs.unlinkSync(filePath);
            }
            return 0;
        }
    }

    function writeUpdateStatus(time) {
        var filePath = UPDATE_FILE;
        try {
            if (path.existsSync(filePath)) {
                // delete the log file file.
                fs.unlinkSync(filePath);
            }
            var err = fs.writeFileSync(filePath, time.toString());
            if (err) {
                console.error("Error Writing Update Time " + err);
            } else {
                console.info("New Time update to update log  : " + time);
            }
        } catch (ex) {
            console.error("Error Writing Update Time " + ex);
            if (path.existsSync(filePath)) {
                // delete the log file file.
                fs.unlinkSync(filePath);
            }
        }
    }

    function compareVersions(currentVersionStr, serverVersionStr) {
        function versionStrToObj(versionStr) {
            var splitArray = versionStr.split('.');
            var major = parseInt(splitArray[0], 10) || 0;
            var minor = parseInt(splitArray[1], 10) || 0;
            var patch = parseInt(splitArray[2], 10) || 0;
            var versionObj = {
                major: major,
                minor: minor,
                patch: patch
            };
            return versionObj;
        }
        if (typeof currentVersionStr == 'string' && typeof serverVersionStr == 'string') {
            var currentVersion = versionStrToObj(currentVersionStr);
            var serverVersion = versionStrToObj(serverVersionStr);
            if (currentVersion.major < serverVersion.major) {
                return true; // newer version
            } else if (currentVersion.minor < serverVersion.minor || currentVersion.patch < serverVersion.patch) {
                return true; // newer version
            } else {
                return false; // same version
            }
        } else {
            throw ("Invalid Versions to compare");
        }
    }

    function checkUpdates() {
        try {
            var now = Date.now();
            var lastCheck = readUpdateStatus();
            console.info("In check updates");
            if (!lastCheck || (parseInt(now.toString(), 10) > (parseInt(lastCheck, 10) + getProperty("clientUpdatePeriod")))) {
                console.info("Updates check required now: " + now + " lastCheck : " + lastCheck + " updatePeriod :" + getProperty("clientUpdatePeriod"));
                var mods = getDownloadedModules();
                // Get Core modules
                // getCoreModules(mods);
                if (mods.length > 0) {
                    try {
                        console.info("Reques server for updates");
                        getLatestVersions(mods, function(result, versions) {
                            if (result) {
                                console.info("Module List :" + mods);
                                console.info("Server Vesion List :" + sys.inspect(versions));
                                var jsonObj = JSON.parse(versions);
                                var moduleVersions = jsonObj.versionList;
                                for (var n = 0; n < mods.length; ++n) {
                                    if (moduleVersions[n] !== null) {
                                        var localVersion = getModuleVersion(mods[n]);
                                        console.info("Module : " + mods[n] + " Local Version : " + localVersion + " Server Version : " + moduleVersions[n]);
                                        // compare server and local version
                                        if (compareVersions(localVersion, moduleVersions[n])) {
                                            // remove the module
                                            try {
                                                rmdirRSync(PROTEUS_PATH + mods[n]);
                                            } catch (ex) {
                                                console.error("checkPkgs:: rmdirRSync :" + PROTEUS_PATH + mods[n] + " Failed");
                                            }
                                        }
                                    }
                                }
                                // update the timestamp for versioncheck
                                writeUpdateStatus(now);
                            }
                            // do we block
                            // set that the modules have been updated.
                            process.setModuleUpdates(2);
                            // clear the lock so that loadmodule can continue
                            process.releaseLock();
                        });
                    } catch (ex) {
                        console.error("In checkUpdates : Error " + ex);
                        // set that the modules have been updated.
                        process.setModuleUpdates(2);
                        // clear the lock so that loadmodule can continue
                        process.releaseLock();
                        return;
                    }
                } // if modules present
                else {
                    console.info("In checkUpdates : No modules on device");
                    // set that the modules have been updated.
                    process.setModuleUpdates(2);
                    process.releaseLock();
                }
            } // if updates checked
            else {
                console.info("Updates check not required : " + now + " lastCheck : " + lastCheck + " updatePeriod :" + getProperty("clientUpdatePeriod"));
                // set that the modules have been updated.
                process.setModuleUpdates(2);
                process.releaseLock();
            }
        } //try
        catch (ex) {
            console.error("In checkUpdates : Error " + ex);
            // set that the modules have been updated.
            process.setModuleUpdates(2);
            process.releaseLock();
        }
    }

    function downloadUpdates() {
        process.setModuleUpdates(1); //set the flag to inprogress state
        process.acquireLock(checkUpdates);
    }

    // creates and returns an Error object


    function createError(err, msg) {
        var e = new Error(msg);
        e.name = err;
        return e;
    }

    proteusModManager.prototype.loadPackage = function(pkgName, successCB, failureCB) {
        var timerObj, clearTimeoutfn;
        if (typeof failureCB !== 'function') {
            failureCB = function(err) {
                console.error("error in loadPackage: " + err);
            };
        }
        if (typeof pkgName !== 'string' || typeof successCB != 'function') {
            console.error("pkgName: " + pkgName + " successCB: " + successCB);
            return failureCB(createError(consts.TYPE_MISMATCH_ERR, consts.TYPE_MISMATCH_ERR_MSG));
        }
        console.info("loadPackage  : " + pkgName);
        // Check if downloads directory exists
        if (path.existsSync(PROTEUS_PATH) === false) {
            fs.mkdirsRSync(PROTEUS_PATH, PERM);
        }
        // Download updates only the first call to load Pkg.
        console.info("process.getModuleUpdates() " + process.getModuleUpdates());
        if (process.getModuleUpdates() === 0) {
            // Check and download updates (getVersions)
            downloadUpdates();
            console.info("loadPackage  : " + pkgName + " delayed due to getVersion - Request for Update");
            clearTimeoutfn = function() {
                if (timerObj) {
                    clearTimeout(timerObj);
                    timerObj = null;
                }
            };
            // block the load pkg until getVersion returns
            timerObj = setTimeout(function() {
                timerObj = null;
                // remove the listener which listens to node destruction
                process.removeListener('exit', clearTimeoutfn);
                proteusModManager.prototype.loadPackage(pkgName, successCB, failureCB);
            }, getProperty("clientRetryTime"));
            // if the request if being cancelled cleanup
            process.on('exit', clearTimeoutfn);
        } // else check if we have completed checking the version
        else if (process.getModuleUpdates() == 2) {
            // check is pkg and its dependencies are available else download it
            getPackage(pkgName, function(result, err) {
                if (result) {
                    console.info("loadPackage  :" + pkgName + " Return Success to client");
                    successCB();
                } else {
                    console.info("loadPackage  :" + pkgName + " Return Failure to client");
                    failureCB(err);
                }
            });
        } else {
            // block the load pkg until getVersion returns
            // function to clear timer if node is being destroyed
            clearTimeoutfn = function() {
                if (timerObj) {
                    clearTimeout(timerObj);
                    timerObj = null;
                }
            };
            console.info("loadPackage  : " + pkgName + "delayed due to getVersion");
            timerObj = setTimeout(function() {
                timerObj = null;
                // remove the listener which listens to node destruction
                process.removeListener('exit', clearTimeoutfn);
                proteusModManager.prototype.loadPackage(pkgName, successCB, failureCB);
            }, getProperty("clientRetryTime"));
            // if the request if being cancelled cleanup
            process.on('exit', clearTimeoutfn);
        }
    };
}
