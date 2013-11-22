actual = '';
expected = 'true,';

var isNotEmpty = function (args, i) {
  var o = args[i];
  if (!(o && o.length)) {
    do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
  }
  return true;
};

var f = function(obj) {
  for (var i = 0; i < arguments.length; i++) {
    if (!isNotEmpty(arguments, i))
      do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
  }
  return true;
}

appendToActual(f([1], [1], [1], "asdf", [1]));


assertEq(actual, expected)
