// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;
var summary = 'float32x4 greaterThanOrEqual';

var T = TypedObject;
var S = SIMD;

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- write some clever tests that ensure we have correct
  // float32 semantics in the borderline cases.

  var a = T.float32x4(1, 20, 30, 40);
  var b = T.float32x4(10, 20, 30, 4);
  var c = S.float32x4.greaterThanOrEqual(b,a);
  assertEq(c.x == -1);
  assertEq(c.y == -1);
  assertEq(c.y == -1);
  assertEq(c.z == 0);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();


