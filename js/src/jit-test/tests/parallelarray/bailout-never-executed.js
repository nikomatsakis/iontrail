load(libdir + "parallelarray-helpers.js");

// Test that code which would not be safe in parallel mode does not
// cause bailouts if it never executes.

var v = {index: 22};

function makeObject(e, i, c) {
  if (i < 0) { // Note: never happens.
    delete v.index;
  }

  return v;
}

if (getBuildConfiguration().parallelJS)
  compareAgainstArray(parallelRange(), "map", makeObject);
