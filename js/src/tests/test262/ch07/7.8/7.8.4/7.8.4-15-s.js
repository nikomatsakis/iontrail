/// Copyright (c) 2012 Ecma International.  All rights reserved. 
/// Ecma International makes this code available under the terms and conditions set
/// forth on http://hg.ecmascript.org/tests/test262/raw-file/tip/LICENSE (the 
/// "Use Terms").   Any redistribution of this code must retain the above 
/// copyright and this notice and otherwise comply with the Use Terms.
/**
 * @path ch07/7.8/7.8.4/7.8.4-15-s.js
 * @description An OctalEscapeSequence is not allowed in a String under Strict Mode
 * @onlyStrict
 */




function testcase()
{
  try 
  {
    eval('"use strict"; var x = "\\37";');
    do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
}
runTestCase(testcase);