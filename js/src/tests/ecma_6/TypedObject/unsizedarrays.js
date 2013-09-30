// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 922115;
var summary = 'TypedObjects ArrayType implementation';

var ArrayType = TypedObject.ArrayType;
var StructType = TypedObject.StructType;
var uint8 = TypedObject.uint8;
var float32 = TypedObject.float32;
var uint32 = TypedObject.uint32;
var ObjectType = TypedObject.Object;


function runTests() {
  print(BUGNUMBER + ": " + summary);

  (function SimpleArrayOfTwoObjects() {
    var Objects = new ArrayType(ObjectType);
    var objects2 = new Objects(2, [{f: "Hello"},
                                   {f: "World"}]);
    assertEq(objects2[0].f, "Hello");
    assertEq(objects2[1].f, "World");
    assertEq(objects2.length, 2);
  })();

  (function EmbedUnsizedArraysBad() {
    var Objects = new ArrayType(ObjectType);
    assertThrows(() => new ArrayType(Objects));
    assertThrows(() => new StructType({f: Objects}));
  })();

  (function MultipleSizes() {
    var Uints = new ArrayType(uint32);
    var Point = new StructType({values: new ArrayType(uint32).dimension(3)});

    var uints = new Uints(3, [0, 1, 2]);
    var point = new Point({values: uints});

    assertEq(uints.length, point.values.length);
    for (var i = 0; i < uints.length; i++) {
      assertEq(uints[i], i);
      assertEq(uints[i], point.values[i]);
    }
  })();

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

runTests();

