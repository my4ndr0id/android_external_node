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

var fs = require('fs'),
    path = require('path'),
    crypto = require('crypto'),
    proteusUnzip = require('proteusUnzip');

var throwError = function (err, msg) {
    var e = new Error(msg||'');
    e.name = err;
    throw e;
};

var readUInt32LE = function (buf, index) {
    if (index+3 >= buf.length) {
        throwError('INVALID_VALUES_ERR');
    }
    var b1 = buf[index].charCodeAt(0),
        b2 = buf[index+1].charCodeAt(0),
        b3 = buf[index+2].charCodeAt(0),
        b4 = buf[index+3].charCodeAt(0);

    return b1 + (b2 << 8) + (b3 << 16) + (b4 << 24);
};

function validatePackJson(temporaryPath) {
    var JsonPath = temporaryPath + "/package.json";
    var valid = true;
    var jsonBuffer = JSON.parse(fs.readFileSync(JsonPath));
    if ((!jsonBuffer.name) || (!jsonBuffer.version) || (!jsonBuffer.main)) {
        valid = false;
    }
    return valid;
}

// creates and returns an Error object
function createError(err, msg) {
    var e = new Error(msg);
    e.name = err;
    return e;
}

function unzipAndInstall(zipObj, installPath, moduleName, successCB, failureCB) {
    var TEMP_PATH = process.downloadPath + '/temp/';
    var tempPath = TEMP_PATH + moduleName;
    var err = createError('IO_ERR', "In Unzip Failed");
    try {
        if (!path.existsSync(tempPath)) {
            fs.mkdirSync(tempPath, 448);
        }
        var result = proteusUnzip.decompressZipBuffer(new Buffer(zipObj, 'binary'), tempPath);
        if (result) {
            if (!(validatePackJson(tempPath))) {
                throwError('NOT_FOUND_ERR', "Invalid package.json");
            }
            // delete existing module in correct path and rename
            if (path.existsSync(installPath)) {
                fs.rmdirRSync(installPath);
            }
            fs.renameSync(tempPath, installPath); // rename the temp folder to be the module folder
            fs.rmdirRSync(TEMP_PATH); // delete the downloaded file & temp folder
            return successCB();
        }
    } catch (err) { }

    try {
        fs.rmdirRSync(TEMP_PATH); // delete the downloaded file & temp folder
        fs.rmdirRSync(installPath); // delete files/folder install path folder
    } catch (err) {}
    failureCB(err);
}

var extract = function (filePath, moduleName , successCB, failureCB) {
    var installPkg = function(pkg) {
        var installationPath = process.downloadPath + '/' + moduleName;
        unzipAndInstall(pkg.zip, installationPath, moduleName, successCB, failureCB);
    };
    parseCRX(filePath,function(pkg) {verifySig(pkg, installPkg ,failureCB);}, failureCB);
};

var parseCRX = function (filePath, successCB, failureCB) {
    var buf, pkg = {};
    try {
        buf = fs.readFileSync(filePath, 'binary');
        pkg.magicNumber     = buf.slice(0, 4).toString();
        if (pkg.magicNumber !== "Cr24") {
            throwError('SECURITY_ERR', "Magic Number not matched");
        }

        pkg.version = readUInt32LE(buf, 4);
        if (pkg.version !== 2) {
            throwError('SECURITY_ERR', "Bad CRX version: " + pkg.version);
        }

        pkg.publicKeyLength = readUInt32LE(buf, 8);
        pkg.signatureLength = readUInt32LE(buf, 12);

        if (pkg.publicKeyLength+pkg.signatureLength+8 >= buf.length) {
            throwError('SECURITY_ERR', "CRX file size is not correct");
        }
        var stats = fs.lstatSync(filePath);

        pkg.publicKey       = buf.slice(16, 16+pkg.publicKeyLength);
        pkg.signature       = buf.slice(16+pkg.publicKeyLength, 16+pkg.publicKeyLength+pkg.signatureLength).toString('binary');
        pkg.zip             = buf.slice(16+pkg.publicKeyLength+pkg.signatureLength, stats.size);


    } catch (err) {
        return failureCB(err);
    }

    successCB(pkg);
};

var verifySig = function (pkg, successCB, failureCB) {
    var proteusConfig = require("proteusConfig");
    var publicKey = "-----BEGIN PUBLIC KEY-----MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4QS7FPPTmh4k2rHr1opaOMOiSraXrtq8/ZAELxEioh35HUoYYZeL3EHW5lz4N4mogjZSiCu0IyNLP9Y5yuHm7Rw9C1WQhm+RAyrdXAojASR9UAblhnhOQjdE9+OTFhIcjYtdNqH18Cr3YARt1ZsD1WmmHyXXNZX1PX6uW/UlvPuS1/vAtfjp3iat2OlGPFTRGGNlY8+PbqRWKrs27Lhq8zS6xx7tCyTLlBGAHyN1HQku+XAuHJKmPTV0EBWVHJMW94tKNGqSieR8KLdPyLyWSfmfYl5BIF6ZN/bn/Oo2+KgoZeqOx55oe10K3kWG7MFFQ5x8pncKPSLRk/tCg5Of6wIDAQAB-----END PUBLIC KEY-----";
    var proteusConfigObj = new proteusConfig();
    var configObj = proteusConfigObj.getConfig();
    if (configObj && typeof configObj.packageExtractor === 'object') {
        var packageExtractorConfig = configObj.packageExtractor;
        var configFileValue = packageExtractorConfig["publicKey"];
        if (configFileValue) {
            publicKey = configFileValue;
        }
    }

    if (publicKey === pkg.publicKey.replace(/\r\n/, '')) {
        return failureCB(createError('SECURITY_ERR', 'Failed to match public keys'));
    }

    //verify signature
    var verifier = crypto.createVerify("sha256");
    verifier.update(pkg.zip);

    if (verifier.verify(pkg.publicKey, pkg.signature, "binary")) {
        successCB(pkg);
    } else {
        failureCB( createError('SECURITY_ERR', "Sig check failed"));
    }
};

exports.verifySig = verifySig;
exports.extract = extract;
exports.parseCRX =  parseCRX;