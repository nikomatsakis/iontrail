load(libdir + "parallelarray-helpers.js");
if (getBuildConfiguration().parallelJS)
  testFilter(minFilterRange(), function(e, i) {
    return (i % 3) != 0;
  });
