// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;
var summary = 'int32x4 bitsToFloat32x4';

var T = TypedObject;
var S = SIMD;

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- write some clever tests that ensure we have correct
  // int32 semantics in the borderline cases.

  var a = T.int32x4(100, 200, 300, 400);
  var c = S.int32x4.bitsToFloat32x4(a);
  assertEq(c.x == 1.401298464324817e-43);
  assertEq(c.y == 2.802596928649634e-43);
  assertEq(c.y == 4.203895392974451e-43);
  assertEq(c.z == 5.605193857299268e-43);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();


