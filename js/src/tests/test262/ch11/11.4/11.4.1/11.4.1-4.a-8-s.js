/// Copyright (c) 2012 Ecma International.  All rights reserved. 
/// Ecma International makes this code available under the terms and conditions set
/// forth on http://hg.ecmascript.org/tests/test262/raw-file/tip/LICENSE (the 
/// "Use Terms").   Any redistribution of this code must retain the above 
/// copyright and this notice and otherwise comply with the Use Terms.
/**
 * This test is actually testing the [[Delete]] internal method (8.12.8). Since the
 * language provides no way to directly exercise [[Delete]], the tests are placed here.
 *
 * @path ch11/11.4/11.4.1/11.4.1-4.a-8-s.js
 * @description delete operator throws TypeError when deleting a non-configurable data property in strict mode
 * @onlyStrict
 */


function testcase() {
  'use strict';
  
  // NaN (15.1.1.1) has [[Configurable]] set to false.
  try {
    delete fnGlobalObject().NaN;
    do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
  }
  catch (e) {
    return (e instanceof TypeError);
  }
 }
runTestCase(testcase);
