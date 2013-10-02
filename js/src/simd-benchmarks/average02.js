//load ('ecmascript_simd.js');
load ('float32x4array.js');

var a = new Float32Array(100);

function simdAverage() {
    var sum4;
    var a4 = new Float32x4Array(a.buffer);
    sum4 = float32x4.splat(0.0);
    for (var j = 0, l = a4.length; j < l; ++j) {
        sum4 = SIMD.add(sum4, a4.getAt(j));
    }
    return (sum4.x + sum4.y + sum4.z + sum4.w) / a.length;
}

function init () {
  for (var i = 0, l = a.length; i < l; ++i) {
    a[i] = i;
  }
}

function main () {
  init();
  var avg;
  for (var i = 0; i < 10000; ++i) {
    avg = simdAverage();
  }
  print(avg);
}

main();
