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

#define SIMD_FUNCTION_LIST(V)                                                  \
    V(Float32x4, add, float, 2, 0)

namespace js {

class TypedObject;

class SIMDObject : public JSObject
{
  public:
    static const Class class_;
    static JSObject* initClass(JSContext *cx, Handle<GlobalObject *> global);
    static bool toString(JSContext *cx, unsigned int argc, jsval *vp);
};

#define DECLARE_SIMD_FUNCTION(X4Type, OpCode, ElementType, Operands, Flags)    \
extern bool                                                                    \
js_simd_##X4Type##_##OpCode(JSContext *cx, unsigned argc, Value *vp);
SIMD_FUNCTION_LIST(DECLARE_SIMD_FUNCTION)
#undef DECLARE_SIMD_FUNCTION

// These classes exist for use with templates below.

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
        return obj.getInt32x4TypeObject();
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
