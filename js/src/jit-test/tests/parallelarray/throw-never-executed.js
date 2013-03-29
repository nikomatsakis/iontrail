load(libdir + "parallelarray-helpers.js");

function inc(n) {
  if (n < 0) // never happens
    throw n;
  return n + 1;
}

if (getBuildConfiguration().parallelJS)
  compareAgainstArray(parallelRange(), "map", inc);
