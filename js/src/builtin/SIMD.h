/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SIMD_h
#define SIMD_h

#include "mozilla/MemoryReporting.h"

#include "jsapi.h"
#include "jsobj.h"

/*
 * JS SIMD functions.
 */

namespace js {
	class SIMD
	{
		public:
			static Class class_;

			static bool addf(JSContext *cx, unsigned argc, Value *vp);

			static bool mulf(JSContext *cx, unsigned argc, Value *vp);

			static bool addu(JSContext *cx, unsigned argc, Value *vp);

			static bool mulu(JSContext *cx, unsigned argc, Value *vp);

	};

}  /* namespace js */

#endif /* SIMD_h */
