load(libdir + "parallelarray-helpers.js");

function wrapInObject(v) {
    var obj = {f: 2};
    obj.f += v;
    return obj;
}

if (getBuildConfiguration().parallelJS)
  compareAgainstArray(parallelRange(), "map", wrapInObject);
