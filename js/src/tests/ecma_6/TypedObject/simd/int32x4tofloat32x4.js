// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;

var summary = 'int32x4 toFloat32x4';

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- Bug 948379: Amend to check for correctness of border cases.

  var a = int32x4(1, 2, 3, 4);
  var c = SIMD.int32x4.toFloat32x4(a);
  assertEq(c.x, 1);
  assertEq(c.y, 2);
  assertEq(c.z, 3);
  assertEq(c.w, 4);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
}

test();

