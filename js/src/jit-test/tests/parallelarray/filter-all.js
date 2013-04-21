load(libdir + "parallelarray-helpers.js");
compareAgainstArray(range(0, 1024), "filter", function() { return true; });
