load(libdir + "parallelarray-helpers.js");

if (getBuildConfiguration().parallelJS)
  compareAgainstArray(parallelRange(), "map", function(e) { return e+1; });
