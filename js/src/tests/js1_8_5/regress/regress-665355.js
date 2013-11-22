var x = new ArrayBuffer(2);

var test = function(newProto) {
try {
    x.__proto__ = newProto;
    do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
} catch(e) {
    return true;
}
}

assertEq(test(x), true);
assertEq(test({}), true);
assertEq(test(null), true);

reportCompare(true, true);
