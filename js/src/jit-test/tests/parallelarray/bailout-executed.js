load(libdir + "parallelarray-helpers.js");

// Test a function that bails out along a conditional path
// that is taken.  See also bailout-not-executed.js.

function makeObject(e, i, c) {
  if (inParallelSection()) {
    var v = {element: e, index: i, collection: c};
    delete v.index;
  }

  return 22;
}

function test() {
  var array = parallelRange();
  var array1 = array.map(makeObject);

  var pa = new ParallelArray(array);
  var pa1 = pa.map(makeObject, {mode: "par", expect: "disqualified"});

  assertStructuralEq(pa1, array1);
}

if (getBuildConfiguration().parallelJS)
  test();
