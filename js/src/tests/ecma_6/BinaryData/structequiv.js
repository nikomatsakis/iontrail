// |reftest| skip-if(!this.hasOwnProperty("Type"))
var BUGNUMBER = 578700;
var summary = 'BinaryData Equivalent StructTypes';

function assertThrows(f) {
    var ok = false;
    try {
        f();
    } catch (exc) {
        ok = true;
    }
    if (!ok)
        throw new TypeError("Assertion failed: " + f + " did not throw as expected");
}

function runTests() {
    print(BUGNUMBER + ": " + summary);

    // Create a line:
    var PixelType1 = new StructType({x: uint8, y: uint8});
    var LineType1 = new StructType({from: PixelType1, to: PixelType1});
    var origin1 = new PixelType1({x: 0, y: 0});
    var unit1 = new PixelType1({x: 1, y: 1});
    var line1 = new LineType1({from: origin1, to: unit1});

    assertEq(line1.from.x, 0);
    assertEq(line1.from.y, 0);
    assertEq(line1.to.x, 1);
    assertEq(line1.to.y, 1);

    // Assign from a JS struct with the proper fields.
    line1.from = {x: 99, y: 3};
    assertEq(line1.from.x, 99);
    assertEq(line1.from.y, 3);

    // Assign from a JS struct with extra fields.
    assertThrows(function() { line1.from = {w: 10, x: 99, y: 3, z: 11}; });

    // Define the same pixel type. Assignments should be permitted.
    var PixelType2 = new StructType({x: uint8, y: uint8});
    var somePoint2 = new PixelType2({x: 22, y: 44});
    var anotherPoint2 = new PixelType2({x: 61, y: 11});
    line1.from = somePoint2;
    line1.to = anotherPoint2;

    assertEq(line1.from.x, 22);
    assertEq(line1.from.y, 44);
    assertEq(line1.to.x, 61);
    assertEq(line1.to.y, 11);

    // Define the pixel type with field order reversed.
    // Assignments should not be permitted.
    var PixelType3 = new StructType({y: uint8, x: uint8});
    var somePoint3 = new PixelType3({x: 22, y: 44});
    assertThrows(function() { line1.from = somePoint3; });

    if (typeof reportCompare === "function")
        reportCompare(true, true);
    print("Tests complete");
}

runTests();
