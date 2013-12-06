// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;
var summary = 'float32x4 abs';

var T = TypedObject;
var S = SIMD;

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- write some clever tests that ensure we have correct
  // float32 semantics in the borderline cases.

  var a = T.float32x4(-1, 2, -3, 4);
  var c = S.float32x4.abs(a);
  assertEq(c.x == 1);
  assertEq(c.y == 2);
  assertEq(c.y == 3);
  assertEq(c.z == 4);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();


