/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

Class Float32x4::class_ = {
    "float32x4",
    JSCLASS_HAS_RESERVED_SLOTS(SLOT_COUNT) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_float32x4),
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    NULL,
    NULL,
    NULL,
    NULL,
    Float32x4::construct,
    NULL
};

const JSPropertySpec Float32x4::properties[] = {
    JS_PSG("x", getX, 0),
    JS_PSG("y", getY, 0),
    JS_PSG("z", getZ, 0),
    JS_PSG("w", getW, 0),
    JS_PS_END
};

JSObject *
Float32x4::initClass(JSContext *cx, Handle<GlobalObject *> global)
{
    RootedObject proto(cx, global->createBlankPrototype(cx, &Float32x4::class_));
    if (!proto)
        return NULL;

    RootedFunction ctor(cx,
        global->createConstructor(cx, Float32x4::construct, cx->names().float32x4, 4));
    if (!ctor ||
        !LinkConstructorAndPrototype(cx, ctor, proto) ||
        !DefinePropertiesAndBrand(cx, proto, properties, NULL) ||
        !DefineConstructorAndPrototype(cx, global, JSProto_float32x4, ctor, proto)) {
        return NULL;
    }

    return proto;
}

bool
Float32x4::is(const Value &v)
{
    return v.isObject() && v.toObject().hasClass(&class_);
}

// Only used by Float32x4::construct
static bool
setSlot(JSContext *cx, HandleObject obj, int slot, HandleValue val)
{
    /*
     * The semantics of float32x4 is to store 4 float32 values.  However, when any value is read,
     * it is immediately coerced back to a JS number (i.e., float64).  Thus, float32x4(a,b,c,d).x
     * === double(float(ToNumber(a))).  Since the contents of float32x4 are not aliased, we don't
     * actually need to store a float32, we can store the coerced float64, which is good because
     * JS::Value only provides float64.
     */
    double d;
    if (!ToNumber(cx, val, &d)) {
        return false;
    }

    obj->setReservedSlot(slot, DoubleValue(float(d)));
    return true;
}

bool
Float32x4::construct(JSContext *cx, unsigned int argc, jsval *vp)
{
    if (JS_IsConstructing(cx, vp)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_NOT_CONSTRUCTOR, "float32x4");
        return false;
    }

    CallArgs args = CallArgsFromVp(argc, vp);

    if (argc != 0 && argc != 1 && argc != 4) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_FLOAT32X4_BAD_ARGS);
        return false;
    }

    RootedObject obj(cx, NewBuiltinClassInstance(cx, &Float32x4::class_));
    if (!obj)
        return false;

	 if(argc == 0 ){
	   double d = 0;
		obj->setReservedSlot(SLOT_X,DoubleValue(float(d))); 
		obj->setReservedSlot(SLOT_Y,DoubleValue(float(d))); 
		obj->setReservedSlot(SLOT_Z,DoubleValue(float(d))); 
		obj->setReservedSlot(SLOT_W,DoubleValue(float(d))); 
	 } else if (argc == 1){
		 if (!setSlot(cx, obj, SLOT_X, args[0]) ||
				 !setSlot(cx, obj, SLOT_Y, args[0]) ||
				 !setSlot(cx, obj, SLOT_Z, args[0]) ||
				 !setSlot(cx, obj, SLOT_W, args[0])) {
			 return false;
		 }
	 } else if (argc == 4){
		 if (!setSlot(cx, obj, SLOT_X, args[0]) ||
				 !setSlot(cx, obj, SLOT_Y, args[1]) ||
				 !setSlot(cx, obj, SLOT_Z, args[2]) ||
				 !setSlot(cx, obj, SLOT_W, args[3])) {
			 return false;
		 }
	 }

    args.rval().setObject(*obj);
    return true;
}

bool
Float32x4::getLane(JSContext *cx, unsigned int argc, jsval *vp, unsigned int slot, const char *laneName)
{
    JS_ASSERT(slot < SLOT_COUNT);
    CallArgs args = CallArgsFromVp(argc, vp);
    if (!Float32x4::is(args.thisv())) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             class_.name, laneName, InformalValueTypeName(args.thisv()));
        return false;
    }

    RootedObject obj(cx, args.thisv().toObjectOrNull());
    args.rval().set(obj->getReservedSlot(slot));
    return true;
}

JSObject *
js_InitFloat32x4(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->is<GlobalObject>());
    Rooted<GlobalObject *> global(cx, &obj->as<GlobalObject>());
    return Float32x4::initClass(cx, global);
}
