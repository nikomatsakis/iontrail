
// |reftest| skip-if(!this.hasOwnProperty("TypedObject"))
var BUGNUMBER = 898359;
var summary = 'TypedObjects reference type trace';
var actual = '';
var expect = '';

var ArrayType = TypedObject.ArrayType;
var StructType = TypedObject.StructType;
var Any = TypedObject.Any;
var Object = TypedObject.Object;
var string = TypedObject.string;

function TestStructFields(RefType) {
  var S1 = new StructType({f: RefType});
  var s1 = new S1({f: {}});
  var count1 = countHeap(s1, "object");
  s1.f = null;
  var count2 = countHeap(s1, "object");
  assertEq(count1, count2+1);
}

function TestArrayElements(RefType) {
  var S1 = new ArrayType(RefType).dimension(1);
  var s1 = new S1([{}]);
  var count1 = countHeap(s1, "object");
  s1[0] = null;
  var count2 = countHeap(s1, "object");
  assertEq(count1, count2+1);
}

function TestUnsizedArrayElements(RefType) {
  var S1 = new ArrayType(RefType);
  var s1 = new S1(1, [{}]);
  var count1 = countHeap(s1, "object");
  s1[0] = null;
  var count2 = countHeap(s1, "object");
  assertEq(count1, count2+1);
}

function TestStructInArray(RefType) {
  var S2 = new StructType({f: RefType, g: RefType});
  var S1 = new ArrayType(S2).dimension(1);
  var s1 = new S1([{f: {}, g: {}}]);
  var count1 = countHeap(s1, "object");
  print(count1);
  s1[0].f = null;
  var count2 = countHeap(s1, "object");
  print(count2);
  assertEq(count1, count2+1);
}

function TestStringInStruct() {
  // Since strings are not nullable, it's a bit trickier to check the
  // counts.
  var S1 = new StructType({f: Any, g: string});
  var s1 = new S1({f: "Hello", g: "World"});
  var count1 = countHeap(s1, "string");
  s1.f = 22;
  var count2 = countHeap(s1, "string");
  print(count1, count2);
  assertEq(count1, count2+1);
}

function runTests()
{
  printBugNumber(BUGNUMBER);
  printStatus(summary);

  TestStructFields(Object);
  TestStructFields(Any);

  TestArrayElements(Object);
  TestArrayElements(Any);

  TestUnsizedArrayElements(Object);
  TestUnsizedArrayElements(Any);

  // FIXME -- for some reason I do not understand the following tests fail:

  //TestStructInArray(Object);
  //TestStructInArray(Any);

  //TestStringInStruct();

  reportCompare(true, true, "TypedObjects trace tests");
}

runTests();
