load(libdir + "parallelarray-helpers.js");
compareAgainstArray(range(0, 1024), "filter", function(e, i) {
  return (i % 3) != 0;
});

