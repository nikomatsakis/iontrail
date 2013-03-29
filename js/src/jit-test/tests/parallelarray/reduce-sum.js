load(libdir + "parallelarray-helpers.js");

function testReduce() {
  function sum(v, p) { return v+p; }
  compareAgainstArray(parallelRange(), "reduce", sum);
}

if (getBuildConfiguration().parallelJS)
  testReduce();
