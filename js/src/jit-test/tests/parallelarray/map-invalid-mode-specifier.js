// |jit-test| error: Error;

// This test tests the mode assertions.  It provides a function which
// should successfully execute and specifies that it should fail.  We
// expect an exception to result.

load(libdir + "parallelarray-helpers.js");

function testMap() {
  var p = new ParallelArray(parallelRange());
  var m = p.map(function (v) { return v+1; }, { mode: "par", expect: "fail" });
  assertEqParallelArray(m, new ParallelArray(range(1, ParallelSize+1)));
}

if (getBuildConfiguration().parallelJS && parallelTestsShouldPass())
  testMap();
else
  throw new Error();

