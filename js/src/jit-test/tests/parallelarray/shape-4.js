load(libdir + "eqArrayHelper.js");

function testShape() {
  // Test shape immutability.
  var p = new ParallelArray([1,2,3,4]);
  p.shape[0] = 0;
  p.shape[1] = 42;
  assertEq(p[0], 1);
  assertEqArray(p.shape, [4]);
}

// FIXME immutability not enforced
// testShape();
