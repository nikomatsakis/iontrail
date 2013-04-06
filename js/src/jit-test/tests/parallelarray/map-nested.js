// |jit-test| thread-count: 1
// ^^^^^^^^^^^^^^^^^^^^^^^^^^ Note: use a low thread-count so as to keep
//                            the total work under control when running
//                            under --tbpl.  Otherwise, the arrays are so
//                            large that the interpreted sequential
//                            performance leads to timeouts.

// Test that we can successfully run map() inside of another parallel
// map().

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
