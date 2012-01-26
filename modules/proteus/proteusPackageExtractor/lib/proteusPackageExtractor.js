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
var fs = require('fs');
var path = require('path');
var crypto = require('crypto');
var proteusUnzip = require("proteusUnzip");

var DEFAULT_PUBLIC_KEY_LENGTH = 256;

//Package Class which contains package assets; to be returned via callback

function PackageContents() {
    var magicNumber;
    var version;
    var publicKeyLength;
    var signatureLength;
    var publicKey;
    var signature;
    var zip;
};

/*
 * Extracts the contents of the crx package, gets verification of its contents using its signature & makes a call
 * to have the contents unzipped & installed on the device. On success, the success callback is fired w/o any params.
 *
 * @param {String} filePath - A PackContents object which contains the properties of the crx package
 * @param {String} moduleName - A PackContents object which contains the properties of the crx package
 * @param {Function} successCB - function to call w/ array of files
 * @param {Function} errorCB - function to call when there is an error attaining directory files
 */

function PackageExtractor() {}

PackageExtractor.prototype.extract = function (filePath, moduleName, successCB, failureCB) {

    var installationPath = process.downloadPath + '/' + moduleName;

    console.log("filePath =" + filePath);
    console.log("installationPath =" + installationPath);
    console.log("moduleName =" + moduleName);

    if (typeof failureCB !== 'function') {
        failureCB = function (e) {
            console.error("Error: " + e);
            throw e;
        };
    }

    if (typeof filePath !== 'string' || typeof successCB != 'function') {
        console.error("filePath: " + filePath + " successCb: " + successCB);
        return failureCB("Invalid arguments");
    }


    if (!path.existsSync(filePath)) {
      return failureCB("File Path Provided Not Valid");
    }


    fs.open(filePath, 'r', function (err, fd) {
        if (err) {
            throw (err.message);
        }

        var stats = fs.fstatSync(fd);
        var fileSize = stats.size;
        var contents = new Buffer(fileSize);

        console.log("fileSize =" + fileSize);

        fs.read(fd, contents, 0, fileSize, 0, function (err, bytesRead) {
            if (err) {
                if (path.existsSync(filePath)) {
                    fs.unlink(filePath);
                }

                return failureCB("Error: Error Reading File - " + err);
            }

            if (bytesRead != fileSize) {
                if (path.existsSync(filePath)) {
                    fs.unlink(filePath);
                }

                return failureCB("Error: Bytes Read Does Not Match File Size");
            }

            var packageContents = new PackageContents();
            var byteStart = 0;
            var byteEnd = 0;
            var iteration = 0;
            var numBytes = 4;

            //Get first 4 bytes - magic number
            byteEnd = numBytes;
            var bufMagicNumber = contents.slice(byteStart, byteEnd);
            packageContents.magicNumber = bufMagicNumber.toString('binary', 0, bufMagicNumber.length);
            iteration++;

            //Get second set of 4 bytes - version
            byteStart = iteration * numBytes;
            byteEnd = byteStart + 4;
            var bufVersion = contents.slice(byteStart, byteEnd);
            packageContents.version = (bufVersion[0]);
            iteration++;

            //Get third set of 4 bytes - public key length
            byteStart = iteration * numBytes;
            byteEnd = byteStart + 4;
            var bufPubKeyLen = contents.slice(byteStart, byteEnd);
            var publicKeyLength = (bufPubKeyLen[0]);
            publicKeyLength += DEFAULT_PUBLIC_KEY_LENGTH;
            packageContents.publicKeyLength = publicKeyLength;
            iteration++;

            //Get fourth set of 4 bytes - Signature length
            byteStart = iteration * numBytes;
            byteEnd = byteStart + 4;
            var bufSigLen = contents.slice(byteStart, byteEnd);
            var signatureLength = (bufSigLen[0]);
            packageContents.signatureLength = signatureLength;
            iteration++;

            //Get public key
            byteStart = iteration * numBytes;
            byteEnd = byteStart + publicKeyLength;
            var bufPubKey = contents.slice(byteStart, byteEnd);
            packageContents.publicKey = bufPubKey.toString('ascii');

            //Get signature
            byteStart = byteEnd;
            byteEnd = byteStart + signatureLength;
            var bufSig = contents.slice(byteStart, byteEnd);
            packageContents.signature = bufSig.toString('binary'); //.toString('base64', 0, bufSig.length);
            //Get zip contents
            byteStart = byteEnd;
            byteEnd = fileSize; //read to the end of the file
            var bufZip = contents.slice(byteStart, byteEnd);
            packageContents.zip = bufZip; //.toString('base64', 0, bufZip.length);
            //verify signature
            if (verifySig(packageContents)) {
                try {
                    unzipAndInstall(packageContents, installationPath, moduleName);
                } catch (ex) {
                    if (path.existsSync(filePath)) {
                        fs.unlink(filePath);
                    }

                    return failureCB("Exception Has Occurred " + ex.message);
                }
            } else {
                if (path.existsSync(filePath)) {
                    fs.unlink(filePath);
                }

                return failureCB("Exception Has Occurred - Invalid Signature");
            }

            successCB();
        });
    });
};


