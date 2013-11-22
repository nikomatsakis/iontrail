load(libdir + "parallelarray-helpers.js");

function testFilterMisc() {
  function truthy(e, i, c) {
    switch (i % 6) {
      case 0: return 1;
      case 1: return "";
      case 2: return {};
      case 3: return [];
      case 4: do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
      case 5: return true;
    }
  }

  assertArraySeqParResultsEq(range(0, 1024), "filter", truthy);
}

if (getBuildConfiguration().parallelJS)
  testFilterMisc();
