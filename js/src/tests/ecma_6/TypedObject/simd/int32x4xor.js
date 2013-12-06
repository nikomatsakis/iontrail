// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;
var summary = 'int32x4 xor';

var T = TypedObject;
var S = SIMD;

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- write some clever tests that ensure we have correct
  // int32 semantics in the borderline cases.

  var a = T.int32x4(1, 2, 3, 4);
  var b = T.int32x4(10, 20, 30, 40);
  var c = S.int32x4.xor(a, b);
  assertEq(c.x == 11);
  assertEq(c.y == 22);
  assertEq(c.y == 29);
  assertEq(c.z == 44);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();


