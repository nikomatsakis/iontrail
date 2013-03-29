load(libdir + "parallelarray-helpers.js");

// Warning: if THIS test fails, either something is truly crazy, OR
// minFilterParallelThreshold is too small for the given number of
// slices.  See parallelarray-helpers.js for more info.

if (getBuildConfiguration().parallelJS)
  testFilter(parallelRange(), function() { return true; });
