// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 938728;
var summary = 'int32x4 alignment';

var StructType = TypedObject.StructType;
var uint8 = TypedObject.uint8;
var int32x4 = TypedObject.int32x4;

function test() {
  print(BUGNUMBER + ": " + summary);

  assertEq(int32x4.byteLength, 16);
  assertEq(int32x4.byteAlignment, 16);

  var Compound = new StructType({c: uint8, d: uint8, i: int32x4});
  assertEq(Compound.fieldOffsets["c"], 0);
  assertEq(Compound.fieldOffsets["d"], 1);
  assertEq(Compound.fieldOffsets["i"], 16);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();
