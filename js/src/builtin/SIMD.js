#include "TypedObjectConstants.h"
#include "TypedObjectSelfHosted.h"

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
  var T = StandardTypeObjectDescriptors(); 
  return T.float32x4(0.0,0.0,0.0,0.0); 
}

function Int32x4Bool(x,y,z,w) {
  var T = StandardTypeObjectDescriptors(); 
  return T.int32x4(x ? -1 : 0x0,
                     y ? -1 : 0x0,
                     z ? -1 : 0x0,
                     w ? -1 : 0x0);
  
}

function Float32x4Scale(arg0, s) {
  if (!IsVectorDatum(arg0, JS_X4TYPEREPR_FLOAT32)) {
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS);
  }
  var lane0 = Load_float32(arg0, 0<<2); 
  var lane1 = Load_float32(arg0, 1<<2); 
  var lane2 = Load_float32(arg0, 2<<2); 
  var lane3 = Load_float32(arg0, 3<<2); 
  var T = StandardTypeObjectDescriptors(); 
  return T.float32x4(lane0*s, lane1*s, lane2*s, lane3*s);
}

function Float32x4Clamp(arg0, lowerLimit, upperLimit) {
  if (!IsVectorDatum(arg0, JS_X4TYPEREPR_FLOAT32)) {
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS);
  }
  var argX = Load_float32(arg0, 0<<2); 
  var argY = Load_float32(arg0, 1<<2); 
  var argZ = Load_float32(arg0, 2<<2); 
  var argW = Load_float32(arg0, 3<<2); 
  var llX = Load_float32(lowerLimit, 0<<2); 
  var llY = Load_float32(lowerLimit, 1<<2); 
  var llZ = Load_float32(lowerLimit, 2<<2); 
  var llW = Load_float32(lowerLimit, 3<<2); 
  var ulX = Load_float32(upperLimit, 0<<2); 
  var ulY = Load_float32(upperLimit, 1<<2); 
  var ulZ = Load_float32(upperLimit, 2<<2); 
  var ulW = Load_float32(upperLimit, 3<<2); 
  var cx = argX < llX ? llX : argX;
  var cy = argY < llY ? llY : argY;
  var cz = argZ < llZ ? llZ : argZ;
  var cw = argW < llW ? llW : argW;
  cx = cx > ulX ? ulX : cx;
  cy = cy > ulY ? ulY : cy;
  cz = cz > ulZ ? ulZ : cz;
  cw = cw > ulW ? ulW : cw;
  var T = StandardTypeObjectDescriptors(); 
  return T.float32x4(cx,cy,cz,cw);
}

function Int32x4Select(t, trueValue, falseValue) {
   if (!IsVectorDatum(t, JS_X4TYPEREPR_INT32) || 
       !IsVectorDatum(trueValue, JS_X4TYPEREPR_FLOAT32)|| 
       !IsVectorDatum(falseValue, JS_X4TYPEREPR_FLOAT32)) {
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS);
  }
  var tv = Int32x4BitsToInt32x4(trueValue);
  var fv = Int32x4BitsToInt32x4(falseValue);
  var tr = Int32x4And(t,tv);
  var nt = Int32x4Not(t);
  var fr = Int32x4And(nt,fv);
  var otf = Int32x4Or(tr,fr);
  return Int32x4BitsToFloat32x4(otf);
}

#define DECLARE_SIMD_SPLAT(foo32, Foo32, FOO32, OPNAME) \
function Foo32##x4##OPNAME (arg0) { \
  var T = StandardTypeObjectDescriptors(); \
  return T.foo32##x4(arg0, arg0, arg0, arg0); \
}

