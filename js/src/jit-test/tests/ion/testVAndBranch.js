// vim: set ts=8 sts=4 et sw=4 tw=99:
function f(x) {
    if (x)
        return true;
    do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
}

assertEq(f(NaN), false);
assertEq(f(-0), false);
assertEq(f(3.3), true);
assertEq(f(0), false);
assertEq(f(3), true);
assertEq(f("hi"), true);
assertEq(f(""), false);
assertEq(f(true), true);
assertEq(f(false), false);
assertEq(f(undefined), false);
assertEq(f({}), true);
assertEq(f(null), false);

