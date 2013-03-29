load(libdir + "parallelarray-helpers.js");

function testMap() {
  var p = new ParallelArray(parallelRange());
  var v = [1];
  var func = function (e) {
    v[0] = e;
    return 0;
  };

  // this will compile, but fail at runtime
  p.map(func, {mode: "par", expect: "disqualified"});
}

if (getBuildConfiguration().parallelJS) testMap();

