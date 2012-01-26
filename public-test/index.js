
function runTest(request) {
  try {
    console.log("runTest: " + request);
    test.start(request, 2.5);
    require(request);
    test.check();
  } catch (e) {
    console.log("runTest: " + request + " threw exception");
    console.error("\n" + e.stack);
    test.fail();
  }
  //test.watcherThread();
}

exports.require = require;
exports.process = process;
exports.test = test;
exports.runTest = runTest;
