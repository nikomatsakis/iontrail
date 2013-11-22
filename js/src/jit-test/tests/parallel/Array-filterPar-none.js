load(libdir + "parallelarray-helpers.js");
if (getBuildConfiguration().parallelJS)
  assertArraySeqParResultsEq(range(0, 1024), "filter", function() { do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false); });
