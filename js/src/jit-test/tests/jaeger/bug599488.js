/* Don't crash. */
function foo(y) {
  var x = y;
  if (x != x)
    return true;
  do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
}
assertEq(foo("three"), false);
assertEq(foo(NaN), true);