function unzipAndInstall(packageObj, installPath, moduleName) {
    var TEMP_PATH = process.downloadPath + '/temp/';
    var tempPath = TEMP_PATH + moduleName;

    if (!path.existsSync(tempPath)) {
        fs.mkdirSync(tempPath, 448);
    }

    //var buffer = new Buffer(package.zip , 'base64');
    var result = proteusUnzip.decompressZipBuffer(packageObj.zip, tempPath);

    if (result) {
        console.info("unzipAndInstall - delete and move the package");

        if (!(validatePackJson(tempPath))) {
            console.error("PackageExtractor::unzipAndInstall- Error:Package.json does not contain required fields");
            throw ("Package.json does not contain required fields");
        }

        if (path.existsSync(installPath)) { // delete existing module in correct path and rename
            try {
                fs.rmdirRSync(installPath);
            } catch (ex) {
                console.error("PackageExtractor::unzipAndInstall- Error:Failed to rmdirRSync install Path" + ex);
            }
        }

        fs.renameSync(tempPath, installPath);
        try {
            fs.rmdirRSync(TEMP_PATH); // delete the downloaded file & temp folder
        } catch (ex) {
            console.error("PackageExtractor::unzipAndInstall- Error:Failed to rmdirRSync temp Path" + ex);
        }
    } else {
        try {
            fs.rmdirRSync(TEMP_PATH); // delete the downloaded file & temp folder
            fs.rmdirRSync(installPath); // delete files/folder install path folder
        } catch (ex) {
            console.error("PackageExtractor::unzipAndInstall- Error:Failed to rmdirRSync temp Path" + ex);
            throw ("Unzip process failed; zip contents are invalid");
        }
        throw ("Unzip process failed; zip contents are invalid");
    }
}


function validatePackJson(temporaryPath) {
    var JsonPath = temporaryPath + "/package.json";
    var valid = true;
    var jsonBuffer = JSON.parse(fs.readFileSync(JsonPath));
    if ((!jsonBuffer.name) || (!jsonBuffer.version) || (!jsonBuffer.main)) {
      valid = false;
    }
    return valid;
}

/*
 * Verifies the package contents - First ensures the package public key matches that what is on the client.
 * Then verifies contents against the signature & public key of the crx package. If veriried correctly, returns bool TRUE.
 *
 * @param {Object} packageContents - A PackContents object which contains the properties of the crx package
 */

function verifySig(packageContents) {
    var configPath = process.downloadPath + '/' + "proteusConfig" + '/package.json';
    var proteusConfig = require("proteusConfig");
    var publicKey = "-----BEGIN PUBLIC KEY-----MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDGCjNQJYL2XuI8VrXEswxzw5N4DgDLyaNncSGUMkG5vz/XSEYsXih2+tjn89gDvoddXqeuVrkqm/ufGSnj6DhaobiDsXatO4Gk2md+IOwJOl/CjdBs6MzmKY05MwLk1aguDltsF1tfuzdD5czwTBVd2beYm97fKfA1SJBXX59g+QIDAQAB-----END PUBLIC KEY-----";
    var proteusConfigObj = new proteusConfig();
    var configObj = proteusConfigObj.getConfig();
    if (configObj && typeof configObj.proteusPackageExtractor === 'object') {
      var packageExtractorConfig = configObj.proteusPackageExtractor;
      var configFileValue = packageExtractorConfig["Public-Key"];
      if (configFileValue) {
          publicKey = configFileValue;
      }
      console.info("Config file : [Public-Key] => [" + publicKey + "]");
    }
    //verify public key from crx matches that on client (hex compare)var bufPackagePubKey = new Buffer(packageContents.publicKey);
    var bufPackagePubKey = new Buffer(packageContents.publicKey);
    var packagePubKeyHex = bufPackagePubKey.toString('hex');
    packagePubKeyHex = packagePubKeyHex.replace(/0a/g, ''); //remove the line feeds from the package's pub key
    var bufClientPubKey = new Buffer(publicKey);
    var clientPubKeyHex = bufClientPubKey.toString('hex');
    if (packagePubKeyHex != clientPubKeyHex) {
        console.error("verifySig Error - Public Keys Do Not Match");
        return false;
    }
    //verify signature
    var verifier = crypto.createVerify("sha256");
    verifier.update(packageContents.zip);
    var verification = verifier.verify(packageContents.publicKey, packageContents.signature, "binary");
    return verification;
}

exports.packageExtractor = new PackageExtractor();
