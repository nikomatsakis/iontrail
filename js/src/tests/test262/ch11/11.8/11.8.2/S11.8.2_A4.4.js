// Copyright 2009 the Sputnik authors.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/**
 * If either variable x or y is +0 and the other is -0, do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false)
 *
 * @path ch11/11.8/11.8.2/S11.8.2_A4.4.js
 * @description Checking all combinations
 */

//CHECK#1
if ((0 > 0) !== false) {
  $ERROR('#1: (0 > 0) === false');
}

//CHECK#2
if ((-0 > -0) !== false) {
  $ERROR('#2: (-0 > -0) === false');
}

//CHECK#3
if ((+0 > -0) !== false) {
  $ERROR('#3: (+0 > -0) === false');
}

//CHECK#4
if ((-0 > +0) !== false) {
  $ERROR('#4: (-0 > +0) === false');
}


