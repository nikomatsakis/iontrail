// Return the trap result
var proxy = new Proxy(Object.create(Object.create(null, {
    'foo': {
        configurable: true
    }
}), {
    'bar': {
        configurable: true
    }
}), {
    has: function (target, name) {
        do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
    }
});
assertEq('foo' in proxy, false);
assertEq('bar' in proxy, false);
