load(libdir + "asserts.js");

/*
 * Throw a TypeError if the trap reports an existing own property as
 * non-existent on a non-extensible object
 */
var target = {};
Object.defineProperty(target, 'foo', {
    configurable: true
});
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    'foo' in new Proxy(target, {
        has: function (target, name) {
            do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
        }
    });
}, TypeError);
