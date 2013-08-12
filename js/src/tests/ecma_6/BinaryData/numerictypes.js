// |reftest| skip-if(!this.hasOwnProperty("Type"))
var BUGNUMBER = 578700;
var summary = 'BinaryData numeric types';
var actual = '';
var expect = '';

function runTests()
{
    printBugNumber(BUGNUMBER);
    printStatus(summary);

    var TestPassCount = 0;
    var TestFailCount = 0;
    var TestTodoCount = 0;

    var TODO = 1;

    function check(fun, todo) {
        var thrown = null;
        var success = false;
        try {
            success = fun();
        } catch (x) {
            thrown = x;
        }

        if (thrown)
            success = false;

        if (todo) {
            TestTodoCount++;

            if (success) {
                var ex = new Error;
                print ("=== TODO but PASSED? ===");
                print (ex.stack);
                print ("========================");
            }

            return;
        }

        if (success) {
            TestPassCount++;
        } else {
            TestFailCount++;

            var ex = new Error;
            print ("=== FAILED ===");
            print (ex.stack);
            if (thrown) {
                print ("    threw exception:");
                print (thrown);
            }
            print ("==============");
        }
    }

    function checkThrows(fun, todo) {
        var thrown = false;
        try {
            fun();
        } catch (x) {
            thrown = true;
        }

        check(function() thrown, todo);
    }

    var types = [Uint8, Uint16, Uint32, Int8, Int16, Int32];
    var strings = ["Uint8", "Uint16", "Uint32", "Int8", "Int16", "Int32"];
    for (var i = 0; i < types.length; i++) {
        var type = types[i];

        check(function() type(true) === 1);
        check(function() type(false) === 0);
        check(function() type(+Infinity) === 0);
        check(function() type(-Infinity) === 0);
        check(function() type(NaN) === 0);
        check(function() type.toString() === strings[i]);
        check(function() type(null) == 0);
        check(function() type(undefined) == 0);
        check(function() type([]) == 0);
        check(function() type({}) == 0);
        check(function() type(/abcd/) == 0);

        checkThrows(function() new type());
    }

    var floatTypes = [Float32, Float64];
    var floatStrings = ["Float32", "Float64"];
    for (var i = 0; i < floatTypes.length; i++) {
        var type = floatTypes[i];

        check(function() type(true) === 1);
        check(function() type(false) === 0);
        check(function() type(+Infinity) === Infinity);
        check(function() type(-Infinity) === -Infinity);
        check(function() Number.isNaN(type(NaN)));
        check(function() type.toString() === floatStrings[i]);
        check(function() type(null) == 0);
        check(function() Number.isNaN(type(undefined)));
        check(function() Number.isNaN(type([])));
        check(function() Number.isNaN(type({})));
        check(function() Number.isNaN(type(/abcd/)));

        checkThrows(function() new type());
    }

    ///// test ranges and creation
    /// Uint8
    // valid
    check(function() Uint8(0) == 0);
    check(function() Uint8(-0) == 0);
    check(function() Uint8(129) == 129);
    check(function() Uint8(255) == 255);

    if (typeof ctypes != "undefined") {
        check(function() Uint8(ctypes.Uint64(99)) == 99);
        check(function() Uint8(ctypes.Int64(99)) == 99);
    }

    // overflow is allowed for explicit conversions
    check(function() Uint8(-1) == 255);
    check(function() Uint8(-255) == 1);
    check(function() Uint8(256) == 0);
    check(function() Uint8(2345678) == 206);
    check(function() Uint8(3.14) == 3);
    check(function() Uint8(342.56) == 86);
    check(function() Uint8(-342.56) == 170);

    if (typeof ctypes != "undefined") {
        checkThrows(function() Uint8(ctypes.Uint64("18446744073709551615")) == 255);
        checkThrows(function() Uint8(ctypes.Int64("0xcafebabe")) == 190);
    }

    /// Uint8clamped
    // valid
    check(function() Uint8Clamped(0) == 0);
    check(function() Uint8Clamped(-0) == 0);
    check(function() Uint8Clamped(129) == 129);
    check(function() Uint8Clamped(-30) == 0);
    check(function() Uint8Clamped(254.5) == 254);
    check(function() Uint8Clamped(257) == 255);
    check(function() Uint8Clamped(513) == 255);
    check(function() Uint8Clamped(60000) == 255);

    // strings
    check(function() Uint8("0") == 0);
    check(function() Uint8("255") == 255);
    check(function() Uint8("256") == 0);
    check(function() Uint8("0x0f") == 15);
    check(function() Uint8("0x00") == 0);
    check(function() Uint8("0xff") == 255);
    check(function() Uint8("0x1ff") == 255);
    // in JS, string literals with leading zeroes are interpreted as decimal
    check(function() Uint8("-0777") == 247);
    check(function() Uint8("-0xff") == 0);

    /// uint16
    // valid
    check(function() Uint16(65535) == 65535);

    if (typeof ctypes != "undefined") {
        check(function() Uint16(ctypes.Uint64("0xb00")) == 2816);
        check(function() Uint16(ctypes.Int64("0xb00")) == 2816);
    }

    // overflow is allowed for explicit conversions
    check(function() Uint16(-1) == 65535);
    check(function() Uint16(-65535) == 1);
    check(function() Uint16(-65536) == 0);
    check(function() Uint16(65536) == 0);

    if (typeof ctypes != "undefined") {
        check(function() Uint16(ctypes.Uint64("18446744073709551615")) == 65535);
        check(function() Uint16(ctypes.Int64("0xcafebabe")) == 47806);
    }

    // strings
    check(function() Uint16("0x1234") == 0x1234);
    check(function() Uint16("0x00") == 0);
    check(function() Uint16("0xffff") == 65535);
    check(function() Uint16("-0xffff") == 0);
    check(function() Uint16("0xffffff") == 0xffff);

    // wrong types
    check(function() Uint16(3.14) == 3); // c-like casts in explicit conversion

    print("done");

    reportCompare(0, TestFailCount, "BinaryData numeric type tests");
}

runTests();
