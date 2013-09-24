// Simple perf test of SIMD.add operation.  Use SIMD.add to average up elements in a Float32Array.
// Compare to scalar implementation of same function.
// Author: Peter Jensen
//load ('ecmascript_simd.js');
load('float32x4array.js');

function average (a) {
  var sum = 0.0;
  for (var i = 0, l = a.length; i < l; ++i) {
    sum += a[i];
  }
  return sum/a.length;
}

function SIMDaverage (a) {
  var a4   = new Float32x4Array(a.buffer);
  var sum4 = float32x4.splat(0.0);
  for (var i = 0, l = a4.length; i < l; ++i) {
    sum4 = SIMD.add(sum4, a4.getAt(i));
  }
  return (sum4.x + sum4.y + sum4.z + sum4.w) / a.length;
}

function init (a) {
  for (var i = 0, l = a.length; i < l; ++i) {
    a[i] = i;
  }
}

function sanityTest() {
  var a = new Float32Array(100);
  init(a);
  var av1 = average(a);
  var av2 = SIMDaverage(a);
  if (av1 != av2) {
    print("Non SIMD != SIMD (" + av1 + " != " + av2 + ")");
    return false;
  }
  else {
    return true;
  }
}

function timeIt(fct) {
  var a = new Float32Array(100000);
  init(a);
  var start = Date.now();
  for (var i = 0; i < 10; ++i) {
    avg = fct(a);
  }
  var stop = Date.now();
  return stop - start;
}

function main() {
  if (!sanityTest()) {
    return;
  }
  
  var non_simd = timeIt(average);
  print("Non SIMD time: " + non_simd + "ms");
  var simd     = timeIt(SIMDaverage);
  print("    SIMD time: " + simd + "ms");
  print("      Speedup: " + (non_simd / simd).toFixed(3));
}

function printFloat32x4(fx4) {
  print ("{x:" + fx4.x + ",y:" + fx4.y + ",z:" + fx4.z + ",w:" + fx4.w + "}");
}

function test() {
  var x = new Int32Array(20);
  var f = new Float32Array(x.buffer);
  var y = new Float64Array(x.buffer);
  var z = new Float32x4Array(x.buffer);
  print(x.length);
  print(y.length);
  print(z.length);
  f[0] = 1; f[1] = 2; f[2] = 3; f[3] = 4;
  printFloat32x4(z.getAt(0));
}

main();