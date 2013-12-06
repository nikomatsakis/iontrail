/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SIMD_h
#define SIMD_h

#include "mozilla/MemoryReporting.h"
#include "builtin/TypedObject.h"

#include "jsapi.h"
#include "jsobj.h"

/*
 * JS SIMD functions.
 */

namespace js {

class SIMDObject : public JSObject
{
  public:
    static const Class class_;

    static JSObject* initClass(JSContext *cx, Handle<GlobalObject *> global);

    static bool toString(JSContext *cx, unsigned int argc, jsval *vp);
};

}  /* namespace js */

extern JSObject *
js_InitSIMD(JSContext *cx, js::HandleObject obj);

#endif /* SIMD_h */
