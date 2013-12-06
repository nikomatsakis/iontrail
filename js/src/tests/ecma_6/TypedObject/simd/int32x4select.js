// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 946042;
var summary = 'int32x4 select';

var T = TypedObject;
var S = SIMD;

function test() {
  print(BUGNUMBER + ": " + summary);

  // FIXME -- write some clever tests that ensure we have correct
  // int32 semantics in the borderline cases.

  var a = T.float32x4(0.0,4.0,9.0,16.0)
  var b = T.float32x4(1.0,2.0,3.0,4.0)
  var sel_ttff = SIMD.int32x4.bool(true, true, false, false);
  var c = S.int32x4.select(sel_ttff,a,b);
  assertEq(c.x == 0);
  assertEq(c.y == 4);
  assertEq(c.y == 3);
  assertEq(c.z == 4);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();


