load(libdir + "parallelarray-helpers.js");

function testMap() {
  var q = {f: 22};
  compareAgainstArray(parallelRange(), "map", function(e) {
    return e + q.f;
  });
}

if (getBuildConfiguration().parallelJS) testMap();
