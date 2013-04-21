load(libdir + "parallelarray-helpers.js");

function MyClass(a) {
  this.a = a;
}

MyClass.prototype.getA = function() {
  return this.a;
}

function testMap() {
  var instances = range(22, 22+64).map(function (i) { return new MyClass(i); });

  // ICs should work.
  MyClass.prototype.getA = function() {
    return 222;
  };

  compareAgainstArray(instances, "map", function(e) e.getA());
}

testMap();

