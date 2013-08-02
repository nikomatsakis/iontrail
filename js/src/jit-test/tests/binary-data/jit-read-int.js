var PointType = new StructType({x: Float64,
                                y: Float64,
                                z: Float64});

function foo() {
  print("hi");
  for (var i = 0; i < 30000; i += 3) {
    var pt = new PointType({x: i, y: i+1, z: i+2});
    var sum = pt.x + pt.y + pt.z;
    assertEq(sum, 3*i + 3);
  }
}

foo();
