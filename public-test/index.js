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
var results = "";
var browserCallback;
var PASSED = 0, FAILED = 0
var count = 1;

exports.setTestCallback = function(cb) {
  browserCallback = cb;
}

function TestDriver(name) {
  this.name = name;
  this.status = "";
  this.error = undefined;
  this.startTime = (new Date).getTime();
}

TestDriver.prototype.reportResults = function() {
  with (this) {
    var time = (new Date).getTime() - startTime;
    console.error("Test " + status + ": " + name + "(" + time + ")");
    if (error) {
      console.error(error.stack);
    }

    if (status != 'PASSED') {
      test.exitCode(1);
    }

    if (browserCallback) {
      var color = 'white';
      status == 'PASSED' ? PASSED++ : (FAILED++, color = 'gray');
      results = '<tr bgcolor="' + color +'"> <td><pre>' + count++ + '</pre></td> <td> <pre>' + name + '</pre></td> <td><pre>' + status +
        '</pre></td><td><pre>' + time + '</pre></td><td><pre>' + (error ? error.stack : "") + '</pre>' + "\n" + results;

      // create the HTML to be sent to the client (to show up in iframe as an example)
      var html = "<html> <body> <H1> Core Test Results, Passed:" + PASSED + ", Failed: " + FAILED + "</H1>\n";
      html += '<table border="1">\n';
      html += results;
      html += '</table>\n';
      html += '</body></html>';

      require('webapp').callback(browserCallback, html);
    }

    // cleanup all existing listeners and add our listeners back..
    process.removeAllListeners();

    if (timer) {
      clearTimeout(timer);
      process.ref();
    }
  }
}

TestDriver.prototype.addListeners = function() {
  var that = this;

  process.on('idle', function() {
    with (that) {
      that.status = "PASSED";
      try {
        process.emit('exit');
      } catch (e) {
        that.status = "FAILED";
        that.error = e;
      }
      reportResults();
    }
  });

  process.on('uncaughtException', function(e) {
    with (that) {
      status = "FAILED";
      error = e;
      reportResults();
    }
  });
}

TestDriver.prototype.startTimer = function(timeout) {
  var that = this;
  this.timer = setTimeout(function() {
      with (that) {
      status = "TIMEOUT";
      reportResults();
      }
      }, timeout);
  process.unref();
}

TestDriver.prototype.run = function() {
  with (this) {
    console.error("Test STARTED: " + name);
    addListeners();
    startTimer(5000);
    try {
      require(name);
    } catch (e) {
      process.emit('uncaughtException', e);
    }
  }
}

exports.runTest = function(name) {
  var driver = new TestDriver(name);
  driver.run();
}

// start a watcher thread parallely, which adds/removes ev watchers at random time
// test.watcherThread();

exports.require = require;
exports.process = process;
exports.test = test;
exports.setTimeout = setTimeout;
