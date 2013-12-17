/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef builtin_SIMD_h
#define builtin_SIMD_h

#include "mozilla/MemoryReporting.h"

#include "jsapi.h"
#include "jsobj.h"

/*
 * JS SIMD functions.
 * Spec matching polyfill:
 * https://github.com/johnmccutchan/ecmascript_simd/blob/master/src/ecmascript_simd_tests.js
 */

#define FLOAT32X4_FUNCTION_LIST(V)                                                                        \
   V(Float32x4, add, (Func<Float32x4, Add<float, Float32x4>, Float32x4>), 2, 0)                           \
   V(Float32x4, abs, (Func<Float32x4, Abs<float, Float32x4>, Float32x4>), 1, 0)                           \
   V(Float32x4, neg, (Func<Float32x4, Neg<float, Float32x4>, Float32x4>), 1, 0)                           \
   V(Float32x4, reciprocal, (Func<Float32x4, Rec<float, Float32x4>, Float32x4>), 1, 0)                    \
   V(Float32x4, reciprocalSqrt, (Func<Float32x4, RecSqrt<float, Float32x4>, Float32x4>), 1, 0)            \
   V(Float32x4, sqrt, (Func<Float32x4, Sqrt<float, Float32x4>, Float32x4>), 1, 0)                         \
   V(Float32x4, sub, (Func<Float32x4, Sub<float, Float32x4>, Float32x4>), 2, 0)                           \
   V(Float32x4, div, (Func<Float32x4, Div<float, Float32x4>, Float32x4>), 2, 0)                           \
   V(Float32x4, mul, (Func<Float32x4, Mul<float, Float32x4>, Float32x4>), 2, 0)                           \
   V(Float32x4, max, (Func<Float32x4, Maximum<float, Float32x4>, Float32x4>), 2, 0)                       \
   V(Float32x4, min, (Func<Float32x4, Minimum<float, Float32x4>, Float32x4>), 2, 0)                       \
   V(Float32x4, lessThan, (Func<Float32x4, LessThan<float, Int32x4>, Int32x4>), 2, 0)                     \
   V(Float32x4, lessThanOrEqual, (Func<Float32x4, LessThanOrEqual<float, Int32x4>, Int32x4>), 2, 0)       \
   V(Float32x4, greaterThan, (Func<Float32x4, GreaterThan<float, Int32x4>, Int32x4>), 2, 0)               \
   V(Float32x4, greaterThanOrEqual, (Func<Float32x4, GreaterThanOrEqual<float, Int32x4>, Int32x4>), 2, 0) \
   V(Float32x4, equal, (Func<Float32x4, Equal<float, Int32x4>, Int32x4>), 2, 0)                           \
   V(Float32x4, notEqual, (Func<Float32x4, NotEqual<float, Int32x4>, Int32x4>), 2, 0)                     \
   V(Float32x4, withX, (FuncWith<Float32x4, WithX<float, Float32x4>, Float32x4>), 2, 0)                   \
   V(Float32x4, withY, (FuncWith<Float32x4, WithY<float, Float32x4>, Float32x4>), 2, 0)                   \
   V(Float32x4, withZ, (FuncWith<Float32x4, WithZ<float, Float32x4>, Float32x4>), 2, 0)                   \
   V(Float32x4, withW, (FuncWith<Float32x4, WithW<float, Float32x4>, Float32x4>), 2, 0)                   \
   V(Float32x4, shuffle, (FuncShuffle<Float32x4, Shuffle<float, Float32x4>, Float32x4>), 2, 0)            \
   V(Float32x4, shuffleMix, (FuncShuffle<Float32x4, Shuffle<float, Float32x4>, Float32x4>), 3, 0)         \
   V(Float32x4, scale, (FuncWith<Float32x4, Scale<float, Float32x4>, Float32x4>), 2, 0)                   \
   V(Float32x4, clamp, Float32x4Clamp, 3, 0)                                                              \
   V(Float32x4, toInt32x4, (FuncConvert<Float32x4, Int32x4>), 1, 0)                                       \
   V(Float32x4, bitsToInt32x4, (FuncConvertBits<Float32x4, Int32x4>), 1, 0)                               \
   V(Float32x4, zero, (FuncZero<Float32x4>), 0, 0)                                                        \
   V(Float32x4, splat, (FuncSplat<Float32x4>), 0, 0)                                                     

