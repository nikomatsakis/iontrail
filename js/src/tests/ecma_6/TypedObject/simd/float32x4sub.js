// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;

var summary = 'float32x4 sub';

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- Bug 948379: Amend to check for correctness of border cases.

  var a = float32x4(1, 2, 3, 4);
  var b = float32x4(10, 20, 30, 40);
  var c = SIMD.float32x4.sub(b,a);
  assertEq(c.x, 9);
  assertEq(c.y, 18);
  assertEq(c.z, 27);
  assertEq(c.w, 36);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
}

test();

