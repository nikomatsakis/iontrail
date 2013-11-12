// Test that we can optimize stores of entire objects.

if (!this.hasOwnProperty("TypedObject"))
  quit();

var PointType = new TypedObject.StructType({x: TypedObject.float64,
                                            y: TypedObject.float64});
var LineType = new TypedObject.StructType({source: PointType,
                                           target: PointType});

function manhattenDistance(line) {
  return (Math.abs(line.target.x - line.source.x) +
          Math.abs(line.target.y - line.source.y));
}

function budge(line) {
  line.source = {x: line.source.x + 1,
                 y: line.source.y + 1};
  line.target = line.source;
}

function foo() {
  var N = 30000;
  var points = [];
  var obj;
  var s;

  var fromAToB = new LineType({source: {x: 22, y: 44},
                               target: {x: 66, y: 88}});

  for (var i = 0; i < N; i++) {
    budge(fromAToB);
    assertEq(fromAToB.source.x, 22 + i + 1);
    assertEq(fromAToB.source.y, 44 + i + 1);
    assertEq(fromAToB.target.x, 22 + i + 1);
    assertEq(fromAToB.target.y, 44 + i + 1);
  }
}

foo();
