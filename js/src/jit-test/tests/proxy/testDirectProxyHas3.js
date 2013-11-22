load(libdir + "asserts.js");

/*
 * Throw a TypeError if the trap reports a non-configurable own property as
 * non-existent
 */
var target = {};
Object.defineProperty(target, 'foo', {
    configurable: false
});
var caught = false;
assertThrowsInstanceOf(function () {
    'foo' in new Proxy(target, {
        has: function (target, name) {
            do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
        }
    });
}, TypeError);