#define DECLARE_SIMD_CONVERT(foo32, Foo32, FOO32, OPNAME, ret32) \
function Foo32##x4##OPNAME (arg0) { \
  if (!IsVectorDatum(arg0, JS_X4TYPEREPR_##FOO32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = Load_##foo32(arg0, 0<<2); \
  var lane1 = Load_##foo32(arg0, 1<<2); \
  var lane2 = Load_##foo32(arg0, 2<<2); \
  var lane3 = Load_##foo32(arg0, 3<<2); \
  var T = StandardTypeObjectDescriptors(); \
  return T.ret32##x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_CONVERT_BITWISE(foo32, Foo32, FOO32, OPNAME, ret32) \
function Foo32##x4##OPNAME (arg0) { \
  if (!IsVectorDatum(arg0, JS_X4TYPEREPR_##FOO32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = Load_##ret32(arg0, 0<<2); \
  var lane1 = Load_##ret32(arg0, 1<<2); \
  var lane2 = Load_##ret32(arg0, 2<<2); \
  var lane3 = Load_##ret32(arg0, 3<<2); \
  var T = StandardTypeObjectDescriptors(); \
  return T.ret32##x4(lane0, lane1, lane2, lane3); \
}


#define DECLARE_SIMD_OP_1ARG(foo32, Foo32, FOO32, OPNAME, OP) \
function Foo32##x4##OPNAME (arg0) { \
  if (!IsVectorDatum(arg0, JS_X4TYPEREPR_##FOO32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = OP(Load_##foo32(arg0, 0<<2)); \
  var lane1 = OP(Load_##foo32(arg0, 1<<2)); \
  var lane2 = OP(Load_##foo32(arg0, 2<<2)); \
  var lane3 = OP(Load_##foo32(arg0, 3<<2)); \
  var T = StandardTypeObjectDescriptors(); \
  return T.foo32##x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_OP_2ARGS(foo32, Foo32, FOO32, OPNAME, OP) \
function Foo32##x4##OPNAME (arg0, arg1) { \
  if (!IsVectorDatum(arg0, JS_X4TYPEREPR_##FOO32) || \
      !IsVectorDatum(arg1, JS_X4TYPEREPR_##FOO32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = OP(Load_##foo32(arg0, 0<<2), Load_##foo32(arg1, 0<<2)); \
  var lane1 = OP(Load_##foo32(arg0, 1<<2), Load_##foo32(arg1, 1<<2)); \
  var lane2 = OP(Load_##foo32(arg0, 2<<2), Load_##foo32(arg1, 2<<2)); \
  var lane3 = OP(Load_##foo32(arg0, 3<<2), Load_##foo32(arg1, 3<<2)); \
  var T = StandardTypeObjectDescriptors(); \
  return T.foo32##x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_OP_COMP(foo32, Foo32, FOO32, OPNAME, OP) \
function Foo32##x4##OPNAME (arg0, arg1) { \
  if (!IsVectorDatum(arg0, JS_X4TYPEREPR_##FOO32) || \
      !IsVectorDatum(arg1, JS_X4TYPEREPR_##FOO32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = OP(Load_##foo32(arg0, 0<<2), Load_##foo32(arg1, 0<<2)); \
  var lane1 = OP(Load_##foo32(arg0, 1<<2), Load_##foo32(arg1, 1<<2)); \
  var lane2 = OP(Load_##foo32(arg0, 2<<2), Load_##foo32(arg1, 2<<2)); \
  var lane3 = OP(Load_##foo32(arg0, 3<<2), Load_##foo32(arg1, 3<<2)); \
  var T = StandardTypeObjectDescriptors(); \
  return T.int32x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_OP_WITH(foo32, Foo32, FOO32, SLOT, NAME, OP) \
function Foo32##x4##NAME (arg0, arg1) { \
  if (!IsVectorDatum(arg0, JS_X4TYPEREPR_##FOO32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = Load_##foo32(arg0, 0<<2); \
  var lane1 = Load_##foo32(arg0, 1<<2); \
  var lane2 = Load_##foo32(arg0, 2<<2); \
  var lane3 = Load_##foo32(arg0, 3<<2); \
  lane##SLOT = OP(arg1); \
  var T = StandardTypeObjectDescriptors(); \
  return T.foo32##x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_SHUFFLE(foo32, Foo32, FOO32) \
function Foo32##x4Shuffle (arg0, mask) { \
  if (!IsVectorDatum(arg0, JS_X4TYPEREPR_##FOO32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = Load_##foo32(arg0, ((mask) & 0x3)<<2); \
  var lane1 = Load_##foo32(arg0, ((mask >> 2) & 0x3)<<2); \
  var lane2 = Load_##foo32(arg0, ((mask >> 4) & 0x3)<<2); \
  var lane3 = Load_##foo32(arg0, ((mask >> 6) & 0x3)<<2); \
  var T = StandardTypeObjectDescriptors(); \
  return T.foo32##x4(lane0, lane1, lane2, lane3); \
}

#define DECLARE_SIMD_SHUFFLEMIX(foo32, Foo32, FOO32) \
function Foo32##x4ShuffleMix (arg0, arg1, mask) { \
  if (!IsVectorDatum(arg0, JS_X4TYPEREPR_##FOO32) || \
      !IsVectorDatum(arg1, JS_X4TYPEREPR_##FOO32)) \
  { \
    ThrowError(JSMSG_TYPED_ARRAY_BAD_ARGS); \
  } \
 \
  var lane0 = Load_##foo32(arg0, ((mask) & 0x3)<<2); \
  var lane1 = Load_##foo32(arg0, ((mask >> 2) & 0x3)<<2); \
  var lane2 = Load_##foo32(arg1, ((mask >> 4) & 0x3)<<2); \
  var lane3 = Load_##foo32(arg1, ((mask >> 6) & 0x3)<<2); \
  var T = StandardTypeObjectDescriptors(); \
  return T.foo32##x4(lane0, lane1, lane2, lane3); \
}

#define ABS(a) std_Math_abs(a)
#define NEG(a) (-1 * a)
#define RECIPROCAL(a) (1 / (a))
#define RECIPROCALSQRT(a) (1 / std_Math_sqrt(a))
#define SQRT(a) std_Math_sqrt(a)


#define ADD(a,b) ((a) + (b))
#define SUB(a,b) ((a) - (b))
#define DIV(a,b) ((a) / (b))
#define FMUL(a,b) ((a) * (b))
#define MIN(a,b) std_Math_min(a, b)
#define MAX(a,b) std_Math_max(a, b)
#define LESSTHAN(a,b) ((a) < (b) ? -1 : 0x0)
#define LESSTHANEQ(a,b) ((a) <= (b) ? -1 : 0x0)
#define GREATERTHAN(a,b) ((a) > (b) ? -1 : 0x0)
#define GREATERTHANEQ(a,b) ((a) >= (b) ? -1 : 0x0)
#define EQ(a,b) ((a) == (b) ? -1 : 0x0)
#define NOTEQ(a,b) ((a) != (b) ? -1 : 0x0)

#define XOR(a,b) ((a) ^ (b))
#define AND(a,b) ((a) & (b))
#define OR(a,b) ((a) | (b))
#define NOT(a) (~a)
#define IMUL(a,b) std_Math_imul(a, b)
#define WITH(b) (b)
#define WITHFLAG(b) (b ? -1 : 0x0)

// We need to ensure that the operations have float32 semantics.  I
// believe they should, because the data originates from a float32
// source and the result of op is coerced back to float32 as part of
// the float32x4 constructor. However, we could insert calls to
// Math.fround to be certain.
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


// We need to ensure the operations have int32 semantics. Addition is
// fine but Math.imul is required for multiplication, I believe.
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
