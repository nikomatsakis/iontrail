/// Copyright (c) 2012 Ecma International.  All rights reserved. 
/// Ecma International makes this code available under the terms and conditions set
/// forth on http://hg.ecmascript.org/tests/test262/raw-file/tip/LICENSE (the 
/// "Use Terms").   Any redistribution of this code must retain the above 
/// copyright and this notice and otherwise comply with the Use Terms.
/**
 * @path ch13/13.2/13.2-16-s.js
 * @description StrictMode - enumerating over a function object looking for 'arguments' fails inside the function
 * @onlyStrict
 */



function testcase() {
            var foo = new Function("'use strict'; for (var tempIndex in this) {if (tempIndex===\"arguments\") {do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);}}; return true;");
            return foo();
}
runTestCase(testcase);