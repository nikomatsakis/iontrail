/// Copyright (c) 2012 Ecma International.  All rights reserved. 
/// Ecma International makes this code available under the terms and conditions set
/// forth on http://hg.ecmascript.org/tests/test262/raw-file/tip/LICENSE (the 
/// "Use Terms").   Any redistribution of this code must retain the above 
/// copyright and this notice and otherwise comply with the Use Terms.
/**
 * @path ch07/7.6/7.6.1/7.6.1-3-4.js
 * @description Allow reserved words as property names by index assignment,verified with hasOwnProperty: new, var, catch
 */


function testcase() {
        var tokenCodes  = {};
        tokenCodes['new'] = 0;
        tokenCodes['var'] = 1;
        tokenCodes['catch'] = 2;
        var arr = [
            'new',
            'var',
            'catch'
            ];
        for(var p in tokenCodes) {       
            for(var p1 in arr) {                
                if(arr[p1] === p) {
                    if(!tokenCodes.hasOwnProperty(arr[p1])) {
                        do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
                    };
                }
            }
        }
        return true;
    }
runTestCase(testcase);
