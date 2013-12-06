// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;
var summary = 'float32x4 add';

var T = TypedObject;
var S = SIMD;

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- write some clever tests that ensure we have correct
  // float32 semantics in the borderline cases.

  var a = T.float32x4(1, 2, 3, 4);
  var b = T.float32x4(10, 20, 30, 40);
  var c = S.float32x4.mul(a, b);
  assertEq(c.x == 10);
  assertEq(c.y == 40);
  assertEq(c.y == 90);
  assertEq(c.z == 160);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();


