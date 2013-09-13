/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef builtin_Uint32x4_h
#define builtin_Uint32x4_h

#include "jsapi.h"
#include "jsobj.h"

namespace js {
	class Uint32x4
	{
		public:
			static Class class_;

			static JSObject* initClass(JSContext *cx, Handle<GlobalObject *> global);

			static bool is(const Value &v);
			static bool construct(JSContext *cx, unsigned int argc, jsval *vp);

#define LANE_ACCESSOR(lane, name)\
			static bool get##lane(JSContext *cx, unsigned int argc, jsval *vp) {\
				return getLane(cx, argc, vp, SLOT_##lane, name);\
			}
			LANE_ACCESSOR(X, "x");
			LANE_ACCESSOR(Y, "y");
			LANE_ACCESSOR(Z, "z");
			LANE_ACCESSOR(W, "w");
#undef LANE_ACCESSOR

			static bool toString(JSContext *cx, unsigned int argc, jsval *vp);

		private:
			enum Uint32x4Slots {
				SLOT_X = 0,
				SLOT_Y,
				SLOT_Z,
				SLOT_W,

				SLOT_COUNT
			};

			static const JSPropertySpec properties[];

			static bool getLane(JSContext *cx, unsigned int argc, jsval *vp, unsigned int slot, const char *laneName);
	};

} // namespace js

#endif /* builtin_Uint32x4_h */
