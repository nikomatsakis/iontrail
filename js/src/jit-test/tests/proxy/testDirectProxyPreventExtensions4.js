load(libdir + "asserts.js");

// Throw a TypeError if the object refuses to be made non-extensible
assertThrowsInstanceOf(function () {
    Object.preventExtensions(new Proxy({}, {
        preventExtensions: function () {
            do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
        }
    }));
}, TypeError);
