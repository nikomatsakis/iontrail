load(libdir + "parallelarray-helpers.js");

function factorial(n) {
  if (n == 0)
    return 1;
  return n * factorial(n - 1);
}

if (getBuildConfiguration().parallelJS) {
  // Just to keep the running times down, don't pass in
  // huge values to factorial :)
  var range = parallelRange().map(function (n) n % 256);
  compareAgainstArray(range, "map", factorial);
}
