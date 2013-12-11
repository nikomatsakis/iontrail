/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * JS SIMD package.
 * Specification matches polyfill:
 * https://github.com/johnmccutchan/ecmascript_simd/blob/master/src/ecmascript_simd_tests.js
 * The objects (float32x4, int32x4) are implemented in TypedObjects.cpp 
 * The SIMD functions are declared here and implemented in self-hosted SIMD.js
 */

#include "builtin/SIMD.h"
#include "builtin/TypedObject.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsobjinlines.h"

using namespace js;

using mozilla::ArrayLength;
using mozilla::IsFinite;
using mozilla::IsNaN;

namespace js {
extern const JSFunctionSpec Float32x4Methods[];
extern const JSFunctionSpec Int32x4Methods[];
}

const Class SIMDObject::class_ = {
        "SIMD",
        JSCLASS_HAS_CACHED_PROTO(JSProto_SIMD),
        JS_PropertyStub,         /* addProperty */
        JS_DeletePropertyStub,   /* delProperty */
        JS_PropertyStub,         /* getProperty */
        JS_StrictPropertyStub,   /* setProperty */
        JS_EnumerateStub,
        JS_ResolveStub,
        JS_ConvertStub,
        nullptr,                 /* finalize    */
        nullptr,                 /* checkAccess */
        nullptr,                 /* call        */
        nullptr,                 /* hasInstance */
        nullptr,                 /* construct   */
        nullptr
};

JSObject *
SIMDObject::initClass(JSContext *cx, Handle<GlobalObject *> global)
{
    // Ensure that the TypedObject class is resolved. This is
    // convenient because it allows us to assume that whenever a SIMD
    // function runs, the `float32x4` and `int32x4` constructors have
    // been created.
    if (!global->getOrCreateTypedObjectModule(cx))
        return nullptr;

    // Create SIMD Object.
    RootedObject SIMD(cx, NewObjectWithGivenProto(cx, &SIMDObject::class_, nullptr,
                                                  global, SingletonObject));
    if (!SIMD)
        return nullptr;

    // Create Float32x4 and Int32x4 object.
    RootedObject objProto(cx, global->getOrCreateObjectPrototype(cx));
    if (!objProto)
        return nullptr;
    RootedObject float32x4(cx, NewObjectWithGivenProto(cx, &JSObject::class_, objProto,
                           global, SingletonObject));
    RootedObject int32x4(cx, NewObjectWithGivenProto(cx, &JSObject::class_, objProto,
                         global, SingletonObject));
    if(!float32x4 || !int32x4)
        return nullptr;

    // Define Float32x4 & Int32x4 functions.
    if (!JS_DefineFunctions(cx, int32x4, Int32x4Methods) ||
        !JS_DefineFunctions(cx, float32x4, Float32x4Methods))
    {
        return nullptr;
    }

    // Install Float32x4 & Int32x4 object as a properties of SIMD object.
    if (!JS_DefineProperty(cx, SIMD, "float32x4", OBJECT_TO_JSVAL(float32x4),
                           JS_PropertyStub, JS_StrictPropertyStub, 0) ||
        !JS_DefineProperty(cx, SIMD, "int32x4", OBJECT_TO_JSVAL(int32x4),
                           JS_PropertyStub, JS_StrictPropertyStub, 0))
    {
        return nullptr;
    }

    // Everything is setup, install SIMD on the global object.
    RootedValue SIMDValue(cx, ObjectValue(*SIMD));
    global->setConstructor(JSProto_SIMD, SIMDValue);
    if (!JSObject::defineProperty(cx, global, cx->names().SIMD,  SIMDValue,
                                  nullptr, nullptr, 0))
    {
        return nullptr;
    }

    return SIMD;
}

JS_FRIEND_API(JSObject *)
js_InitSIMDClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->is<GlobalObject>());
    Rooted<GlobalObject *> global(cx, &obj->as<GlobalObject>());
    return SIMDObject::initClass(cx, global);
}

namespace js {
struct Float32x4 {
    typedef float Elem;
    static const int32_t lanes = 4;
    static const X4TypeRepresentation::Type type =
        X4TypeRepresentation::TYPE_FLOAT32;

    static JSObject &GetTypeObject(GlobalObject &obj) {
        return obj.getFloat32x4TypeObject();
    }
    static Elem toType(Elem a) { 
        return a;
    }
};
} // namespace js

