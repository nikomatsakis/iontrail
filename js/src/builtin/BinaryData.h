/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef builtin_BinaryData_h
#define builtin_BinaryData_h

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsobj.h"

#include "gc/Heap.h"

namespace js {
typedef float float32_t;
typedef double float64_t;

// Slots common to all type descriptors:
enum TypeCommonSlots {
    // Canonical type representation of this type (see TypeRepresentation.h).
    SLOT_TYPE_REPR=0,
    TYPE_RESERVED_SLOTS
};

// Slots for ArrayType type descriptors:
enum ArrayTypeCommonSlots {
    // Type descriptor for the element type.
    SLOT_ARRAY_ELEM_TYPE = TYPE_RESERVED_SLOTS,
    ARRAY_TYPE_RESERVED_SLOTS
};

// Slots for StructType type descriptors:
enum StructTypeCommonSlots {
    // JS array containing type descriptors for each field, in order.
    SLOT_STRUCT_FIELD_TYPES = TYPE_RESERVED_SLOTS,
    STRUCT_TYPE_RESERVED_SLOTS
};

// Slots for data blocks:
enum BlockCommonSlots {
    // Refers to the type descriptor with which this block is associated.
    SLOT_DATATYPE = 0,

    // If this value is NULL, then the block instance owns the
    // uint8_t* in its priate data. Otherwise, this field contains the
    // owner, and thus keeps the owner alive.
    SLOT_BLOCKREFOWNER,

    BLOCK_RESERVED_SLOTS
};

extern Class DataClass;

extern Class TypeClass;

template <ScalarTypeRepresentation::Type type, typename T>
class NumericType
{
  private:
    static Class * typeToClass();
  public:
    static bool convert(JSContext *cx, HandleValue val, T* converted);
    static bool reify(JSContext *cx, void *mem, MutableHandleValue vp);
    static JSBool call(JSContext *cx, unsigned argc, Value *vp);
};

extern Class NumericTypeClasses[ScalarTypeRepresentation::TYPE_MAX];

/* This represents the 'A' and it's [[Prototype]] chain
 * in:
 *   A = new ArrayType(Type, N);
 *   a = new A();
 */
class ArrayType : public JSObject
{
  private:
  public:
    static Class class_;

    static JSObject *create(JSContext *cx, HandleObject arrayTypeGlobal,
                            HandleObject elementType, uint32_t length);
    static JSBool construct(JSContext *cx, unsigned int argc, jsval *vp);
    static JSBool repeat(JSContext *cx, unsigned int argc, jsval *vp);

    static JSBool toString(JSContext *cx, unsigned int argc, jsval *vp);

    static JSObject *elementType(JSContext *cx, HandleObject obj);
};

/* Operations common to all memory block objects */
class BinaryBlock
{
  private:
    // Creates a binary data object of the given type and class, but with
    // a NULL memory pointer. Caller must use setPrivate() to set the
    // memory pointer properly.
    static JSObject *createNull(JSContext *cx, HandleObject type,
                                HandleValue owner);
  public:
    // Returns the offset within the object where the `void*` pointer
    // can be found.
    static int dataOffset();

    static bool isBlock(HandleObject val);
    static uint8_t *mem(HandleObject val);

    // creates zeroed memory of size of type
    static JSObject *createZeroed(JSContext *cx, HandleObject type);

    // creates a block which aliases the memory owned by `owner`
    // at the given offset
    static JSObject *createDerived(JSContext *cx, HandleObject type,
                                   HandleObject owner, size_t offset);

    // user-accessible constructor (`new TypeDescriptor(...)`)
    static JSBool construct(JSContext *cx, unsigned int argc, jsval *vp);

};

/* This represents the 'a' and it's [[Prototype]] chain */
class BinaryArray : public BinaryBlock
{
  private:
    static JSObject *createEmpty(JSContext *cx, HandleObject type,
                                 HandleValue owner);

  public:
    static Class class_;

    static bool isArray(HandleObject val);

    static void finalize(FreeOp *op, JSObject *obj);
    static void obj_trace(JSTracer *tracer, JSObject *obj);

    static JSBool subarray(JSContext *cx, unsigned int argc, jsval *vp);
    static JSBool fill(JSContext *cx, unsigned int argc, jsval *vp);

    static JSBool obj_lookupGeneric(JSContext *cx, HandleObject obj,
                                    HandleId id, MutableHandleObject objp,
                                    MutableHandleShape propp);

    static JSBool obj_lookupProperty(JSContext *cx, HandleObject obj,
                                     HandlePropertyName name,
                                     MutableHandleObject objp,
                                     MutableHandleShape propp);

    static JSBool obj_lookupElement(JSContext *cx, HandleObject obj,
                                    uint32_t index, MutableHandleObject objp,
                                    MutableHandleShape propp);

    static JSBool obj_lookupSpecial(JSContext *cx, HandleObject obj,
                                    HandleSpecialId sid,
                                    MutableHandleObject objp,
                                    MutableHandleShape propp);

    static JSBool obj_getGeneric(JSContext *cx, HandleObject obj,
                                 HandleObject receiver,
                                 HandleId id,
                                 MutableHandleValue vp);

