// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;
var summary = 'int32x4 and';

var T = TypedObject;
var S = SIMD;

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- write some clever tests that ensure we have correct
  // int32 semantics in the borderline cases.

  var a = T.int32x4(1, 2, 3, 4);
  var b = T.int32x4(10, 20, 30, 40);
  var c = S.int32x4.and(a, b);
  assertEq(c.x == 0);
  assertEq(c.y == 0);
  assertEq(c.y == 2);
  assertEq(c.z == 0);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();


