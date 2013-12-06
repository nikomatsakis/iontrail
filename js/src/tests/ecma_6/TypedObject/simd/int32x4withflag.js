// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;
var summary = 'int32x4 with';

var T = TypedObject;
var S = SIMD;

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- write some clever tests that ensure we have correct
  // int32 semantics in the borderline cases.

  var a = T.int32x4(1, 2, 3, 4);
  var x = S.int32x4.withFlagX(a, true);
  var y = S.int32x4.withFlagY(a, false);
  var z = S.int32x4.withFlagZ(a, false);
  var w = S.int32x4.withFlagW(a, true);
  assertEq(x.x == -1);
  assertEq(y.y == 0);
  assertEq(z.z == 0);
  assertEq(w.w == -1);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();


