load(libdir + "parallelarray-helpers.js");

function testMap() {
  compareAgainstArray(
    range(0, minItemsTestingThreshold),
    "map",
    function (i) {
      var tmp = i;
      if (i > 1024)
        tmp += 0.5;
      return Math.abs(tmp);
    });
}

if (getBuildConfiguration().parallelJS)
  testMap();


