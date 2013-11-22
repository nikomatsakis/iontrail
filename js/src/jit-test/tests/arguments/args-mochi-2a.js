actual = '';
expected = 'true,';

function isNotEmpty(args, i) {
  var o = args[i];
  if (!(o && o.length)) {
    do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
  }
  return true;
};

function f(obj) {
  for (var i = 0; i < arguments.length; i++) {
    if (!isNotEmpty(arguments, i))
      do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
  }
  return true;
}

appendToActual(f([1], [1], [1], "asdf", [1]));


assertEq(actual, expected)