namespace js {
struct Int32x4 {
    typedef int32_t Elem;
    static const int32_t lanes = 4;
    static const X4TypeRepresentation::Type type =
        X4TypeRepresentation::TYPE_INT32;
    static JSObject &GetTypeObject(GlobalObject &obj) {
        return obj.getInt32x4TypeObject();
    }
    static Elem toType(Elem a) { 
        return ToInt32(a);
    }
};
} // namespace js

template<typename V>
static bool
ObjectIsVector(JSObject &obj) {
    if (!IsTypedDatum(obj))
        return false;
    TypeRepresentation *typeRepr = AsTypedDatum(obj).datumTypeRepresentation();
    if (typeRepr->kind() != TypeRepresentation::X4)
        return false;
    return typeRepr->asX4()->type() == V::type;
}

template<typename V>
static JSObject *
Create(JSContext *cx, typename V::Elem *data)
{
    RootedObject typeObj(cx, &V::GetTypeObject(*cx->global()));
    JS_ASSERT(typeObj);

    Rooted<TypedObject *> result(cx, TypedObject::createZeroed(cx, typeObj, 0));
    if (!result)
        return nullptr;

    typename V::Elem *resultMem = (typename V::Elem *) result->typedMem();
    memcpy(resultMem, data, sizeof(typename V::Elem) * V::lanes);
    return result;
}

namespace js {
template<typename T, typename V>
struct Abs {
    static inline T apply(T x, T zero) {return V::toType(x < 0 ? (-1*x) : x);}
};
template<typename T, typename V>
struct Neg {
    static inline T apply(T x, T zero) {return V::toType(-1 * x);}
};
template<typename T, typename V>
struct Not {
    static inline T apply(T x, T zero) {return V::toType(~x);}
};
template<typename T, typename V>
struct Rec {
    static inline T apply(T x, T zero) {return V::toType(1 / x);}
};
template<typename T, typename V>
struct RecSqrt {
    static inline T apply(T x, T zero) {return V::toType(1 / sqrt(x));}
};
template<typename T, typename V>
struct Sqrt {
    static inline T apply(T x, T zero) {return V::toType(sqrt(x));}
};
template<typename T, typename V>
struct Add {
    static inline T apply(T l, T r) {return V::toType(l + r);}
};
template<typename T, typename V>
struct Sub {
    static inline T apply(T l, T r) {return V::toType(l - r);}
};
template<typename T, typename V>
struct Div {
    static inline T apply(T l, T r) {return V::toType(l / r);}
};
template<typename T, typename V>
struct Mul {
    static inline T apply(T l, T r) {return V::toType(l * r);}
};
template<typename T, typename V>
struct Minimum {
    static inline T apply(T l, T r) {return V::toType(l < r ? l : r);}
};
template<typename T, typename V>
struct Maximum {
    static inline T apply(T l, T r) {return V::toType(l > r ? l : r);}
};
template<typename T, typename V>
struct LessThan {
    static inline T apply(T l, T r) {return V::toType(l < r ? 0xFFFFFFFF: 0x0);}
};
template<typename T, typename V>
struct LessThanOrEqual {
    static inline T apply(T l, T r) {return V::toType(l <= r ? 0xFFFFFFFF: 0x0);}
};
template<typename T, typename V>
struct GreaterThan {
    static inline T apply(T l, T r) {return V::toType(l > r ? 0xFFFFFFFF: 0x0);}
};
template<typename T, typename V>
struct GreaterThanOrEqual {
    static inline T apply(T l, T r) {return V::toType(l >= r ? 0xFFFFFFFF: 0x0);}
};
template<typename T, typename V>
struct Equal {
    static inline T apply(T l, T r) {return V::toType(l == r ? 0xFFFFFFFF: 0x0);}
};
template<typename T, typename V>
struct NotEqual {
    static inline T apply(T l, T r) {return V::toType(l != r ? 0xFFFFFFFF: 0x0);}
};
template<typename T, typename V>
struct Xor {
    static inline T apply(T l, T r) {return V::toType(l ^ r);}
};
template<typename T, typename V>
struct And {
    static inline T apply(T l, T r) {return V::toType(l & r);}
};
template<typename T, typename V>
struct Or {
    static inline T apply(T l, T r) {return V::toType(l | r);}
};
template<typename T, typename V>
struct WithX {
    static inline T apply(T lane, T scalar, T x) {return V::toType(lane == 0 ? scalar : x);}
};
template<typename T, typename V>
struct WithY {
    static inline T apply(T lane, T scalar, T x) {return V::toType(lane == 1 ? scalar : x);}
};
template<typename T, typename V>
struct WithZ {
    static inline T apply(T lane, T scalar, T x) {return V::toType(lane == 2 ? scalar : x);}
};
template<typename T, typename V>
struct WithW {
    static inline T apply(T lane, T scalar, T x) {return V::toType(lane == 3 ? scalar : x);}
};
template<typename T, typename V>
struct WithFlagX {
    static inline T apply(T l, T f, T x) { return V::toType(l == 0 ? (f ? 0xFFFFFFFF : 0x0) : x);}
};
template<typename T, typename V>
struct WithFlagY {
    static inline T apply(T l, T f, T x) { return V::toType(l == 1 ? (f ? 0xFFFFFFFF : 0x0) : x);}
};
template<typename T, typename V>
struct WithFlagZ {
    static inline T apply(T l, T f, T x) { return V::toType(l == 2 ? (f ? 0xFFFFFFFF : 0x0) : x);}
};
template<typename T, typename V>
struct WithFlagW {
    static inline T apply(T l, T f, T x) { return V::toType(l == 3 ? (f ? 0xFFFFFFFF : 0x0) : x);}
};
}

