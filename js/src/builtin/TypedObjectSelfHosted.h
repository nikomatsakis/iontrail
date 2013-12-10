/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


// Getters and setters for various slots for use in TypedObject self-hosted
// code.

// Type object slots

#define TYPE_TYPE_REPR(obj) \
    UnsafeGetReservedSlot(obj, JS_TYPEOBJ_SLOT_TYPE_REPR)

// Typed object slots

#define DATUM_TYPE_OBJ(obj) \
    UnsafeGetReservedSlot(obj, JS_DATUM_SLOT_TYPE_OBJ)
#define DATUM_OWNER(obj) \
    UnsafeGetReservedSlot(obj, JS_DATUM_SLOT_OWNER)
#define DATUM_LENGTH(obj) \
	 TO_INT32(UnsafeGetReservedSlot(obj, JS_DATUM_SLOT_LENGTH))

// Type repr slots

#define REPR_KIND(obj)   \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEREPR_SLOT_KIND))
#define REPR_SIZE(obj)   \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEREPR_SLOT_SIZE))
#define REPR_ALIGNMENT(obj) \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEREPR_SLOT_ALIGNMENT))
#define REPR_LENGTH(obj)   \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEREPR_SLOT_LENGTH))
#define REPR_TYPE(obj)   \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEREPR_SLOT_TYPE))

#define HAS_PROPERTY(obj, prop) \
    callFunction(std_Object_hasOwnProperty, obj, prop)

// Handy shortcut

#define DATUM_TYPE_REPR(obj) \
	 TYPE_TYPE_REPR(DATUM_TYPE_OBJ(obj))

#ifdef ENABLE_BINARYDATA
#define ABS(a) std_Math_abs(a)
#define NEG(a) (-1 * a)
#define RECIPROCAL(a) (1 / (a))
#define RECIPROCALSQRT(a) (1 / std_Math_sqrt(a))
#define SQRT(a) std_Math_sqrt(a)
#define ADD(a,b) ((a) + (b))
#define SUB(a,b) ((a) - (b))
#define DIV(a,b) ((a) / (b))
#define FMUL(a,b) ((a) * (b))
#define MIN(a,b) std_Math_min(a, b)
#define MAX(a,b) std_Math_max(a, b)
#define LESSTHAN(a,b) ((a) < (b) ? -1 : 0x0)
#define LESSTHANEQ(a,b) ((a) <= (b) ? -1 : 0x0)
#define GREATERTHAN(a,b) ((a) > (b) ? -1 : 0x0)
#define GREATERTHANEQ(a,b) ((a) >= (b) ? -1 : 0x0)
#define EQ(a,b) ((a) == (b) ? -1 : 0x0)
#define NOTEQ(a,b) ((a) != (b) ? -1 : 0x0)
#define XOR(a,b) ((a) ^ (b))
#define AND(a,b) ((a) & (b))
#define OR(a,b) ((a) | (b))
#define NOT(a) (~a)
#define IMUL(a,b) std_Math_imul(a, b)
#define WITH(b) (b)
#define WITHFLAG(b) (b ? -1 : 0x0)
#endif
