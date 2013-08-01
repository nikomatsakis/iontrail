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

enum TypeCommonSlots {
    SLOT_TYPE_REPR=0,
    TYPE_RESERVED_SLOTS
};

enum ArrayTypeCommonSlots {
    SLOT_ARRAY_ELEM_TYPE = TYPE_RESERVED_SLOTS,
    ARRAY_TYPE_RESERVED_SLOTS
};

enum StructTypeCommonSlots {
    SLOT_STRUCT_FIELD_TYPES = TYPE_RESERVED_SLOTS,
    STRUCT_TYPE_RESERVED_SLOTS
};

enum BlockCommonSlots {
    SLOT_DATATYPE = 0,
    SLOT_BLOCKREFOWNER,
    BLOCK_RESERVED_SLOTS
};

extern Class DataClass;

extern Class TypeClass;

template <typename T>
class NumericType
{
  private:
    static Class * typeToClass();
  public:
    static bool convert(JSContext *cx, HandleValue val, T* converted);
    static bool reify(JSContext *cx, void *mem, MutableHandleValue vp);
    static JSBool call(JSContext *cx, unsigned argc, Value *vp);
};

extern Class NumericTypeClasses[ScalarTypeRepresentation::NumTypes];

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

class BinaryBlock
{
  public:
    static bool isBlock(HandleObject val);
    static uint8_t *mem(HandleObject val);
};

/* This represents the 'a' and it's [[Prototype]] chain */
class BinaryArray : public BinaryBlock
{
  private:
    static JSObject *createEmpty(JSContext *cx, HandleObject type);

  public:
    static Class class_;

    static bool isArray(HandleObject val);

    // creates zeroed memory of size of type
    static JSObject *create(JSContext *cx, HandleObject type);

    // uses passed block as memory
    static JSObject *create(JSContext *cx, HandleObject type,
                            HandleObject owner, size_t offset);
    static JSBool construct(JSContext *cx, unsigned int argc, jsval *vp);

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

    static JSBool construct(JSContext *cx, unsigned int argc, jsval *vp);
    static JSBool toString(JSContext *cx, unsigned int argc, jsval *vp);

    static bool convertAndCopyTo(JSContext *cx,
                                 StructTypeRepresentation *typeRepr,
                                 HandleValue from, uint8_t *mem);
};

class BinaryStruct : public JSObject
{
  private:
    static JSObject *createEmpty(JSContext *cx, HandleObject type);
    static JSObject *create(JSContext *cx, HandleObject type);

  public:
    static Class class_;

    static bool isStruct(HandleObject val);

    static JSObject *create(JSContext *cx, HandleObject type,
                            HandleObject owner, size_t offset);
    static JSBool construct(JSContext *cx, unsigned int argc, jsval *vp);

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
