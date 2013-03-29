load(libdir + "parallelarray-helpers.js");
if (getBuildConfiguration().parallelJS)
  testFilter(parallelRange(), function() { return false; });
