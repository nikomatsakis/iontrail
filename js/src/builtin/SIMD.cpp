/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * JS SIMD package.
 */

#include "builtin/SIMD.h"
#include "builtin/Float32x4.h"
#include "jsapi.h"

#include "jsfriendapi.h"
#include "jsobjinlines.h"

#include <xmmintrin.h>

using namespace js;

using mozilla::ArrayLength;
using mozilla::IsFinite;
using mozilla::IsNaN;

Class SIMD::class_ = {
		"SIMD",
		JSCLASS_HAS_CACHED_PROTO(JSProto_SIMD),
		JS_PropertyStub,         /* addProperty */
		JS_DeletePropertyStub,   /* delProperty */
		JS_PropertyStub,         /* getProperty */
		JS_StrictPropertyStub,   /* setProperty */
		JS_EnumerateStub,
		JS_ResolveStub,
		JS_ConvertStub
};

static const JSFunctionSpec SIMD_static_methods[] = {
		JS_FN("addf",          SIMD::addf,             2, 0),
		JS_FN("mulf",          SIMD::mulf,             2, 0),
		JS_FN("addu",          SIMD::addu,             2, 0),
		JS_FN("mulu",          SIMD::mulu,             2, 0),
		JS_FS_END
};

bool
SIMD::addf(JSContext *cx, unsigned argc, Value *vp)
{
	CallArgs args = CallArgsFromVp(argc, vp);

   //Check Float32x4 arguments
	if(!Float32x4::is(args[0]) || !Float32x4::is(args[1])){
		return false;
	}
	//printf ("All arguments checked as Float32x4\n");

	JSObject *op1, *op2;
	op1 = &(args[0].toObject());
	op2 = &(args[1].toObject());

   RootedObject ret(cx, NewBuiltinClassInstance(cx, &Float32x4::class_));
	if(!ret)
		return false;

	for (int i=0; i< 4; i++){
		double d1, d2;
		if(!ToNumber(cx,op1->getReservedSlot(i),&d1) || !ToNumber(cx,op2->getReservedSlot(i),&d2)){
			return false;
		}
		ret->setReservedSlot(i, DoubleValue(float(d1+d2)));
	}

   args.rval().setObject(*ret);
	return true;
}

bool
SIMD::mulf(JSContext *cx, unsigned argc, Value *vp)
{
	CallArgs args = CallArgsFromVp(argc, vp);

   //Check Float32x4 arguments
	if(!Float32x4::is(args[0]) || !Float32x4::is(args[1])){
		return false;
	}
	//printf ("All arguments checked as Float32x4\n");

	JSObject *op1, *op2;
	op1 = &(args[0].toObject());
	op2 = &(args[1].toObject());

   RootedObject ret(cx, NewBuiltinClassInstance(cx, &Float32x4::class_));
	if(!ret)
		return false;

	for (int i=0; i< 4; i++){
		double d1, d2;
		if(!ToNumber(cx,op1->getReservedSlot(i),&d1) || !ToNumber(cx,op2->getReservedSlot(i),&d2)){
			return false;
		}
		ret->setReservedSlot(i, DoubleValue(float(d1*d2)));
	}

   args.rval().setObject(*ret);
	return true;
}

SIMD::addu(JSContext *cx, unsigned argc, Value *vp)
{
	CallArgs args = CallArgsFromVp(argc, vp);

   //Check Uint32x4 arguments
	if(!Uint32x4::is(args[0]) || !Uint32x4::is(args[1])){
		return false;
	}
	//printf ("All arguments checked as Uint32x4\n");

	JSObject *op1, *op2;
	op1 = &(args[0].toObject());
	op2 = &(args[1].toObject());

   RootedObject ret(cx, NewBuiltinClassInstance(cx, &Uint32x4::class_));
	if(!ret)
		return false;

	for (int i=0; i< 4; i++){
		double d1, d2;
		if(!ToNumber(cx,op1->getReservedSlot(i),&d1) || !ToNumber(cx,op2->getReservedSlot(i),&d2)){
			return false;
		}
		ret->setReservedSlot(i, DoubleValue(d1+d2));
	}

   args.rval().setObject(*ret);
	return true;
}

bool
SIMD::mulu(JSContext *cx, unsigned argc, Value *vp)
{
	CallArgs args = CallArgsFromVp(argc, vp);

   //Check Float32x4 arguments
	if(!Uint32x4::is(args[0]) || !Uint32x4::is(args[1])){
		return false;
	}
	//printf ("All arguments checked as Uint32x4\n");

	JSObject *op1, *op2;
	op1 = &(args[0].toObject());
	op2 = &(args[1].toObject());

   RootedObject ret(cx, NewBuiltinClassInstance(cx, &Uintt32x4::class_));
	if(!ret)
		return false;

	for (int i=0; i< 4; i++){
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
js_InitSIMD(JSContext *cx, HandleObject obj)
{
	RootedObject SIMD(cx, NewObjectWithClassProto(cx, &SIMD::class_, NULL, obj, SingletonObject));
	if (!SIMD)
		return NULL;

	if (!JS_DefineProperty(cx, obj, js_SIMD_str, OBJECT_TO_JSVAL(SIMD),
			JS_PropertyStub, JS_StrictPropertyStub, 0)) {
		return NULL;
	}

	if (!JS_DefineFunctions(cx, SIMD, SIMD_static_methods))
		return NULL;

	MarkStandardClassInitializedNoProto(obj, &SIMD::class_);

	return SIMD;
}