template<typename V, typename Op, typename Vret>
static bool
Oper(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (argc == 1) {
        if((!args[0].isObject() || !ObjectIsVector<V>(args[0].toObject()))) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }
        typename V::Elem *val = (typename V::Elem*) AsTypedDatum(args[0].toObject()).typedMem();
        typename Vret::Elem result[Vret::lanes];
        for (int32_t i = 0; i < Vret::lanes; i++)
            result[i] = Op::apply(val[i], 0);

        RootedObject obj(cx, Create<Vret>(cx, result));
        if (!obj)
            return false;

        args.rval().setObject(*obj);
        return true;

    } else if (argc == 2) {
        if((!args[0].isObject() || !ObjectIsVector<V>(args[0].toObject())) ||
           (!args[1].isObject() || !ObjectIsVector<V>(args[1].toObject())))
        {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }

        typename V::Elem *left = (typename V::Elem*) AsTypedDatum(args[0].toObject()).typedMem();
        typename V::Elem *right = (typename V::Elem*) AsTypedDatum(args[1].toObject()).typedMem();

        typename Vret::Elem result[Vret::lanes];
        for (int32_t i = 0; i < Vret::lanes; i++)
            result[i] = Op::apply(left[i], right[i]);

        RootedObject obj(cx, Create<Vret>(cx, result));
        if (!obj)
            return false;

        args.rval().setObject(*obj);
        return true;
    } else if (argc == 3) {

    }

    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,JSMSG_TYPED_ARRAY_BAD_ARGS);
    return false;

}

