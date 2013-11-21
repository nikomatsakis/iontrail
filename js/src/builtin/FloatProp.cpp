/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "builtin/FloatProp.h"
#include "builtin/Float32x4.h"

#include "jscompartment.h"
#include "jsfun.h"
#include "jsobj.h"
#include "jsutil.h"

#include "vm/GlobalObject.h"
#include "vm/StringBuffer.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"

using mozilla::DebugOnly;

using namespace js;

Class FloatProp::class_ = {
    "floatProp",
    JSCLASS_HAS_CACHED_PROTO(JSProto_floatProp),
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

static const JSFunctionSpec methods[] = {
    JS_FN("add",          FloatProp::add,    2, 0),
    JS_FN("mul",          FloatProp::mul,    2, 0),
    JS_FS_END
};

bool
FloatProp::add(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

   //Check Float32x4 arguments
    if (!Float32x4::is(args[0]) || !Float32x4::is(args[1])){
        return false;
    }

    JSObject *op1, *op2;
    op1 = &(args[0].toObject());
    op2 = &(args[1].toObject());

    RootedObject ret(cx, NewBuiltinClassInstance(cx, &Float32x4::class_));
    if (!ret)
        return false;

    for (int i = 0; i < 4; i++){
        double d1, d2;
        if (!ToNumber(cx,op1->getReservedSlot(i),&d1) || !ToNumber(cx,op2->getReservedSlot(i),&d2)){
            return false;
        }
        ret->setReservedSlot(i, DoubleValue(float(d1+d2)));
    }

    args.rval().setObject(*ret);
    return true;
}

bool
FloatProp::mul(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    //Check Float32x4 arguments
    if (!Float32x4::is(args[0]) || !Float32x4::is(args[1])){
        return false;
    }

    JSObject *op1, *op2;
    op1 = &(args[0].toObject());
    op2 = &(args[1].toObject());

    RootedObject ret(cx, NewBuiltinClassInstance(cx, &Float32x4::class_));
    if (!ret)
        return false;

    for (int i = 0; i < 4; i++){
        double d1, d2;
        if(!ToNumber(cx,op1->getReservedSlot(i),&d1) || !ToNumber(cx,op2->getReservedSlot(i),&d2)){
            return false;
        }
        ret->setReservedSlot(i, DoubleValue(float(d1*d2)));
    }

    args.rval().setObject(*ret);
    return true;
}

JSObject *
js_InitFloatProp(JSContext *cx, HandleObject obj)
{
		RootedObject proto(cx, NewObjectWithClassProto(cx, &FloatProp::class_, NULL, obj, SingletonObject));
		if (!proto)
			return NULL;

		if (!JS_DefineProperty(cx, obj, js_floatProp_str, OBJECT_TO_JSVAL(proto),
				JS_PropertyStub, JS_StrictPropertyStub, 0)) {
			return NULL;
		}

		if (!JS_DefineFunctions(cx, proto, methods))
			return NULL;

		MarkStandardClassInitializedNoProto(obj, &FloatProp::class_);

		return proto;
}
