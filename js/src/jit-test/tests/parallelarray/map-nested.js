load(libdir + "parallelarray-helpers.js")

function test() {
  var pa0 = new ParallelArray(range(0, 256));
  var pa1 = new ParallelArray(ParallelSize, function (x) {
    return pa0.map(function(y) { return x * 1000 + y; });
  }, {mode: "par", expect: "success"});

  for (var x = 0; x < ParallelSize; x++) {
    var pax = pa1.get(x);
    for (var y = 0; y < 256; y++) {
      assertEq(pax.get(y), x * 1000 + y);
    }
  }
}

if (getBuildConfiguration().parallelJS) test();
