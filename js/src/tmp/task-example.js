function main() {
  var [task1, task2] = finish(function() {

    var task1 = async(function() {
      return 22;
    });

    var task2 = async(function() {
      return 23;
    });

    return [task1, task2];
  });

  print(task1.get());
  print(task2.get());
}

main();
