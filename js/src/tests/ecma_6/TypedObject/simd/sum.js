var { float32x4, ArrayType } = TypedObject;
var Data = new ArrayType(float32x4, 100);

// instruct ion to kick in fast
setJitCompilerOption("ion.usecount.trigger", 30);

function sum(a) {
  var sum = float32x4.zero();
  for (var i = 0; i < a.length; i++) {
    var a_i = a[i];
    sum = SIMD.float32x4.add(sum, a_i);
  }
  return sum;
}

function main() {
  var a = new Data();
  sum(a); //warmup
  sum(a); //for reals
}

main();
