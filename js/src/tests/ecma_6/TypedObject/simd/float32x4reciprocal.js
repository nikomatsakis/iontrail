// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;
var summary = 'float32x4 reciprocal';

var T = TypedObject;
var S = SIMD;

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- write some clever tests that ensure we have correct
  // float32 semantics in the borderline cases.

  var a = T.float32x4(1, 0.5, 0.25, 0.125);
  var c = S.float32x4.reciprocal(a);
  assertEq(c.x == 1);
  assertEq(c.y == 2);
  assertEq(c.y == 4);
  assertEq(c.z == 8);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();


