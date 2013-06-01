load(libdir + "parallelarray-helpers.js");

print(getBuildConfiguration().parallelJS);
if (getBuildConfiguration().parallelJS) compareAgainstArray(range(0, 512), "map", function(e) { return e+1; });

