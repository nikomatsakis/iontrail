/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * JS SIMD functions.
 * Spec matching polyfill:
 * https://github.com/johnmccutchan/ecmascript_simd/blob/master/src/ecmascript_simd_tests.js
 */

function IsVectorDatum(val, type) {
  if (!IsObject(val))
    return false;
  if (!ObjectIsTypedDatum(val))
    return false;
  var typeRepr = DATUM_TYPE_REPR(val);
  if (REPR_KIND(typeRepr) != JS_TYPEREPR_X4_KIND)
    return false;
  return REPR_TYPE(typeRepr) == type;
}

function Float32x4Zero() {
  var T = GetTypedObjectModule();
  return T.float32x4(0.0,0.0,0.0,0.0);
}

function Int32x4Zero() {
  var T = GetTypedObjectModule();
  return T.int32x4(0.0,0.0,0.0,0.0);
}

function Int32x4Bool(x, y, z, w) {
  var T = GetTypedObjectModule();
  return T.int32x4(x ? -1 : 0x0,
                   y ? -1 : 0x0,
                   z ? -1 : 0x0,
                   w ? -1 : 0x0);
}

function Float32x4Scale(float32x4val, scalar) {
  if (!IsVectorDatum(float32x4val, JS_X4TYPEREPR_FLOAT32))
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS);
  var lane0 = Load_float32(float32x4val, 0<<2);
  var lane1 = Load_float32(float32x4val, 1<<2);
  var lane2 = Load_float32(float32x4val, 2<<2);
  var lane3 = Load_float32(float32x4val, 3<<2);
  var T = GetTypedObjectModule();
  return T.float32x4(lane0*scalar, lane1*scalar, lane2*scalar, lane3*scalar);
}

function Float32x4Clamp(float32x4val, lowerLimit, upperLimit) {
  if (!IsVectorDatum(float32x4val, JS_X4TYPEREPR_FLOAT32))
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS);
  var valX = Load_float32(float32x4val, 0<<2);
  var valY = Load_float32(float32x4val, 1<<2);
  var valZ = Load_float32(float32x4val, 2<<2);
  var valW = Load_float32(float32x4val, 3<<2);
  var llX = Load_float32(lowerLimit, 0<<2);
  var llY = Load_float32(lowerLimit, 1<<2);
  var llZ = Load_float32(lowerLimit, 2<<2);
  var llW = Load_float32(lowerLimit, 3<<2);
  var ulX = Load_float32(upperLimit, 0<<2);
  var ulY = Load_float32(upperLimit, 1<<2);
  var ulZ = Load_float32(upperLimit, 2<<2);
  var ulW = Load_float32(upperLimit, 3<<2);
  var cx = valX < llX ? llX : valX;
  var cy = valY < llY ? llY : valY;
  var cz = valZ < llZ ? llZ : valZ;
  var cw = valW < llW ? llW : valW;
  cx = cx > ulX ? ulX : cx;
  cy = cy > ulY ? ulY : cy;
  cz = cz > ulZ ? ulZ : cz;
  cw = cw > ulW ? ulW : cw;
  var T = GetTypedObjectModule();
  return T.float32x4(cx, cy, cz, cw);
}

function Int32x4Select(int32x4val, trueValue, falseValue) {
  if (!IsVectorDatum(int32x4val, JS_X4TYPEREPR_INT32) ||
       !IsVectorDatum(trueValue, JS_X4TYPEREPR_FLOAT32) ||
       !IsVectorDatum(falseValue, JS_X4TYPEREPR_FLOAT32)) 
  {
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS);
  }
  var tv = Float32x4BitsToInt32x4(trueValue);
  var fv = Float32x4BitsToInt32x4(falseValue);
  var tr = Int32x4And(int32x4val,tv);
  var nt = Int32x4Not(int32x4val);
  var fr = Int32x4And(nt,fv);
  var otf = Int32x4Or(tr,fr);
  return Int32x4BitsToFloat32x4(otf);
}

#define DECLARE_SIMD_SPLAT(type32, Type32, TYPE32, OPNAME) \
function Type32##x4##OPNAME (scalar) { \
  var T = GetTypedObjectModule(); \
  return T.type32##x4(scalar, scalar, scalar, scalar); \
}

