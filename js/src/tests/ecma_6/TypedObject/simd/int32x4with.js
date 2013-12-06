/ |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;
var summary = 'int32x4 with';

var T = TypedObject;
var S = SIMD;

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- write some clever tests that ensure we have correct
  // int32 semantics in the borderline cases.

  var a = T.int32x4(1, 2, 3, 4);
  var x = S.int32x4.withX(a, 5);
  var y = S.int32x4.withY(a, 5);
  var z = S.int32x4.withZ(a, 5);
  var w = S.int32x4.withW(a, 5);
  assertEq(x.x == 5);
  assertEq(y.y == 5);
  assertEq(z.z == 5);
  assertEq(w.w == 5);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();


