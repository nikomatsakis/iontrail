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

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsobjinlines.h"

using namespace js;

using mozilla::ArrayLength;
using mozilla::IsFinite;
using mozilla::IsNaN;

const JSFunctionSpec Float32x4Methods[] = {
        JS_SELF_HOSTED_FN("abs", "Float32x4Abs", 1, 0),
        JS_SELF_HOSTED_FN("neg", "Float32x4Neg", 1, 0),
        JS_SELF_HOSTED_FN("reciprocal", "Float32x4Reciprocal", 1, 0),
        JS_SELF_HOSTED_FN("reciprocalSqrt", "Float32x4ReciprocalSqrt", 1, 0),
        JS_SELF_HOSTED_FN("sqrt", "Float32x4Sqrt", 1, 0),
        JS_SELF_HOSTED_FN("add", "Float32x4Add", 2, 0),
        JS_SELF_HOSTED_FN("sub", "Float32x4Sub", 2, 0),
        JS_SELF_HOSTED_FN("div", "Float32x4Div", 2, 0),
        JS_SELF_HOSTED_FN("mul", "Float32x4Mul", 2, 0),
        JS_SELF_HOSTED_FN("min", "Float32x4Min", 2, 0),
        JS_SELF_HOSTED_FN("max", "Float32x4Max", 2, 0),
        JS_SELF_HOSTED_FN("lessThan", "Float32x4LessThan", 2, 0),
        JS_SELF_HOSTED_FN("lessThanOrEqual", "Float32x4LessThanOrEqual", 2, 0),
        JS_SELF_HOSTED_FN("greaterThan", "Float32x4GreaterThan", 2, 0),
        JS_SELF_HOSTED_FN("greaterThanOrEqual", "Float32x4GreaterThanOrEqual", 2, 0),
        JS_SELF_HOSTED_FN("equal", "Float32x4Equal", 2, 0),
        JS_SELF_HOSTED_FN("notEqual", "Float32x4NotEqual", 2, 0),
        JS_SELF_HOSTED_FN("withX", "Float32x4WithX", 2, 0),
        JS_SELF_HOSTED_FN("withY", "Float32x4WithY", 2, 0),
        JS_SELF_HOSTED_FN("withZ", "Float32x4WithZ", 2, 0),
        JS_SELF_HOSTED_FN("withW", "Float32x4WithW", 2, 0),
        JS_SELF_HOSTED_FN("shuffle", "Float32x4Shuffle", 2, 0),
        JS_SELF_HOSTED_FN("shuffleMix", "Float32x4ShuffleMix", 3, 0),
        JS_SELF_HOSTED_FN("clamp", "Float32x4Clamp", 3, 0),
        JS_SELF_HOSTED_FN("scale", "Float32x4Scale", 2, 0),
        JS_SELF_HOSTED_FN("toInt32x4", "Float32x4ToInt32x4", 1, 0),
        JS_SELF_HOSTED_FN("bitsToInt32x4", "Float32x4BitsToInt32x4", 1, 0),
        JS_FS_END
};

const JSFunctionSpec Int32x4Methods[] = {
        JS_SELF_HOSTED_FN("not", "Int32x4Not", 1, 0),
        JS_SELF_HOSTED_FN("neg", "Int32x4Neg", 1, 0),
        JS_SELF_HOSTED_FN("add", "Int32x4Add", 2, 0),
        JS_SELF_HOSTED_FN("sub", "Int32x4Sub", 2, 0),
        JS_SELF_HOSTED_FN("mul", "Int32x4Mul", 2, 0),
        JS_SELF_HOSTED_FN("xor", "Int32x4Xor", 2, 0),
        JS_SELF_HOSTED_FN("and", "Int32x4And", 2, 0),
        JS_SELF_HOSTED_FN("or", "Int32x4Or", 2, 0),
        JS_SELF_HOSTED_FN("withX", "Int32x4WithX", 2, 0),
        JS_SELF_HOSTED_FN("withFlagX", "Int32x4WithFlagX", 2, 0),
        JS_SELF_HOSTED_FN("withY", "Int32x4WithY", 2, 0),
        JS_SELF_HOSTED_FN("withFlagY", "Int32x4WithFlagY", 2, 0),
        JS_SELF_HOSTED_FN("withZ", "Int32x4WithZ", 2, 0),
        JS_SELF_HOSTED_FN("withFlagZ", "Int32x4WithFlagZ", 2, 0),
        JS_SELF_HOSTED_FN("withW", "Int32x4WithW", 2, 0),
        JS_SELF_HOSTED_FN("withFlagW", "Int32x4WithFlagW", 2, 0),
        JS_SELF_HOSTED_FN("shuffle", "Int32x4Shuffle", 2, 0),
        JS_SELF_HOSTED_FN("shuffleMix", "Int32x4ShuffleMix", 3, 0),
        JS_SELF_HOSTED_FN("toFloat32x4", "Int32x4ToFloat32x4", 1, 0),
        JS_SELF_HOSTED_FN("bitsToFloat32x4", "Int32x4BitsToFloat32x4", 1, 0),
        JS_SELF_HOSTED_FN("select", "Int32x4Select", 3, 0),
        JS_FS_END
};

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