#define DECLARE_SIMD_CONVERT(type32, Type32, TYPE32, OPNAME, ret32) \
function Type32##x4##OPNAME (x4val0) { \
  if (!IsVectorDatum(x4val0, JS_X4TYPEREPR_##TYPE32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = Load_##type32(x4val0, 0<<2); \
  var lane1 = Load_##type32(x4val0, 1<<2); \
  var lane2 = Load_##type32(x4val0, 2<<2); \
  var lane3 = Load_##type32(x4val0, 3<<2); \
  var T = GetTypedObjectModule(); \
  return T.ret32##x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_CONVERT_BITWISE(type32, Type32, TYPE32, OPNAME, ret32) \
function Type32##x4##OPNAME (x4val0) { \
  if (!IsVectorDatum(x4val0, JS_X4TYPEREPR_##TYPE32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = Load_##ret32(x4val0, 0<<2); \
  var lane1 = Load_##ret32(x4val0, 1<<2); \
  var lane2 = Load_##ret32(x4val0, 2<<2); \
  var lane3 = Load_##ret32(x4val0, 3<<2); \
  var T = GetTypedObjectModule(); \
  return T.ret32##x4(lane0, lane1, lane2, lane3); \
}


#define DECLARE_SIMD_OP_1ARG(type32, Type32, TYPE32, OPNAME, OP) \
function Type32##x4##OPNAME (x4val0) { \
  if (!IsVectorDatum(x4val0, JS_X4TYPEREPR_##TYPE32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = OP(Load_##type32(x4val0, 0<<2)); \
  var lane1 = OP(Load_##type32(x4val0, 1<<2)); \
  var lane2 = OP(Load_##type32(x4val0, 2<<2)); \
  var lane3 = OP(Load_##type32(x4val0, 3<<2)); \
  var T = GetTypedObjectModule(); \
  return T.type32##x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_OP_2ARGS(type32, Type32, TYPE32, OPNAME, OP) \
function Type32##x4##OPNAME (x4val0, x4val1) { \
  if (!IsVectorDatum(x4val0, JS_X4TYPEREPR_##TYPE32) || \
      !IsVectorDatum(x4val1, JS_X4TYPEREPR_##TYPE32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = OP(Load_##type32(x4val0, 0<<2), Load_##type32(x4val1, 0<<2)); \
  var lane1 = OP(Load_##type32(x4val0, 1<<2), Load_##type32(x4val1, 1<<2)); \
  var lane2 = OP(Load_##type32(x4val0, 2<<2), Load_##type32(x4val1, 2<<2)); \
  var lane3 = OP(Load_##type32(x4val0, 3<<2), Load_##type32(x4val1, 3<<2)); \
  var T = GetTypedObjectModule(); \
  return T.type32##x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_OP_COMP(type32, Type32, TYPE32, OPNAME, OP) \
function Type32##x4##OPNAME (x4val0, x4val1) { \
  if (!IsVectorDatum(x4val0, JS_X4TYPEREPR_##TYPE32) || \
      !IsVectorDatum(x4val1, JS_X4TYPEREPR_##TYPE32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = OP(Load_##type32(x4val0, 0<<2), Load_##type32(x4val1, 0<<2)); \
  var lane1 = OP(Load_##type32(x4val0, 1<<2), Load_##type32(x4val1, 1<<2)); \
  var lane2 = OP(Load_##type32(x4val0, 2<<2), Load_##type32(x4val1, 2<<2)); \
  var lane3 = OP(Load_##type32(x4val0, 3<<2), Load_##type32(x4val1, 3<<2)); \
  var T = GetTypedObjectModule(); \
  return T.int32x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_OP_WITH(type32, Type32, TYPE32, SLOT, NAME, OP) \
function Type32##x4##NAME (x4val0, x4val1) { \
  if (!IsVectorDatum(x4val0, JS_X4TYPEREPR_##TYPE32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = Load_##type32(x4val0, 0<<2); \
  var lane1 = Load_##type32(x4val0, 1<<2); \
  var lane2 = Load_##type32(x4val0, 2<<2); \
  var lane3 = Load_##type32(x4val0, 3<<2); \
  lane##SLOT = OP(x4val1); \
  var T = GetTypedObjectModule(); \
  return T.type32##x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_SHUFFLE(type32, Type32, TYPE32) \
function Type32##x4Shuffle (x4val0, mask) { \
  if (!IsVectorDatum(x4val0, JS_X4TYPEREPR_##TYPE32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = Load_##type32(x4val0, ((mask) & 0x3)<<2); \
  var lane1 = Load_##type32(x4val0, ((mask >> 2) & 0x3)<<2); \
  var lane2 = Load_##type32(x4val0, ((mask >> 4) & 0x3)<<2); \
  var lane3 = Load_##type32(x4val0, ((mask >> 6) & 0x3)<<2); \
  var T = GetTypedObjectModule(); \
  return T.type32##x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_SHUFFLEMIX(type32, Type32, TYPE32) \
function Type32##x4ShuffleMix (x4val0, x4val1, mask) { \
  if (!IsVectorDatum(x4val0, JS_X4TYPEREPR_##TYPE32) || \
      !IsVectorDatum(x4val1, JS_X4TYPEREPR_##TYPE32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = Load_##type32(x4val0, ((mask) & 0x3)<<2); \
  var lane1 = Load_##type32(x4val0, ((mask >> 2) & 0x3)<<2); \
  var lane2 = Load_##type32(x4val1, ((mask >> 4) & 0x3)<<2); \
  var lane3 = Load_##type32(x4val1, ((mask >> 6) & 0x3)<<2); \
  var T = GetTypedObjectModule(); \
  return T.type32##x4(lane0, lane1, lane2, lane3); \
}

DECLARE_SIMD_SPLAT(float32,Float32, FLOAT32, Splat)
DECLARE_SIMD_OP_1ARG(float32, Float32, FLOAT32, Abs, ABS)
DECLARE_SIMD_OP_1ARG(float32, Float32, FLOAT32, Neg, NEG)
DECLARE_SIMD_OP_1ARG(float32, Float32, FLOAT32, Reciprocal, RECIPROCAL)
DECLARE_SIMD_OP_1ARG(float32, Float32, FLOAT32, ReciprocalSqrt, RECIPROCALSQRT)
DECLARE_SIMD_OP_1ARG(float32, Float32, FLOAT32, Sqrt, SQRT)
DECLARE_SIMD_OP_2ARGS(float32, Float32, FLOAT32, Add, ADD)
DECLARE_SIMD_OP_2ARGS(float32, Float32, FLOAT32, Sub, SUB)
DECLARE_SIMD_OP_2ARGS(float32, Float32, FLOAT32, Div, DIV)
DECLARE_SIMD_OP_2ARGS(float32, Float32, FLOAT32, Mul, FMUL)
DECLARE_SIMD_OP_2ARGS(float32, Float32, FLOAT32, Min, MIN)
DECLARE_SIMD_OP_2ARGS(float32, Float32, FLOAT32, Max, MAX)
DECLARE_SIMD_OP_COMP(float32, Float32, FLOAT32, LessThan, LESSTHAN)
DECLARE_SIMD_OP_COMP(float32, Float32, FLOAT32, LessThanOrEqual, LESSTHANEQ)
DECLARE_SIMD_OP_COMP(float32, Float32, FLOAT32, GreaterThan, GREATERTHAN)
DECLARE_SIMD_OP_COMP(float32, Float32, FLOAT32, GreaterThanOrEqual, GREATERTHANEQ)
DECLARE_SIMD_OP_COMP(float32, Float32, FLOAT32, Equal, EQ)
DECLARE_SIMD_OP_COMP(float32, Float32, FLOAT32, NotEqual, NOTEQ)
DECLARE_SIMD_OP_WITH(float32, Float32, FLOAT32, 0, WithX, WITH)
DECLARE_SIMD_OP_WITH(float32, Float32, FLOAT32, 1, WithY, WITH)
DECLARE_SIMD_OP_WITH(float32, Float32, FLOAT32, 2, WithZ, WITH)
DECLARE_SIMD_OP_WITH(float32, Float32, FLOAT32, 3, WithW, WITH)
DECLARE_SIMD_SHUFFLE(float32, Float32, FLOAT32)
DECLARE_SIMD_SHUFFLEMIX(float32, Float32, FLOAT32)
DECLARE_SIMD_CONVERT(float32, Float32, FLOAT32, ToInt32x4, int32)
DECLARE_SIMD_CONVERT_BITWISE(float32, Float32, FLOAT32, BitsToInt32x4, int32)

DECLARE_SIMD_SPLAT(int32, Int32, INT32, Splat)
DECLARE_SIMD_OP_1ARG(int32, Int32, INT32, Not, NOT)
DECLARE_SIMD_OP_1ARG(int32, Int32, INT32, Neg, NEG)
DECLARE_SIMD_OP_2ARGS(int32, Int32, INT32, Add, ADD)
DECLARE_SIMD_OP_2ARGS(int32, Int32, INT32, Sub, SUB)
DECLARE_SIMD_OP_2ARGS(int32, Int32, INT32, Mul, IMUL)
DECLARE_SIMD_OP_2ARGS(int32, Int32, INT32, Xor, XOR)
DECLARE_SIMD_OP_2ARGS(int32, Int32, INT32, And, AND)
DECLARE_SIMD_OP_2ARGS(int32, Int32, INT32, Or, OR)
DECLARE_SIMD_OP_WITH(int32, Int32, INT32, 0, WithX, WITH)
DECLARE_SIMD_OP_WITH(int32, Int32, INT32, 0, WithFlagX, WITHFLAG)
DECLARE_SIMD_OP_WITH(int32, Int32, INT32, 1, WithY, WITH)
DECLARE_SIMD_OP_WITH(int32, Int32, INT32, 1, WithFlagY, WITHFLAG)
DECLARE_SIMD_OP_WITH(int32, Int32, INT32, 2, WithZ, WITH)
DECLARE_SIMD_OP_WITH(int32, Int32, INT32, 2, WithFlagZ, WITHFLAG)
DECLARE_SIMD_OP_WITH(int32, Int32, INT32, 3, WithW, WITH)
DECLARE_SIMD_OP_WITH(int32, Int32, INT32, 3, WithFlagW, WITHFLAG)
DECLARE_SIMD_SHUFFLE(int32, Int32, INT32)
DECLARE_SIMD_SHUFFLEMIX(int32, Int32, INT32)
DECLARE_SIMD_CONVERT(int32, Int32, INT32, ToFloat32x4, float32)
DECLARE_SIMD_CONVERT_BITWISE(int32, Int32, INT32, BitsToFloat32x4, float32)
