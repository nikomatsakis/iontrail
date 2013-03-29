load(libdir + "parallelarray-helpers.js");

function kernel(n) {
  // in parallel mode just recurse infinitely.
  if (n > 10 && inParallelSection())
    return kernel(n);

  return n+1;
}

function testMap() {
  var p = new ParallelArray(parallelRange());
  p.map(kernel, { mode: "par", expect: "disqualified" });
}

if (getBuildConfiguration().parallelJS) testMap();