template<typename V, typename OpWith, typename Vret>
static bool
OperWith(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if ((argc != 2) ||
        (!args[0].isObject() || !ObjectIsVector<V>(args[0].toObject())) ||
        (!args[1].isNumber() && !args[1].isBoolean()))
    {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
        return false;
    }

    typename V::Elem *val = (typename V::Elem*) AsTypedDatum(args[0].toObject()).typedMem();

    typename Vret::Elem result[Vret::lanes];
    for (int32_t i = 0; i < Vret::lanes; i++){
        if(args[1].isNumber())
            result[i] = OpWith::apply(i, args[1].toNumber(), val[i]);
        else if (args[1].isBoolean())
            result[i] = OpWith::apply(i, args[1].toBoolean(), val[i]);
    }
    RootedObject obj(cx, Create<Vret>(cx, result));
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

const JSFunctionSpec js::Float32x4Methods[] = {
        JS_FN("abs", (Oper< Float32x4, Abs< float, Float32x4 >, Float32x4 >), 1, 0),
        JS_FN("neg", (Oper< Float32x4, Neg< float, Float32x4 >, Float32x4 >), 1, 0),
        JS_FN("reciprocal", (Oper< Float32x4, Rec< float, Float32x4 >, Float32x4 >), 1, 0),
        JS_FN("reciprocalSqrt", (Oper< Float32x4, RecSqrt< float, Float32x4 >, Float32x4 >), 1, 0),
        JS_FN("sqrt", (Oper< Float32x4, Sqrt< float, Float32x4 >, Float32x4 >), 1, 0),
        JS_FN("add", (Oper< Float32x4, Add< float, Float32x4 >, Float32x4 >), 2, 0),
        JS_FN("sub", (Oper< Float32x4, Sub< float, Float32x4 >, Float32x4 >), 2, 0),
        JS_FN("div", (Oper< Float32x4, Div< float, Float32x4 >, Float32x4 >), 2, 0),
        JS_FN("mul", (Oper< Float32x4, Mul< float, Float32x4 >, Float32x4 >), 2, 0),
        JS_FN("max", (Oper< Float32x4, Maximum< float, Float32x4 >, Float32x4 >), 2, 0),
        JS_FN("min", (Oper< Float32x4, Minimum< float, Float32x4 >, Float32x4 >), 2, 0),
        JS_FN("lessThan", (Oper< Float32x4, LessThan< float, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("lessThanOrEqual", (Oper< Float32x4, LessThanOrEqual< float, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("greaterThan", (Oper< Float32x4, GreaterThan< float, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("greaterThanOrEqual", (Oper< Float32x4, GreaterThanOrEqual< float, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("equal", (Oper< Float32x4, Equal< float, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("notEqual", (Oper< Float32x4, NotEqual< float, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("withX", (OperWith< Float32x4, WithX< float, Float32x4 >, Float32x4 >), 2, 0),
        JS_FN("withY", (OperWith< Float32x4, WithY< float, Float32x4 >, Float32x4 >), 2, 0),
        JS_FN("withZ", (OperWith< Float32x4, WithZ< float, Float32x4 >, Float32x4 >), 2, 0),
        JS_FN("withW", (OperWith< Float32x4, WithW< float, Float32x4 >, Float32x4 >), 2, 0),
        JS_SELF_HOSTED_FN("shuffle", "Float32x4Shuffle", 2, 0),
        JS_SELF_HOSTED_FN("shuffleMix", "Float32x4ShuffleMix", 3, 0),
        JS_SELF_HOSTED_FN("clamp", "Float32x4Clamp", 3, 0),
        JS_SELF_HOSTED_FN("scale", "Float32x4Scale", 2, 0),
        JS_SELF_HOSTED_FN("toInt32x4", "Float32x4ToInt32x4", 1, 0),
        JS_SELF_HOSTED_FN("bitsToInt32x4", "Float32x4BitsToInt32x4", 1, 0),
        JS_FS_END
};

const JSFunctionSpec js::Int32x4Methods[] = {
        JS_FN("not", (Oper< Int32x4, Not< int32_t, Int32x4 >, Int32x4 >), 1, 0),
        JS_FN("neg", (Oper< Int32x4, Neg< int32_t, Int32x4 >, Int32x4 >), 1, 0),
        JS_FN("add", (Oper< Int32x4, Add< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("sub", (Oper< Int32x4, Sub< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("mul", (Oper< Int32x4, Mul< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("xor", (Oper< Int32x4, Xor< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("and", (Oper< Int32x4, And< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("or", (Oper< Int32x4, Or< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("withX", (OperWith< Int32x4, WithX< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("withY", (OperWith< Int32x4, WithY< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("withZ", (OperWith< Int32x4, WithZ< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("withW", (OperWith< Int32x4, WithW< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("withFlagX", (OperWith< Int32x4, WithFlagX< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("withFlagY", (OperWith< Int32x4, WithFlagY< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("withFlagZ", (OperWith< Int32x4, WithFlagZ< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_FN("withFlagW", (OperWith< Int32x4, WithFlagW< int32_t, Int32x4 >, Int32x4 >), 2, 0),
        JS_SELF_HOSTED_FN("shuffle", "Int32x4Shuffle", 2, 0),
        JS_SELF_HOSTED_FN("shuffleMix", "Int32x4ShuffleMix", 3, 0),
        JS_SELF_HOSTED_FN("toFloat32x4", "Int32x4ToFloat32x4", 1, 0),
        JS_SELF_HOSTED_FN("bitsToFloat32x4", "Int32x4BitsToFloat32x4", 1, 0),
        JS_SELF_HOSTED_FN("select", "Int32x4Select", 3, 0),
        JS_FS_END
};
