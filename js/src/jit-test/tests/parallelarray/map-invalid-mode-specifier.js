// |jit-test| error: Error;

// This test tests the mode assertions.  It provides a function which
// should successfully execute and specifies that it should fail.  We
// expect an exception to result.

load(libdir + "parallelarray-helpers.js");

function testMap() {
  var p = new ParallelArray(range(0, 64));
  assertParallelExecWillBail(function(m) {
    p.map(function(v) v+1, m);
  });
}

if (getBuildConfiguration().parallelJS)
  testMap();
else
  throw new Error();

