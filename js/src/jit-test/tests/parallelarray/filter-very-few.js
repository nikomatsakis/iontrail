load(libdir + "parallelarray-helpers.js");
compareAgainstArray(range(0, 1024), "filter", function(i) { return i <= 1 || i >= 1022; });
