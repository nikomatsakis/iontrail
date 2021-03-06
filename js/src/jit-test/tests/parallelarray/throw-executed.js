load(libdir + "parallelarray-helpers.js");

function test() {
  var x = new Error();
  function inc(n) {
    if (parallelJSActive()) // wait until par execution, then throw
      throw x;
    return n + 1;
  }
  var x = new ParallelArray(range(0, 2048));

  // the disqualification occurs because all parallel executions throw
  // exceptions:
  x.map(inc, {mode: "par", expect: "disqualified"});
}
test();

