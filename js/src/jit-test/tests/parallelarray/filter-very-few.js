load(libdir + "parallelarray-helpers.js");
if (getBuildConfiguration().parallelJS)
  testFilter(parallelRange(), function(i) { return i <= 1 || i >= 1022; });
