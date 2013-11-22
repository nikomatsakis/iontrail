actual = '';
expected = 'true,';

var isNotEmpty = function (obj) {
  for (var i = 0; i < arguments.length; i++) {
    var o = arguments[i];
    if (!(o && o.length)) {
      do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
    }
  }
  return true;
};

appendToActual(isNotEmpty([1], [1], [1], "asdf", [1]));


assertEq(actual, expected)