#define INT32X4_FUNCTION_LIST(V)                                                                          \
   V(Int32x4, add, (Func<Int32x4, Add<int32_t, Int32x4>, Int32x4>), 2, 0)                                 \
   V(Int32x4, not, (Func<Int32x4, Not<int32_t, Int32x4>, Int32x4>), 1, 0)                                 \
   V(Int32x4, neg, (Func<Int32x4, Neg<int32_t, Int32x4>, Int32x4>), 1, 0)                                 \
   V(Int32x4, sub, (Func<Int32x4, Sub<int32_t, Int32x4>, Int32x4>), 2, 0)                                 \
   V(Int32x4, mul, (Func<Int32x4, Mul<int32_t, Int32x4>, Int32x4>), 2, 0)                                 \
   V(Int32x4, xor, (Func<Int32x4, Xor<int32_t, Int32x4>, Int32x4>), 2, 0)                                 \
   V(Int32x4, and, (Func<Int32x4, And<int32_t, Int32x4>, Int32x4>), 2, 0)                                 \
   V(Int32x4, or, (Func<Int32x4, Or<int32_t, Int32x4>, Int32x4>), 2, 0)                                   \
   V(Int32x4, withX, (FuncWith<Int32x4, WithX<int32_t, Int32x4>, Int32x4>), 2, 0)                         \
   V(Int32x4, withY, (FuncWith<Int32x4, WithY<int32_t, Int32x4>, Int32x4>), 2, 0)                         \
   V(Int32x4, withZ, (FuncWith<Int32x4, WithZ<int32_t, Int32x4>, Int32x4>), 2, 0)                         \
   V(Int32x4, withW, (FuncWith<Int32x4, WithW<int32_t, Int32x4>, Int32x4>), 2, 0)                         \
   V(Int32x4, withFlagX, (FuncWith<Int32x4, WithFlagX<int32_t, Int32x4>, Int32x4>), 2, 0)                 \
   V(Int32x4, withFlagY, (FuncWith<Int32x4, WithFlagY<int32_t, Int32x4>, Int32x4>), 2, 0)                 \
   V(Int32x4, withFlagZ, (FuncWith<Int32x4, WithFlagZ<int32_t, Int32x4>, Int32x4>), 2, 0)                 \
   V(Int32x4, withFlagW, (FuncWith<Int32x4, WithFlagW<int32_t, Int32x4>, Int32x4>), 2, 0)                 \
   V(Int32x4, shuffle, (FuncShuffle<Int32x4, Shuffle<int32_t, Int32x4>, Int32x4>), 2, 0)                  \
   V(Int32x4, shuffleMix, (FuncShuffle<Int32x4, Shuffle<int32_t, Int32x4>, Int32x4>), 3, 0)               \
   V(Int32x4, toFloat32x4, (FuncConvert<Int32x4, Float32x4>), 1, 0)                                       \
   V(Int32x4, bitsToFloat32x4, (FuncConvertBits<Int32x4, Float32x4>), 1, 0)                               \
   V(Int32x4, zero, (FuncZero<Int32x4>), 0, 0)                                                            \
   V(Int32x4, splat, (FuncSplat<Int32x4>), 0, 0)                                                          \
   V(Int32x4, select, Int32x4Select, 3, 0)                                                                \
   V(Int32x4, bool, Int32x4Bool, 4, 0)                                                                    

namespace js {

class TypedObject;

class SIMDObject : public JSObject
{
  public:
    static const Class class_;
    static JSObject* initClass(JSContext *cx, Handle<GlobalObject *> global);
    static bool toString(JSContext *cx, unsigned int argc, jsval *vp);
};

#define DECLARE_SIMD_FUNCTION(X4Type, funcName, func, Operands, Flags)    \
extern bool                                                                    \
js_simd_##X4Type##_##funcName(JSContext *cx, unsigned argc, Value *vp);
FLOAT32X4_FUNCTION_LIST(DECLARE_SIMD_FUNCTION)
INT32X4_FUNCTION_LIST(DECLARE_SIMD_FUNCTION)
#undef DECLARE_SIMD_FUNCTION

// These classes exist for use with templates below.

struct Float32x4 {
    typedef float Elem;
    static const int32_t lanes = 4;
    static const X4TypeRepresentation::Type type =
        X4TypeRepresentation::TYPE_FLOAT32;

    static JSObject &GetTypeObject(GlobalObject &obj) {
        return obj.float32x4TypeObject();
    }
    static Elem toType(Elem a) {
        return a;
    }
    static void setReturn(CallArgs &args, float value) {
        args.rval().setDouble(value);
    }

    static const JSFunctionSpec TypeDescriptorMethods[];
    static const JSPropertySpec TypeObjectProperties[];
    static const JSFunctionSpec TypeObjectMethods[];
};

struct Int32x4 {
    typedef int32_t Elem;
    static const int32_t lanes = 4;
    static const X4TypeRepresentation::Type type =
        X4TypeRepresentation::TYPE_INT32;
    static JSObject &GetTypeObject(GlobalObject &obj) {
        return obj.int32x4TypeObject();
    }
    static Elem toType(Elem a) {
        return ToInt32(a);
    }
    static void setReturn(CallArgs &args, int32_t value) {
        args.rval().setInt32(value);
    }
    static const JSFunctionSpec TypeDescriptorMethods[];
    static const JSPropertySpec TypeObjectProperties[];
    static const JSFunctionSpec TypeObjectMethods[];
};

template<typename V>
TypedObject *CreateZeroedSIMDWrapper(JSContext *cx);

}  /* namespace js */

extern JSObject *
js_InitSIMDClass(JSContext *cx, js::HandleObject obj);

#endif /* builtin_SIMD_h */
