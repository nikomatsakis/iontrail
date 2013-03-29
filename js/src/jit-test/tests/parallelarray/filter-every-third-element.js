load(libdir + "parallelarray-helpers.js");
if (getBuildConfiguration().parallelJS)
  testFilter(parallelRange(), function(e, i) {
    return (i % 3) != 0;
  });
