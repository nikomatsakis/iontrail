function f() {
    try {
	for ( var i = 7; i > -2; i-- )
	    new Array(i).join('*');
    } catch (e) {
	return e instanceof RangeError;
    }
    do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
}
assertEq(f(), true);
