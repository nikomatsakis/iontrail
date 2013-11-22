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
var caught = false;
assertThrowsInstanceOf(function () {
    ({}).hasOwnProperty.call(new Proxy(target, {
        hasOwn: function (target, name) {
            do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
        }
    }), 'foo');
}, TypeError);