    static JSBool obj_getProperty(JSContext *cx, HandleObject obj,
                                  HandleObject receiver,
                                  HandlePropertyName name,
                                  MutableHandleValue vp);

    static JSBool obj_getElement(JSContext *cx, HandleObject obj,
                                 HandleObject receiver,
                                 uint32_t index,
                                 MutableHandleValue vp);

    static JSBool obj_getElementIfPresent(JSContext *cx, HandleObject obj,
                                          HandleObject receiver,
                                          uint32_t index,
                                          MutableHandleValue vp,
                                          bool *present);

    static JSBool obj_getSpecial(JSContext *cx, HandleObject obj,
                                 HandleObject receiver,
                                 HandleSpecialId sid,
                                 MutableHandleValue vp);

    static JSBool obj_setGeneric(JSContext *cx, HandleObject obj,
                                 HandleId id, MutableHandleValue vp,
                                 JSBool strict);

    static JSBool obj_setProperty(JSContext *cx, HandleObject obj,
                                  HandlePropertyName name,
                                  MutableHandleValue vp,
                                  JSBool strict);

    static JSBool obj_setElement(JSContext *cx, HandleObject obj,
                                 uint32_t index, MutableHandleValue vp,
                                 JSBool strict);

    static JSBool obj_setSpecial(JSContext *cx, HandleObject obj,
                                 HandleSpecialId sid,
                                 MutableHandleValue vp,
                                 JSBool strict);

    static JSBool obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                           HandleId id, unsigned *attrsp);

    static JSBool obj_getPropertyAttributes(JSContext *cx, HandleObject obj,
                                            HandlePropertyName name,
                                            unsigned *attrsp);

    static JSBool obj_getElementAttributes(JSContext *cx, HandleObject obj,
                                           uint32_t index, unsigned *attrsp);

    static JSBool obj_getSpecialAttributes(JSContext *cx, HandleObject obj,
                                           HandleSpecialId sid,
                                           unsigned *attrsp);

    static JSBool obj_enumerate(JSContext *cx, HandleObject obj,
                                JSIterateOp enum_op,
                                MutableHandleValue statep,
                                MutableHandleId idp);

    static JSBool lengthGetter(JSContext *cx, unsigned int argc, jsval *vp);

};

class StructType : public JSObject
{
  private:
    static JSObject *create(JSContext *cx, HandleObject structTypeGlobal,
                            HandleObject fields);
    /**
     * Sets up structType slots based on calculated memory size
     * and alignment and stores fieldmap as well.
     */
    static bool layout(JSContext *cx, HandleObject structType,
                       HandleObject fields);

  public:
    static Class class_;

    static JSBool toString(JSContext *cx, unsigned int argc, jsval *vp);

    static bool convertAndCopyTo(JSContext *cx,
                                 StructTypeRepresentation *typeRepr,
                                 HandleValue from, uint8_t *mem);

    static JSBool construct(JSContext *cx, unsigned int argc, jsval *vp);
};

class BinaryStruct : public JSObject
{
  private:
    static JSObject *createEmpty(JSContext *cx, HandleObject type,
                                         HandleValue owner);
    static JSObject *create(JSContext *cx, HandleObject type);

  public:
    static Class class_;

    static bool isStruct(HandleObject val);

    static JSObject *create(JSContext *cx, HandleObject type,
                            HandleObject owner, size_t offset);

    static void finalize(js::FreeOp *op, JSObject *obj);
    static void obj_trace(JSTracer *tracer, JSObject *obj);

    static JSBool obj_lookupGeneric(JSContext *cx, HandleObject obj,
                                    HandleId id, MutableHandleObject objp,
                                    MutableHandleShape propp);

    static JSBool obj_lookupProperty(JSContext *cx, HandleObject obj,
                                     HandlePropertyName name,
                                     MutableHandleObject objp,
                                     MutableHandleShape propp);

    static JSBool obj_lookupElement(JSContext *cx, HandleObject obj,
                                    uint32_t index, MutableHandleObject objp,
                                    MutableHandleShape propp);

    static JSBool obj_lookupSpecial(JSContext *cx, HandleObject obj,
                                    HandleSpecialId sid,
                                    MutableHandleObject objp,
                                    MutableHandleShape propp);

    static JSBool obj_enumerate(JSContext *cx, HandleObject obj,
                                JSIterateOp enum_op,
                                MutableHandleValue statep,
                                MutableHandleId idp);

    static JSBool obj_getGeneric(JSContext *cx, HandleObject obj,
                                 HandleObject receiver, HandleId id,
                                 MutableHandleValue vp);

    static JSBool obj_getProperty(JSContext *cx, HandleObject obj,
                                  HandleObject receiver,
                                  HandlePropertyName name,
                                  MutableHandleValue vp);

    static JSBool obj_getSpecial(JSContext *cx, HandleObject obj,
                                 HandleObject receiver, HandleSpecialId sid,
                                 MutableHandleValue vp);

    static JSBool obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                 MutableHandleValue vp, JSBool strict);

    static JSBool obj_setProperty(JSContext *cx, HandleObject obj,
                                  HandlePropertyName name,
                                  MutableHandleValue vp,
                                  JSBool strict);

};

} // namespace js

#endif /* builtin_BinaryData_h */
