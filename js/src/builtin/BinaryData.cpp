/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "builtin/BinaryData.h"

#include "mozilla/FloatingPoint.h"

#include <vector>

#include "jscompartment.h"
#include "jsfun.h"
#include "jsobj.h"
#include "jsutil.h"

#include "builtin/TypeRepresentation.h"
#include "gc/Marking.h"
#include "vm/GlobalObject.h"
#include "vm/String.h"
#include "vm/StringBuffer.h"
#include "vm/TypedArrayObject.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"

using namespace js;

/*
 * Reify() takes a complex binary data object `owner` and an offset and tries to
 * convert the value of type `type` at that offset to a JS Value stored in
 * `vp`.
 *
 * NOTE: `type` is NOT the type of `owner`, but the type of `owner.elementType` in
 * case of BinaryArray or `owner[field].type` in case of BinaryStruct.
 */
static bool Reify(JSContext *cx, TypeRepresentation *typeRepr, HandleObject type,
                  HandleObject owner, size_t offset, MutableHandleValue vp);

/*
 * ConvertAndCopyTo() converts `from` to type `type` and stores the result in
 * `mem`, which MUST be pre-allocated to the appropriate size for instances of
 * `type`.
 */
static bool ConvertAndCopyTo(JSContext *cx, TypeRepresentation *typeRepr,
                             HandleValue from, uint8_t *mem);

static JSBool
TypeThrowError(JSContext *cx, unsigned argc, Value *vp)
{
    return ReportIsNotFunction(cx, *vp);
}

static JSBool
DataThrowError(JSContext *cx, unsigned argc, Value *vp)
{
    return ReportIsNotFunction(cx, *vp);
}

static void
ReportTypeError(JSContext *cx, Value fromValue, const char *toType)
{
    char *valueStr = JS_EncodeString(cx, JS_ValueToString(cx, fromValue));
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_CONVERT_TO,
                         valueStr, toType);
    JS_free(cx, (void *) valueStr);
}

static void
ReportTypeError(JSContext *cx, Value fromValue, JSString *toType)
{
    const char *fnName = JS_EncodeString(cx, toType);
    ReportTypeError(cx, fromValue, fnName);
    JS_free(cx, (void *) fnName);
}

static void
ReportTypeError(JSContext *cx, Value fromValue, TypeRepresentation *toType)
{
    StringBuffer contents(cx);
    if (!toType->appendString(cx, contents))
        return;

    RootedString result(cx, contents.finishString());
    if (!result)
        return;

    ReportTypeError(cx, fromValue, result.get());
}

static int32_t
Clamp(int32_t value, int32_t min, int32_t max)
{
    JS_ASSERT(min < max);
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

static inline bool
IsNumericType(HandleObject type)
{
    return type &&
           &NumericTypeClasses[0] <= type->getClass() &&
           type->getClass() < &NumericTypeClasses[ScalarTypeRepresentation::NumTypes];
}

static inline bool
IsArrayType(HandleObject type)
{
    return type && type->hasClass(&ArrayType::class_);
}

static inline bool
IsStructType(HandleObject type)
{
    return type && type->hasClass(&StructType::class_);
}

static inline bool
IsComplexType(HandleObject type)
{
    return IsArrayType(type) || IsStructType(type);
}

static inline bool
IsBinaryType(HandleObject type)
{
    return IsNumericType(type) || IsComplexType(type);
}

static inline bool
IsBinaryArray(HandleObject type)
{
    return type && type->hasClass(&BinaryArray::class_);
}

static inline bool
IsBinaryStruct(HandleObject type)
{
    return type && type->hasClass(&BinaryStruct::class_);
}

static inline bool
IsBlock(HandleObject type)
{
    return type->hasClass(&BinaryArray::class_) ||
           type->hasClass(&BinaryStruct::class_);
}

static inline uint8_t *
BlockMem(HandleObject val)
{
    JS_ASSERT(IsBlock(val));
    return (uint8_t*) val->getPrivate();
}

/*
 * Given a user-visible type descriptor object, returns the
 * dummy object for the TypeRepresentation* that we use internally.
 */
static JSObject *
typeRepresentationObj(HandleObject typeObj)
{
    JS_ASSERT(IsBinaryType(typeObj));
    return typeObj->getFixedSlot(SLOT_TYPE_REPR).toObjectOrNull();
}

/*
 * Given a user-visible type descriptor object, returns the
 * TypeRepresentation* that we use internally.
 *
 * Note: this pointer is valid only so long as `typeObj` remains rooted.
 */
static TypeRepresentation *
typeRepresentation(JSContext *cx, HandleObject typeObj)
{
    RootedObject typeReprObj(cx, typeRepresentationObj(typeObj));
    return TypeRepresentation::of(typeReprObj);
}

static inline JSObject *
GetType(HandleObject block)
{
    JS_ASSERT(IsBlock(block));
    return block->getFixedSlot(SLOT_DATATYPE).toObjectOrNull();
}

/*
 * Overwrites the contents of `block` with the converted form of `val`
 */
static bool
ConvertAndCopyTo(JSContext *cx, HandleValue val, HandleObject block)
{
    uint8_t *memory = BlockMem(block);
    RootedObject type(cx, GetType(block));
    TypeRepresentation *typeRepr = typeRepresentation(cx, type);
    return ConvertAndCopyTo(cx, typeRepr, val, memory);
}

struct FieldInfo
{
    HeapId name;
    HeapPtrObject type;
    size_t offset;

    FieldInfo() : offset(0) {}

    FieldInfo(const FieldInfo &o)
        : name(o.name.get()), type(o.type), offset(o.offset)
    {
    }
};

Class js::DataClass = {
    "Data",
    JSCLASS_HAS_CACHED_PROTO(JSProto_Data),
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

Class js::TypeClass = {
    "Type",
    JSCLASS_HAS_CACHED_PROTO(JSProto_Type),
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

#define BINARYDATA_NUMERIC_CLASSES(constant_, type_)\
{\
    #type_,\
    JSCLASS_HAS_RESERVED_SLOTS(1) |\
    JSCLASS_HAS_CACHED_PROTO(JSProto_##type_),\
    JS_PropertyStub,       /* addProperty */\
    JS_DeletePropertyStub, /* delProperty */\
    JS_PropertyStub,       /* getProperty */\
    JS_StrictPropertyStub, /* setProperty */\
    JS_EnumerateStub,\
    JS_ResolveStub,\
    JS_ConvertStub,\
    NULL,\
    NULL,\
    NumericType<type_##_t>::call,\
    NULL,\
    NULL,\
    NULL\
},

Class js::NumericTypeClasses[ScalarTypeRepresentation::NumTypes] = {
    JS_FOR_EACH_SCALAR_TYPE_REPR(BINARYDATA_NUMERIC_CLASSES)
};

template <typename Domain, typename Input>
bool
InRange(Input x)
{
    return std::numeric_limits<Domain>::min() <= x &&
           x <= std::numeric_limits<Domain>::max();
}

template <>
bool
InRange<float, int>(int x)
{
    return -std::numeric_limits<float>::max() <= x &&
           x <= std::numeric_limits<float>::max();
}

template <>
bool
InRange<double, int>(int x)
{
    return -std::numeric_limits<double>::max() <= x &&
           x <= std::numeric_limits<double>::max();
}

template <>
bool
InRange<float, double>(double x)
{
    return -std::numeric_limits<float>::max() <= x &&
           x <= std::numeric_limits<float>::max();
}

template <>
bool
InRange<double, double>(double x)
{
    return -std::numeric_limits<double>::max() <= x &&
           x <= std::numeric_limits<double>::max();
}

template <typename T>
Class *
js::NumericType<T>::typeToClass()
{
    abort();
    return NULL;
}

#define BINARYDATA_TYPE_TO_CLASS(constant_, type_)\
    template <>\
    Class *\
    NumericType<type_##_t>::typeToClass()\
    {\
        return &NumericTypeClasses[constant_];\
    }

/**
 * This namespace declaration is required because of a weird 'specialization in
 * different namespace' error that happens in gcc, only on type specialized
 * template functions.
 */
namespace js {
    JS_FOR_EACH_SCALAR_TYPE_REPR(BINARYDATA_TYPE_TO_CLASS);
}

template <typename T>
JS_ALWAYS_INLINE
bool NumericType<T>::reify(JSContext *cx, void *mem, MutableHandleValue vp)
{
    vp.setNumber((double)*((T*)mem) );
    return true;
}

template <typename T>
bool
NumericType<T>::convert(JSContext *cx, HandleValue val, T* converted)
{
    if (val.isInt32()) {
        *converted = T(val.toInt32());
        return true;
    }

    double d;
    if (!ToDoubleForTypedArray(cx, val, &d))
        return false;

    if (TypeIsFloatingPoint<T>()) {
        *converted = T(d);
    } else if (TypeIsUnsigned<T>()) {
        uint32_t n = ToUint32(d);
        *converted = T(n);
    } else {
        int32_t n = ToInt32(d);
        *converted = T(n);
    }

    return true;
}

static bool
ConvertAndCopyTo(JSContext *cx, ScalarTypeRepresentation *typeRepr,
                 HandleValue from, uint8_t *mem)
{
#define CONVERT_CASES(constant_, type_)                                       \
    case constant_: {                                                         \
        type_##_t temp;                                                       \
        if (!NumericType<type_##_t>::convert(cx, from, &temp))                \
            return false;                                                     \
        memcpy(mem, &temp, sizeof(type_##_t));                                \
        return true; }

    switch (typeRepr->type()) {
        JS_FOR_EACH_SCALAR_TYPE_REPR(CONVERT_CASES)
    }
#undef CONVERT_CASES
    return false;
}

static bool
Reify(JSContext *cx, ScalarTypeRepresentation *typeRepr, HandleObject type,
      HandleObject owner, size_t offset, MutableHandleValue to)
{
    JS_ASSERT(&NumericTypeClasses[0] <= type->getClass() &&
              type->getClass() <= &NumericTypeClasses[ScalarTypeRepresentation::NumTypes]);

    switch (typeRepr->type()) {
#define REIFY_CASES(constant_, type_)                                         \
        case constant_:                                                       \
            return NumericType<type_##_t>::reify(                             \
                cx, BlockMem(owner) + offset, to);
        JS_FOR_EACH_SCALAR_TYPE_REPR(REIFY_CASES)
    }
#undef REIFY_CASES

    MOZ_ASSUME_UNREACHABLE("Invalid type");
    return false;
}

template <typename T>
JSBool
NumericType<T>::call(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 1) {
        char *fnName = JS_EncodeString(cx, args.callee().as<JSFunction>().atom());
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED,
                             fnName, "0", "s");
        JS_free(cx, (void *) fnName);
        return false;
    }

    RootedValue arg(cx, args[0]);
    T answer;
    if (!convert(cx, arg, &answer))
        return false; // convert() raises TypeError.

    RootedValue reified(cx);
    if (!NumericType<T>::reify(cx, &answer, &reified)) {
        return false;
    }

    args.rval().set(reified);
    return true;
}

template<ScalarTypeRepresentation::Type N>
JSBool
NumericTypeToString(JSContext *cx, unsigned int argc, Value *vp)
{
    JS_STATIC_ASSERT(N < ScalarTypeRepresentation::NumTypes);
    CallArgs args = CallArgsFromVp(argc, vp);
    JSString *s = JS_NewStringCopyZ(cx, ScalarTypeRepresentation::typeName(N));
    args.rval().set(StringValue(s));
    return true;
}

/*
 * When creating:
 *   var A = new ArrayType(uint8, 10)
 * or
 *   var S = new StructType({...})
 *
 * A.prototype.__proto__ === ArrayType.prototype.prototype (and similar for
 * StructType).
 *
 * This function takes a reference to either ArrayType or StructType and
 * returns a JSObject which can be set as A.prototype.
 */
JSObject *
SetupAndGetPrototypeObjectForComplexTypeInstance(JSContext *cx,
                                                 HandleObject complexTypeGlobal)
{
    RootedObject global(cx, cx->compartment()->maybeGlobal());
    RootedValue complexTypePrototypeVal(cx);
    RootedValue complexTypePrototypePrototypeVal(cx);

    if (!JSObject::getProperty(cx, complexTypeGlobal, complexTypeGlobal,
                               cx->names().classPrototype, &complexTypePrototypeVal))
        return NULL;

    RootedObject complexTypePrototypeObj(cx,
        complexTypePrototypeVal.toObjectOrNull());

    if (!JSObject::getProperty(cx, complexTypePrototypeObj,
                               complexTypePrototypeObj,
                               cx->names().classPrototype,
                               &complexTypePrototypePrototypeVal))
        return NULL;

    RootedObject prototypeObj(cx,
        NewObjectWithGivenProto(cx, &JSObject::class_, NULL, global));

    if (!JS_SetPrototype(cx, prototypeObj,
                         complexTypePrototypePrototypeVal.toObjectOrNull()))
        return NULL;

    return prototypeObj;
}

Class ArrayType::class_ = {
    "ArrayType",
    JSCLASS_HAS_RESERVED_SLOTS(ARRAY_TYPE_RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_ArrayType),
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
    BinaryArray::construct,
    NULL
};

Class BinaryArray::class_ = {
    "BinaryArray",
    Class::NON_NATIVE |
    JSCLASS_HAS_RESERVED_SLOTS(BLOCK_RESERVED_SLOTS) |
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_CACHED_PROTO(JSProto_ArrayType),
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    BinaryArray::finalize,
    NULL,           /* checkAccess */
    NULL,           /* call        */
    NULL,           /* construct   */
    NULL,           /* hasInstance */
    BinaryArray::obj_trace,
    JS_NULL_CLASS_EXT,
    {
        BinaryArray::obj_lookupGeneric,
        BinaryArray::obj_lookupProperty,
        BinaryArray::obj_lookupElement,
        BinaryArray::obj_lookupSpecial,
        NULL,
        NULL,
        NULL,
        NULL,
        BinaryArray::obj_getGeneric,
        BinaryArray::obj_getProperty,
        BinaryArray::obj_getElement,
        BinaryArray::obj_getElementIfPresent,
        BinaryArray::obj_getSpecial,
        BinaryArray::obj_setGeneric,
        BinaryArray::obj_setProperty,
        BinaryArray::obj_setElement,
        BinaryArray::obj_setSpecial,
        BinaryArray::obj_getGenericAttributes,
        BinaryArray::obj_getPropertyAttributes,
        BinaryArray::obj_getElementAttributes,
        BinaryArray::obj_getSpecialAttributes,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        BinaryArray::obj_enumerate,
        NULL,
    }
};

inline JSObject *
ArrayType::elementType(JSContext *cx, HandleObject array)
{
    JS_ASSERT(IsArrayType(array));
    return array->getFixedSlot(SLOT_ARRAY_ELEM_TYPE).toObjectOrNull();
}

static bool
ConvertAndCopyTo(JSContext *cx, ArrayTypeRepresentation *typeRepr,
                 HandleValue from, uint8_t *mem)
{
    if (!from.isObject()) {
        ReportTypeError(cx, from, typeRepr);
        return false;
    }

    RootedObject val(cx, from.toObjectOrNull());
    if (IsBlock(val)) {
        RootedObject valType(cx, GetType(val));
        TypeRepresentation *valTypeRepr = typeRepresentation(cx, valType);
        if (typeRepr == valTypeRepr) {
            memcpy(mem, BlockMem(val), typeRepr->size());
            return true;
        }
        ReportTypeError(cx, from, typeRepr);
        return false;
    }

    RootedObject valRooted(cx, val);
    RootedValue fromLenVal(cx);
    if (!JSObject::getProperty(cx, valRooted, valRooted,
                               cx->names().length, &fromLenVal))
        return false;

    if (!fromLenVal.isInt32()) {
        ReportTypeError(cx, from, typeRepr);
        return false;
    }

    uint32_t fromLen = fromLenVal.toInt32();

    if (typeRepr->length() != fromLen) {
        ReportTypeError(cx, from, typeRepr);
        return false;
    }

    TypeRepresentation *elementType = typeRepr->element();
    uint8_t *p = mem;

    for (uint32_t i = 0; i < fromLen; i++) {
        RootedValue fromElem(cx);
        if (!JSObject::getElement(cx, valRooted, valRooted, i, &fromElem))
            return false;

        if (!ConvertAndCopyTo(cx, elementType, fromElem, p))
            return false;

        p += elementType->size();
    }

    return true;
}

static inline bool
Reify(JSContext *cx, ArrayTypeRepresentation *typeRepr, HandleObject type,
                 HandleObject owner, size_t offset, MutableHandleValue to)
{
    JSObject *obj = BinaryArray::create(cx, type, owner, offset);
    if (!obj)
        return false;
    to.setObject(*obj);
    return true;
}

JSObject *
ArrayType::create(JSContext *cx, HandleObject arrayTypeGlobal,
                  HandleObject elementType, uint32_t length)
{
    JS_ASSERT(elementType);
    JS_ASSERT(IsBinaryType(elementType));

    RootedObject obj(cx, NewBuiltinClassInstance(cx, &ArrayType::class_));
    if (!obj)
        return NULL;

    TypeRepresentation *elementTypeRepr = typeRepresentation(cx, elementType);
    RootedObject typeReprObj(
        cx,
        ArrayTypeRepresentation::New(cx, elementTypeRepr, length));
    obj->setFixedSlot(SLOT_TYPE_REPR, ObjectValue(*typeReprObj));

    RootedValue elementTypeVal(cx, ObjectValue(*elementType));
    if (!JSObject::defineProperty(cx, obj, cx->names().elementType,
                                  elementTypeVal, NULL, NULL,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return NULL;

    obj->setFixedSlot(SLOT_ARRAY_ELEM_TYPE, elementTypeVal);

    RootedValue lengthVal(cx, Int32Value(length));
    if (!JSObject::defineProperty(cx, obj, cx->names().length,
                                  lengthVal, NULL, NULL,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return NULL;

    TypeRepresentation *typeRepr = TypeRepresentation::of(typeReprObj);
    RootedValue typeBytes(cx, NumberValue(typeRepr->size()));
    if (!JSObject::defineProperty(cx, obj, cx->names().bytes,
                                  typeBytes,
                                  NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT))
        return NULL;

    RootedObject prototypeObj(cx,
        SetupAndGetPrototypeObjectForComplexTypeInstance(cx, arrayTypeGlobal));

    if (!prototypeObj)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, obj, prototypeObj))
        return NULL;

    JSFunction *fillFun = DefineFunctionWithReserved(cx, prototypeObj, "fill", BinaryArray::fill, 1, 0);
    if (!fillFun)
        return NULL;

    // This is important
    // so that A.prototype.fill.call(b, val)
    // where b.type != A raises an error
    SetFunctionNativeReserved(fillFun, 0, ObjectValue(*obj));

    RootedId id(cx, NON_INTEGER_ATOM_TO_JSID(cx->names().length));
    unsigned flags = JSPROP_SHARED | JSPROP_GETTER | JSPROP_PERMANENT;

    RootedObject global(cx, cx->compartment()->maybeGlobal());
    JSObject *getter =
        NewFunction(cx, NullPtr(), BinaryArray::lengthGetter,
                    0, JSFunction::NATIVE_FUN, global, NullPtr());
    if (!getter)
        return NULL;

    RootedValue value(cx);
    if (!DefineNativeProperty(cx, prototypeObj, id, value,
                              JS_DATA_TO_FUNC_PTR(PropertyOp, getter), NULL,
                              flags, 0, 0))
        return NULL;
    return obj;
}

JSBool
ArrayType::construct(JSContext *cx, unsigned argc, Value *vp)
{
    if (!JS_IsConstructing(cx, vp)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_NOT_FUNCTION, "ArrayType");
        return false;
    }

    CallArgs args = CallArgsFromVp(argc, vp);

    if (argc != 2 ||
        !args[0].isObject() ||
        !args[1].isNumber() ||
        args[1].toNumber() < 0)
    {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BINARYDATA_ARRAYTYPE_BAD_ARGS);
        return false;
    }

    RootedObject arrayTypeGlobal(cx, &args.callee());
    RootedObject elementType(cx, args[0].toObjectOrNull());

    if (!IsBinaryType(elementType)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BINARYDATA_ARRAYTYPE_BAD_ARGS);
        return false;
    }

    JSObject *obj = create(cx, arrayTypeGlobal, elementType, args[1].toInt32());
    if (!obj)
        return false;
    args.rval().setObject(*obj);
    return true;
}

JSBool
DataInstanceUpdate(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             NULL, JSMSG_MORE_ARGS_NEEDED,
                             "update()", "0", "s");
        return false;
    }

    RootedObject thisObj(cx, args.thisv().toObjectOrNull());
    if (!IsBlock(thisObj)) {
        ReportTypeError(cx, ObjectValue(*thisObj), "BinaryData block");
        return false;
    }

    RootedValue val(cx, args[0]);
    uint8_t *memory = BlockMem(thisObj);

    RootedObject type(cx, GetType(thisObj));
    TypeRepresentation *typeRepr = typeRepresentation(cx, type);
    if (!ConvertAndCopyTo(cx, typeRepr, val, memory))
        return false;

    args.rval().setUndefined();
    return true;
}

static bool
FillBinaryArrayWithValue(JSContext *cx, HandleObject array, HandleValue val)
{
    JS_ASSERT(IsBinaryArray(array));

    RootedObject type(cx, GetType(array));
    ArrayTypeRepresentation *typeRepr = typeRepresentation(cx, type)->toArray();

    uint8_t *base = BlockMem(array);

    // set array[0] = [[Convert]](val)
    if (!ConvertAndCopyTo(cx, typeRepr->element(), val, base))
        return false;

    // Copy a[0] into remaining indices.
    size_t elementSize = typeRepr->element()->size();
    for (uint32_t i = 1; i < typeRepr->length(); i++) {
        uint8_t *dest = base + elementSize * i;
        memcpy(dest, base, elementSize);
    }

    return true;
}

JSBool
ArrayType::repeat(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             NULL, JSMSG_MORE_ARGS_NEEDED,
                             "repeat()", "0", "s");
        return false;
    }

    RootedObject thisObj(cx, args.thisv().toObjectOrNull());
    if (!IsArrayType(thisObj)) {
        JSString *valueStr = JS_ValueToString(cx, args.thisv());
        char *valueChars = const_cast<char*>("(unknown type)");
        if (valueStr)
            valueChars = JS_EncodeString(cx, valueStr);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO, "ArrayType", "repeat", valueChars);
        if (valueStr)
            JS_free(cx, valueChars);
        return false;
    }

    RootedObject binaryArray(cx, BinaryArray::create(cx, thisObj));
    if (!binaryArray)
        return false;

    RootedValue val(cx, args[0]);
    if (!FillBinaryArrayWithValue(cx, binaryArray, val))
        return false;

    args.rval().setObject(*binaryArray);
    return true;
}

JSBool
ArrayType::toString(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject thisObj(cx, args.thisv().toObjectOrNull());
    JS_ASSERT(thisObj);
    if (!IsArrayType(thisObj)) {
        RootedObject obj(cx, args.thisv().toObjectOrNull());
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             NULL, JSMSG_INCOMPATIBLE_PROTO, "ArrayType", "toString", JS_GetClass(obj)->name);
        return false;
    }

    StringBuffer contents(cx);
    if (!typeRepresentation(cx, thisObj)->appendString(cx, contents))
        return false;

    RootedString result(cx, contents.finishString());
    if (!result)
        return false;

    args.rval().setString(result);
    return true;
}

static JSObject *
CreateBinaryDataObject(JSContext *cx,
                       HandleObject type,
                       js::Class *class_)
{
    RootedValue protoVal(cx);
    if (!JSObject::getProperty(cx, type, type,
                               cx->names().classPrototype, &protoVal))
        return NULL;

    RootedObject obj(cx,
         NewObjectWithClassProto(cx, class_, protoVal.toObjectOrNull(), NULL));
    obj->setFixedSlot(SLOT_DATATYPE, ObjectValue(*type));
    obj->setFixedSlot(SLOT_BLOCKREFOWNER, NullValue());
    return obj;
}

JSObject *
BinaryArray::createEmpty(JSContext *cx, HandleObject type)
{
    JS_ASSERT(IsArrayType(type));
    return CreateBinaryDataObject(cx, type, &BinaryArray::class_);
}

JSObject *
BinaryArray::create(JSContext *cx, HandleObject type)
{
    RootedObject obj(cx, createEmpty(cx, type));
    if (!obj)
        return NULL;

    TypeRepresentation *typeRepr = typeRepresentation(cx, type);
    uint32_t memsize = typeRepr->size();
    void *memory = JS_malloc(cx, memsize);
    if (!memory)
        return NULL;
    memset(memory, 0, memsize);
    obj->setPrivate(memory);
    return obj;
}

JSObject *
BinaryArray::create(JSContext *cx, HandleObject type,
                    HandleObject owner, size_t offset)
{
    JS_ASSERT(IsBlock(owner));
    JSObject *obj = createEmpty(cx, type);
    if (!obj)
        return NULL;

    obj->setPrivate(BlockMem(owner) + offset);
    obj->setFixedSlot(SLOT_BLOCKREFOWNER, ObjectValue(*owner));
    return obj;
}

JSBool
BinaryArray::construct(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject callee(cx, &args.callee());

    if (!IsArrayType(callee)) {
        ReportTypeError(cx, args.calleev(), "is not an ArrayType");
        return false;
    }

    RootedObject obj(cx, create(cx, callee));
    if (!obj)
        return false;

    if (argc == 1) {
        RootedValue initial(cx, args[0]);
        if (!ConvertAndCopyTo(cx, initial, obj))
            return false;
    }

    args.rval().setObject(*obj);
    return true;
}

void
BinaryArray::finalize(js::FreeOp *op, JSObject *obj)
{
    if (obj->getFixedSlot(SLOT_BLOCKREFOWNER).isNull())
        op->free_(obj->getPrivate());
}

void
BinaryArray::obj_trace(JSTracer *tracer, JSObject *obj)
{
    Value val = obj->getFixedSlot(SLOT_BLOCKREFOWNER);
    if (val.isObject()) {
        HeapPtrObject owner(val.toObjectOrNull());
        MarkObject(tracer, &owner, "binaryarray.blockRefOwner");
    }
}

JSBool
BinaryArray::lengthGetter(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedObject thisObj(cx, args.thisv().toObjectOrNull());
    JS_ASSERT(IsBinaryArray(thisObj));
    RootedObject type(cx, GetType(thisObj));
    ArrayTypeRepresentation *typeRepr = typeRepresentation(cx, type)->toArray();
    vp->setInt32(typeRepr->length());
    return true;
}

/**
 * The subarray function first creates an ArrayType instance
 * which will act as the elementType for the subarray.
 *
 * var MA = new ArrayType(elementType, 10);
 * var mb = MA.repeat(val);
 *
 * mb.subarray(begin, end=mb.length) => (Only for +ve)
 *     var internalSA = new ArrayType(elementType, end-begin);
 *     var ret = new internalSA()
 *     for (var i = begin; i < end; i++)
 *         ret[i-begin] = ret[i]
 *     return ret
 *
 * The range specified by the begin and end values is clamped to the valid
 * index range for the current array. If the computed length of the new
 * TypedArray would be negative, it is clamped to zero.
 * see: http://www.khronos.org/registry/typedarray/specs/latest/#7
 *
 */
JSBool BinaryArray::subarray(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             NULL, JSMSG_MORE_ARGS_NEEDED,
                             "subarray()", "0", "s");
        return false;
    }

    if (!args[0].isInt32()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BINARYDATA_SUBARRAY_INTEGER_ARG, "1");
        return false;
    }

    RootedObject thisObj(cx, &args.thisv().toObject());
    if (!IsBinaryArray(thisObj)) {
        ReportTypeError(cx, ObjectValue(*thisObj), "binary array");
        return false;
    }

    RootedObject type(cx, GetType(thisObj));
    ArrayTypeRepresentation *typeRepr = typeRepresentation(cx, type)->toArray();
    uint32_t length = typeRepr->length();

    int32_t begin = args[0].toInt32();
    int32_t end = length;

    if (args.length() >= 2) {
        if (!args[1].isInt32()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                    JSMSG_BINARYDATA_SUBARRAY_INTEGER_ARG, "2");
            return false;
        }

        end = args[1].toInt32();
    }

    if (begin < 0)
        begin = length + begin;
    if (end < 0)
        end = length + end;

    begin = Clamp(begin, 0, length);
    end = Clamp(end, 0, length);

    int32_t sublength = end - begin; // end exclusive
    sublength = Clamp(sublength, 0, length);

    RootedObject globalObj(cx, cx->compartment()->maybeGlobal());
    JS_ASSERT(globalObj);
    Rooted<GlobalObject*> global(cx, &globalObj->as<GlobalObject>());
    RootedObject arrayTypeGlobal(cx, global->getOrCreateArrayTypeObject(cx));

    RootedObject elementType(cx, ArrayType::elementType(cx, type));
    RootedObject subArrayType(cx, ArrayType::create(cx, arrayTypeGlobal,
                                                    elementType, sublength));
    if (!subArrayType)
        return false;

    int32_t elementSize = typeRepr->element()->size();
    size_t offset = elementSize * begin;

    RootedObject subarray(cx, BinaryArray::create(cx, subArrayType, thisObj, offset));
    if (!subarray)
        return false;

    args.rval().setObject(*subarray);
    return true;
}

JSBool
BinaryArray::fill(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             NULL, JSMSG_MORE_ARGS_NEEDED,
                             "fill()", "0", "s");
        return false;
    }

    if (!args.thisv().isObject())
        return false;

    RootedObject thisObj(cx, args.thisv().toObjectOrNull());
    if (!IsBinaryArray(thisObj)) {
        ReportTypeError(cx, ObjectValue(*thisObj), "binary array");
        return false;
    }

    Value funArrayTypeVal = GetFunctionNativeReserved(&args.callee(), 0);
    JS_ASSERT(funArrayTypeVal.isObject());

    RootedObject type(cx, GetType(thisObj));
    TypeRepresentation *typeRepr = typeRepresentation(cx, type);
    RootedObject funArrayType(cx, funArrayTypeVal.toObjectOrNull());
    TypeRepresentation *funArrayTypeRepr = typeRepresentation(cx, funArrayType);
    if (typeRepr != funArrayTypeRepr) {
        ReportTypeError(cx, ObjectValue(*thisObj), funArrayTypeRepr);
        return false;
    }

    args.rval().setUndefined();
    RootedValue val(cx, args[0]);
    return FillBinaryArrayWithValue(cx, thisObj, val);
}

JSBool
BinaryArray::obj_lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                MutableHandleObject objp, MutableHandleShape propp)
{
    JS_ASSERT(IsBinaryArray(obj));

    uint32_t index;
    if (js_IdIsIndex(id, &index))
        return obj_lookupElement(cx, obj, index, objp, propp);

    if (JSID_IS_ATOM(id, cx->names().length)) {
        MarkNonNativePropertyFound(propp);
        objp.set(obj);
        return true;
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        objp.set(NULL);
        propp.set(NULL);
        return true;
    }

    return JSObject::lookupGeneric(cx, proto, id, objp, propp);
}

JSBool
BinaryArray::obj_lookupProperty(JSContext *cx,
                                HandleObject obj,
                                HandlePropertyName name,
                                MutableHandleObject objp,
                                MutableHandleShape propp)
{
    RootedId id(cx, NameToId(name));
    return obj_lookupGeneric(cx, obj, id, objp, propp);
}

JSBool
BinaryArray::obj_lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                MutableHandleObject objp, MutableHandleShape propp)
{
    JS_ASSERT(IsBinaryArray(obj));
    MarkNonNativePropertyFound(propp);
    objp.set(obj);
    return true;
}

JSBool
BinaryArray::obj_lookupSpecial(JSContext *cx, HandleObject obj,
                               HandleSpecialId sid, MutableHandleObject objp,
                               MutableHandleShape propp)
{
    RootedId id(cx, SPECIALID_TO_JSID(sid));
    return obj_lookupGeneric(cx, obj, id, objp, propp);
}

JSBool
BinaryArray::obj_getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                             HandleId id, MutableHandleValue vp)
{
    uint32_t index;
    if (js_IdIsIndex(id, &index))
        return obj_getElement(cx, obj, receiver, index, vp);

    RootedValue idValue(cx, IdToValue(id));
    Rooted<PropertyName*> name(cx, ToAtom<CanGC>(cx, idValue)->asPropertyName());
    return obj_getProperty(cx, obj, receiver, name, vp);
}

JSBool
BinaryArray::obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                              HandlePropertyName name, MutableHandleValue vp)
{
    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        vp.setUndefined();
        return true;
    }

    return JSObject::getProperty(cx, proto, receiver, name, vp);
}

JSBool
BinaryArray::obj_getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                             uint32_t index, MutableHandleValue vp)
{
    JS_ASSERT(IsBinaryArray(obj));

    RootedObject type(cx, GetType(obj));
    ArrayTypeRepresentation *typeRepr = typeRepresentation(cx, type)->toArray();

    if (index < typeRepr->length()) {
        RootedObject elementType(cx, ArrayType::elementType(cx, type));
        size_t offset = typeRepr->element()->size() * index;
        return Reify(cx, typeRepr->element(), elementType, obj, offset, vp);
    }

    vp.setUndefined();
    return true;
}

JSBool
BinaryArray::obj_getElementIfPresent(JSContext *cx, HandleObject obj,
                                     HandleObject receiver, uint32_t index,
                                     MutableHandleValue vp, bool *present)
{
    *present = true;
    return obj_getElement(cx, obj, receiver, index, vp);
}

JSBool
BinaryArray::obj_getSpecial(JSContext *cx, HandleObject obj,
                            HandleObject receiver, HandleSpecialId sid,
                            MutableHandleValue vp)
{
    RootedId id(cx, SPECIALID_TO_JSID(sid));
    return obj_getGeneric(cx, obj, receiver, id, vp);
}

JSBool
BinaryArray::obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                             MutableHandleValue vp, JSBool strict)
{
	uint32_t index;
	if (js_IdIsIndex(id, &index)) {
	    return obj_setElement(cx, obj, index, vp, strict);
    }

    if (JSID_IS_ATOM(id, cx->names().length)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             NULL, JSMSG_CANT_REDEFINE_ARRAY_LENGTH);
        return false;
    }

	return true;
}

JSBool
BinaryArray::obj_setProperty(JSContext *cx, HandleObject obj,
                             HandlePropertyName name, MutableHandleValue vp,
                             JSBool strict)
{
    RootedId id(cx, NameToId(name));
    return obj_setGeneric(cx, obj, id, vp, strict);
}

JSBool
BinaryArray::obj_setElement(JSContext *cx, HandleObject obj, uint32_t index,
                             MutableHandleValue vp, JSBool strict)
{
    JS_ASSERT(IsBinaryArray(obj));

    RootedObject type(cx, GetType(obj));
    ArrayTypeRepresentation *typeRepr = typeRepresentation(cx, type)->toArray();

    if (index >= typeRepr->length()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             NULL, JSMSG_BINARYDATA_BINARYARRAY_BAD_INDEX);
        return false;
    }

    uint32_t offset = typeRepr->element()->size() * index;
    uint8_t *mem = BlockMem(obj) + offset;
    return ConvertAndCopyTo(cx, typeRepr->element(), vp, mem);
}

JSBool
BinaryArray::obj_setSpecial(JSContext *cx, HandleObject obj,
                             HandleSpecialId sid, MutableHandleValue vp,
                             JSBool strict)
{
    RootedId id(cx, SPECIALID_TO_JSID(sid));
    return obj_setGeneric(cx, obj, id, vp, strict);
}

JSBool
BinaryArray::obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                       HandleId id, unsigned *attrsp)
{
    uint32_t index;
    RootedObject type(cx, GetType(obj));

    if (js_IdIsIndex(id, &index)) {
        *attrsp = JSPROP_ENUMERATE | JSPROP_PERMANENT;
        return true;
    }

    if (JSID_IS_ATOM(id, cx->names().length)) {
        *attrsp = JSPROP_READONLY | JSPROP_PERMANENT;
        return true;
    }

    return false;
}

JSBool
BinaryArray::obj_getPropertyAttributes(JSContext *cx, HandleObject obj,
                                        HandlePropertyName name,
                                        unsigned *attrsp)
{
    RootedId id(cx, NameToId(name));
    return obj_getGenericAttributes(cx, obj, id, attrsp);
}

JSBool
BinaryArray::obj_getElementAttributes(JSContext *cx, HandleObject obj,
                                       uint32_t index, unsigned *attrsp)
{
    RootedId id(cx, INT_TO_JSID(index));
    return obj_getGenericAttributes(cx, obj, id, attrsp);
}

JSBool
BinaryArray::obj_getSpecialAttributes(JSContext *cx, HandleObject obj,
                                       HandleSpecialId sid, unsigned *attrsp)
{
    RootedId id(cx, SPECIALID_TO_JSID(sid));
    return obj_getGenericAttributes(cx, obj, id, attrsp);
}

JSBool
BinaryArray::obj_enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
                            MutableHandleValue statep, MutableHandleId idp)
{
    JS_ASSERT(IsBinaryArray(obj));

    RootedObject type(cx, GetType(obj));
    ArrayTypeRepresentation *typeRepr = typeRepresentation(cx, type)->toArray();

    uint32_t index;
    switch (enum_op) {
        case JSENUMERATE_INIT_ALL:
        case JSENUMERATE_INIT:
            statep.setInt32(0);
            idp.set(INT_TO_JSID(typeRepr->length()));
            break;

        case JSENUMERATE_NEXT:
            index = static_cast<uint32_t>(statep.toInt32());

            if (index < typeRepr->length()) {
                idp.set(INT_TO_JSID(index));
                statep.setInt32(index + 1);
            } else {
                JS_ASSERT(index == typeRepr->length());
                statep.setNull();
            }

            break;

        case JSENUMERATE_DESTROY:
            statep.setNull();
            break;
    }

	return true;
}

/*********************************
 * Structs
 *********************************/
Class StructType::class_ = {
    "StructType",
    JSCLASS_HAS_RESERVED_SLOTS(STRUCT_TYPE_RESERVED_SLOTS) |
    JSCLASS_HAS_PRIVATE | // used to store FieldList
    JSCLASS_HAS_CACHED_PROTO(JSProto_StructType),
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    NULL, /* finalize */
    NULL, /* checkAccess */
    NULL, /* call */
    NULL, /* hasInstance */
    BinaryStruct::construct,
    NULL  /* trace */
};

Class BinaryStruct::class_ = {
    "BinaryStruct",
    Class::NON_NATIVE |
    JSCLASS_HAS_RESERVED_SLOTS(BLOCK_RESERVED_SLOTS) |
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_CACHED_PROTO(JSProto_StructType),
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    BinaryStruct::finalize,
    NULL,           /* checkAccess */
    NULL,           /* call        */
    NULL,           /* hasInstance */
    NULL,           /* construct   */
    BinaryStruct::obj_trace,
    JS_NULL_CLASS_EXT,
    {
        BinaryStruct::obj_lookupGeneric,
        NULL, /* obj_lookupProperty */
        NULL, /* obj_lookupElement */
        NULL, /* obj_lookupSpecial */
        NULL, /* defineGeneric */
        NULL, /* defineProperty */
        NULL, /* defineElement */
        NULL, /* defineSpecial */
        BinaryStruct::obj_getGeneric,
        BinaryStruct::obj_getProperty,
        NULL, /* getElement */
        NULL, /* getElementIfPresent */
        BinaryStruct::obj_getSpecial,
        BinaryStruct::obj_setGeneric,
        BinaryStruct::obj_setProperty,
        NULL, /* setElement */
        NULL, /* setSpecial */
        NULL, /* getGenericAttributes */
        NULL, /* getPropertyAttributes */
        NULL, /* getElementAttributes */
        NULL, /* getSpecialAttributes */
        NULL, /* setGenericAttributes */
        NULL, /* setPropertyAttributes */
        NULL, /* setElementAttributes */
        NULL, /* setSpecialAttributes */
        NULL, /* deleteProperty */
        NULL, /* deleteElement */
        NULL, /* deleteSpecial */
        BinaryStruct::obj_enumerate,
        NULL, /* thisObject */
    }
};

static bool
ConvertAndCopyTo(JSContext *cx,
                 StructTypeRepresentation *typeRepr,
                 HandleValue from,
                 uint8_t *mem)
{
    if (!from.isObject()) {
        ReportTypeError(cx, from, typeRepr);
        return false;
    }

    RootedObject val(cx, from.toObjectOrNull());
    if (IsBlock(val)) {
        RootedObject valType(cx, GetType(val));
        TypeRepresentation *valTypeRepr = typeRepresentation(cx, valType);
        if (typeRepr == valTypeRepr) {
            memcpy(mem, BlockMem(val), typeRepr->size());
            return true;
        }
        ReportTypeError(cx, from, typeRepr);
        return false;
    }

    RootedObject valRooted(cx, val);
    AutoIdVector ownProps(cx);
    if (!GetPropertyNames(cx, valRooted, JSITER_OWNONLY, &ownProps)) {
        ReportTypeError(cx, from, typeRepr);
        return false;
    }

    if (ownProps.length() != typeRepr->fieldCount()) {
        ReportTypeError(cx, from, typeRepr);
        return false;
    }

    for (uint32_t i = 0; i < ownProps.length(); i++) {
        if (!typeRepr->fieldNamed(ownProps.handleAt(i))) {
            ReportTypeError(cx, from, typeRepr);
            return false;
        }
    }

    for (uint32_t i = 0; i < typeRepr->fieldCount(); i++) {
        const StructField &field = typeRepr->field(i);
        RootedPropertyName fieldName(cx,
            JSID_TO_ATOM(field.id)->asPropertyName());

        RootedValue fromProp(cx);
        if (!JSObject::getProperty(cx, valRooted, valRooted,
                                   fieldName, &fromProp))
            return false;

        if (!ConvertAndCopyTo(cx, field.typeRepr, fromProp,
                              mem + field.offset))
            return false;
    }
    return true;
}

/*
 * NOTE: layout() does not check for duplicates in fields since the arguments
 * to StructType are currently passed as an object literal. Fix this if it
 * changes to taking an array of arrays.
 */
bool
StructType::layout(JSContext *cx, HandleObject structType, HandleObject fields)
{
    AutoIdVector ids(cx);
    if (!GetPropertyNames(cx, fields, JSITER_OWNONLY, &ids))
        return false;

    AutoValueVector fieldTypeObjs(cx);
    AutoObjectVector fieldTypeReprObjs(cx);

    for (unsigned int i = 0; i < ids.length(); i++) {
        RootedValue fieldTypeVal(cx);
        RootedId id(cx, ids[i]);
        if (!JSObject::getGeneric(cx, fields, fields, id, &fieldTypeVal))
            return false;

        if (!fieldTypeObjs.append(fieldTypeVal)) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        RootedObject fieldType(cx, fieldTypeVal.toObjectOrNull());
        if (!IsBinaryType(fieldType)) {
            ReportTypeError(cx, ObjectValue(*fields),
                            "StructType field specifier");
            return false;
        }

        if (!fieldTypeReprObjs.append(typeRepresentationObj(fieldType))) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }

    RootedObject typeReprObj(
        cx,
        StructTypeRepresentation::New(cx, ids, fieldTypeReprObjs));
    TypeRepresentation *typeRepr = TypeRepresentation::of(typeReprObj);

    structType->setFixedSlot(SLOT_TYPE_REPR, ObjectValue(*typeReprObj));

    RootedObject fieldTypeVec(
        cx,
        NewDenseCopiedArray(cx, fieldTypeObjs.length(), fieldTypeObjs.begin()));

    structType->setFixedSlot(SLOT_STRUCT_FIELD_TYPES, ObjectValue(*fieldTypeVec));

    RootedValue typeBytes(cx, NumberValue(typeRepr->size()));
    if (!JSObject::defineProperty(cx, structType, cx->names().bytes, typeBytes,
                                  NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT))
        return NULL;

    return true;
}

static bool
Reify(JSContext *cx, StructTypeRepresentation *typeRepr, HandleObject type,
      HandleObject owner, size_t offset, MutableHandleValue to)
{
    JSObject *obj = BinaryStruct::create(cx, type, owner, offset);
    if (!obj)
        return false;
    to.setObject(*obj);
    return true;
}

JSObject *
StructType::create(JSContext *cx, HandleObject structTypeGlobal,
                   HandleObject fields)
{
    RootedObject obj(cx, NewBuiltinClassInstance(cx, &StructType::class_));
    if (!obj)
        return NULL;

    if (!StructType::layout(cx, obj, fields))
        return NULL;

    RootedObject fieldsProto(cx);
    if (!JSObject::getProto(cx, fields, &fieldsProto))
        return NULL;

    RootedObject clone(cx, CloneObject(cx, fields, fieldsProto, NullPtr()));
    if (!clone)
        return NULL;

    if (!JS_DefineProperty(cx, obj, "fields",
                           ObjectValue(*fields), NULL, NULL,
                           JSPROP_READONLY | JSPROP_PERMANENT))
        return NULL;

    JSObject *prototypeObj =
        SetupAndGetPrototypeObjectForComplexTypeInstance(cx, structTypeGlobal);

    if (!prototypeObj)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, obj, prototypeObj))
        return NULL;

    return obj;
}

JSBool
StructType::construct(JSContext *cx, unsigned int argc, Value *vp)
{
    if (!JS_IsConstructing(cx, vp)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_NOT_FUNCTION, "StructType");
        return false;
    }

    CallArgs args = CallArgsFromVp(argc, vp);

    if (argc >= 1 && args[0].isObject()) {
        RootedObject structTypeGlobal(cx, &args.callee());
        RootedObject fields(cx, args[0].toObjectOrNull());
        JSObject *obj = create(cx, structTypeGlobal, fields);
        if (!obj)
            return false;
        args.rval().setObject(*obj);
        return true;
    }

    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_BINARYDATA_STRUCTTYPE_BAD_ARGS);
    return false;
}

JSBool
StructType::toString(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject thisObj(cx, args.thisv().toObjectOrNull());

    if (!IsStructType(thisObj))
        return false;

    StringBuffer contents(cx);
    if (!typeRepresentation(cx, thisObj)->appendString(cx, contents))
        return false;

    RootedString result(cx, contents.finishString());
    if (!result)
        return false;

    args.rval().setString(result);
    return true;
}

JSObject *
BinaryStruct::createEmpty(JSContext *cx, HandleObject type)
{
    JS_ASSERT(IsStructType(type));
    return CreateBinaryDataObject(cx, type, &BinaryStruct::class_);
}

JSObject *
BinaryStruct::create(JSContext *cx, HandleObject type)
{
    JSObject *obj = createEmpty(cx, type);
    if (!obj)
        return NULL;

    TypeRepresentation *typeRepr = typeRepresentation(cx, type);
    uint32_t memsize = typeRepr->size();
    void *memory = JS_malloc(cx, memsize);
    if (!memory)
        return NULL;
    memset(memory, 0, memsize);
    obj->setPrivate(memory);
    return obj;
}

JSObject *
BinaryStruct::create(JSContext *cx, HandleObject type,
                     HandleObject owner, size_t offset)
{
    JS_ASSERT(IsBlock(owner));
    JSObject *obj = createEmpty(cx, type);
    if (!obj)
        return NULL;

    obj->setPrivate(BlockMem(owner) + offset);
    obj->setFixedSlot(SLOT_BLOCKREFOWNER, ObjectValue(*owner));
    return obj;
}

JSBool
BinaryStruct::construct(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject callee(cx, &args.callee());

    if (!IsStructType(callee)) {
        ReportTypeError(cx, args.calleev(), "is not an StructType");
        return false;
    }

    RootedObject obj(cx, create(cx, callee));
    if (!obj)
        return false;

    if (argc == 1) {
        RootedValue initial(cx, args[0]);
        if (!ConvertAndCopyTo(cx, initial, obj))
            return false;
    }

    args.rval().setObject(*obj);
    return true;
}

void
BinaryStruct::finalize(js::FreeOp *op, JSObject *obj)
{
    if (obj->getFixedSlot(SLOT_BLOCKREFOWNER).isNull())
        op->free_(obj->getPrivate());
}

void
BinaryStruct::obj_trace(JSTracer *tracer, JSObject *obj)
{
    Value val = obj->getFixedSlot(SLOT_BLOCKREFOWNER);
    if (val.isObject()) {
        HeapPtrObject owner(val.toObjectOrNull());
        MarkObject(tracer, &owner, "binarystruct.blockRefOwner");
    }

    HeapPtrObject type(obj->getFixedSlot(SLOT_DATATYPE).toObjectOrNull());
    MarkObject(tracer, &type, "binarystruct.type");
}

JSBool
BinaryStruct::obj_enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
                            MutableHandleValue statep, MutableHandleId idp)
{
    JS_ASSERT(IsBinaryStruct(obj));

    RootedObject type(cx, GetType(obj));
    StructTypeRepresentation *typeRepr =
        typeRepresentation(cx, type)->toStruct();

    uint32_t index;
    switch (enum_op) {
        case JSENUMERATE_INIT_ALL:
        case JSENUMERATE_INIT:
            statep.setInt32(0);
            idp.set(INT_TO_JSID(typeRepr->fieldCount()));
            break;

        case JSENUMERATE_NEXT:
            index = static_cast<uint32_t>(statep.toInt32());

            if (index < typeRepr->fieldCount()) {
                idp.set(typeRepr->field(index).id);
                statep.setInt32(index + 1);
            } else {
                statep.setNull();
            }

            break;

        case JSENUMERATE_DESTROY:
            statep.setNull();
            break;
    }

    return true;
}

JSBool
BinaryStruct::obj_lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                MutableHandleObject objp, MutableHandleShape propp)
{
    JS_ASSERT(IsBinaryStruct(obj));

    RootedObject type(cx, GetType(obj));
    JS_ASSERT(IsStructType(type));
    StructTypeRepresentation *typeRepr = typeRepresentation(cx, type)->toStruct();
    const StructField *field = typeRepr->fieldNamed(id);
    if (field) {
        MarkNonNativePropertyFound(propp);
        objp.set(obj);
        return true;
    }

    RootedObject proto(cx, obj->getProto());
    JS_ASSERT(proto);
    return JSObject::lookupGeneric(cx, proto, id, objp, propp);
}

JSBool
BinaryStruct::obj_getGeneric(JSContext *cx, HandleObject obj,
                             HandleObject receiver, HandleId id,
                             MutableHandleValue vp)
{
    JS_ASSERT(IsBinaryStruct(obj));

    RootedObject type(cx, GetType(obj));
    JS_ASSERT(IsStructType(type));
    StructTypeRepresentation *typeRepr = typeRepresentation(cx, type)->toStruct();
    const StructField *field = typeRepr->fieldNamed(id);
    if (!field) {
        RootedObject proto(cx, obj->getProto());
        JS_ASSERT(proto);
        return JSObject::getGeneric(cx, proto, receiver, id, vp);
    }

    // Recover the original type object here (`field` contains only
    // its canonical form). The difference is of course user
    // observable, e.g. in a program like:
    //
    //     var Point1 = new StructType({x:uint8, y:uint8});
    //     var Point2 = new StructType({x:uint8, y:uint8});
    //     var Line1 = new StructType({start:Point1, end: Point1});
    //     var Line2 = new StructType({start:Point2, end: Point2});
    //     var line1 = new Line1(...);
    //     var line2 = new Line1(...);
    //
    // In this scenario, line1.start.type() === Point1 and
    // line2.start.type() === Point2.
    RootedObject fieldTypes(
        cx,
        type->getFixedSlot(SLOT_STRUCT_FIELD_TYPES).toObjectOrNull());
    RootedValue fieldTypeVal(cx);
    if (!JSObject::getElement(cx, fieldTypes, fieldTypes,
                              field->index, &fieldTypeVal))
        return false;

    RootedObject fieldType(cx, fieldTypeVal.toObjectOrNull());
    return Reify(cx, field->typeRepr, fieldType, obj, field->offset, vp);
}

JSBool
BinaryStruct::obj_getProperty(JSContext *cx, HandleObject obj,
                              HandleObject receiver, HandlePropertyName name,
                              MutableHandleValue vp)
{
    RootedId id(cx, NON_INTEGER_ATOM_TO_JSID(&(*name)));
    return obj_getGeneric(cx, obj, receiver, id, vp);
}

JSBool
BinaryStruct::obj_getSpecial(JSContext *cx, HandleObject obj,
                             HandleObject receiver, HandleSpecialId sid,
                             MutableHandleValue vp)
{
    RootedId id(cx, SPECIALID_TO_JSID(sid));
    return obj_getGeneric(cx, obj, receiver, id, vp);
}

JSBool
BinaryStruct::obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                             MutableHandleValue vp, JSBool strict)
{
    if (!IsBinaryStruct(obj)) {
        char *valueStr = JS_EncodeString(cx, JS_ValueToString(cx, ObjectValue(*obj)));
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                JSMSG_BINARYDATA_NOT_BINARYSTRUCT, valueStr);
        JS_free(cx, (void *) valueStr);
        return false;
    }

    RootedObject type(cx, GetType(obj));
    JS_ASSERT(IsStructType(type));
    StructTypeRepresentation *typeRepr = typeRepresentation(cx, type)->toStruct();
    const StructField *field = typeRepr->fieldNamed(id);
    if (!field) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_UNDEFINED_PROP, IdToString(cx, id));
        return false;
    }

    uint8_t *loc = BlockMem(obj) + field->offset;
    return ConvertAndCopyTo(cx, field->typeRepr, vp, loc);
}

JSBool
BinaryStruct::obj_setProperty(JSContext *cx, HandleObject obj,
                              HandlePropertyName name, MutableHandleValue vp,
                              JSBool strict)
{
    RootedId id(cx, NON_INTEGER_ATOM_TO_JSID(&(*name)));
    return obj_setGeneric(cx, obj, id, vp, strict);
}

static bool
ConvertAndCopyTo(JSContext *cx,
                 TypeRepresentation *typeRepr,
                 HandleValue from,
                 uint8_t *mem)
{
    switch (typeRepr->kind()) {
      case TypeRepresentation::Scalar:
        return ConvertAndCopyTo(cx, typeRepr->toScalar(), from, mem);

      case TypeRepresentation::Array:
        return ConvertAndCopyTo(cx, typeRepr->toArray(), from, mem);

      case TypeRepresentation::Struct:
        return ConvertAndCopyTo(cx, typeRepr->toStruct(), from, mem);
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
    return false;
}

///////////////////////////////////////////////////////////////////////////

static bool
Reify(JSContext *cx, TypeRepresentation *typeRepr, HandleObject type,
      HandleObject owner, size_t offset, MutableHandleValue to)
{
    JS_ASSERT(typeRepr == typeRepresentation(cx, type));
    switch (typeRepr->kind()) {
      case TypeRepresentation::Array:
        return Reify(cx, typeRepr->toArray(), type, owner, offset, to);
      case TypeRepresentation::Struct:
        return Reify(cx, typeRepr->toStruct(), type, owner, offset, to);
      case TypeRepresentation::Scalar:
        return Reify(cx, typeRepr->toScalar(), type, owner, offset, to);
    }
    MOZ_ASSUME_UNREACHABLE("Invalid typeRepr kind");
}

bool
GlobalObject::initDataObject(JSContext *cx, Handle<GlobalObject *> global)
{
    RootedObject DataProto(cx);
    DataProto = NewObjectWithGivenProto(cx, &DataClass,
                                        global->getOrCreateObjectPrototype(cx),
                                        global, SingletonObject);
    if (!DataProto)
        return false;

    RootedAtom DataName(cx, ClassName(JSProto_Data, cx));
    RootedFunction DataCtor(cx,
            global->createConstructor(cx, DataThrowError, DataName,
                                      1, JSFunction::ExtendedFinalizeKind));

    if (!DataCtor)
        return false;

    if (!JS_DefineFunction(cx, DataProto, "update", DataInstanceUpdate, 1, 0))
        return false;

    if (!LinkConstructorAndPrototype(cx, DataCtor, DataProto))
        return false;

    if (!DefineConstructorAndPrototype(cx, global, JSProto_Data,
                                       DataCtor, DataProto))
        return false;

    global->setReservedSlot(JSProto_Data, ObjectValue(*DataCtor));
    return true;
}

bool
GlobalObject::initTypeObject(JSContext *cx, Handle<GlobalObject *> global)
{
    RootedObject TypeProto(cx, global->getOrCreateDataObject(cx));
    if (!TypeProto)
        return false;

    RootedAtom TypeName(cx, ClassName(JSProto_Type, cx));
    RootedFunction TypeCtor(cx,
            global->createConstructor(cx, TypeThrowError, TypeName,
                                      1, JSFunction::ExtendedFinalizeKind));
    if (!TypeCtor)
        return false;

    if (!LinkConstructorAndPrototype(cx, TypeCtor, TypeProto))
        return false;

    if (!DefineConstructorAndPrototype(cx, global, JSProto_Type,
                                       TypeCtor, TypeProto))
        return false;

    global->setReservedSlot(JSProto_Type, ObjectValue(*TypeCtor));
    return true;
}

bool
GlobalObject::initArrayTypeObject(JSContext *cx, Handle<GlobalObject *> global)
{
    RootedFunction ctor(cx,
        global->createConstructor(cx, ArrayType::construct,
                                  cx->names().ArrayType, 2));

    global->setReservedSlot(JSProto_ArrayTypeObject, ObjectValue(*ctor));
    return true;
}

static JSObject *
SetupComplexHeirarchy(JSContext *cx, HandleObject obj, JSProtoKey protoKey,
                      HandleObject complexObject, MutableHandleObject proto,
                      MutableHandleObject protoProto)
{
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    // get the 'Type' constructor
    RootedObject TypeObject(cx, global->getOrCreateTypeObject(cx));
    if (!TypeObject)
        return NULL;

    // Set complexObject.__proto__ = Type
    if (!JS_SetPrototype(cx, complexObject, TypeObject))
        return NULL;

    RootedObject DataObject(cx, global->getOrCreateDataObject(cx));
    if (!DataObject)
        return NULL;

    RootedValue DataProtoVal(cx);
    if (!JSObject::getProperty(cx, DataObject, DataObject,
                               cx->names().classPrototype, &DataProtoVal))
        return NULL;

    RootedObject DataProto(cx, DataProtoVal.toObjectOrNull());
    if (!DataProto)
        return NULL;

    RootedObject prototypeObj(cx,
        NewObjectWithGivenProto(cx, &JSObject::class_, NULL, global));
    if (!prototypeObj)
        return NULL;
    if (!LinkConstructorAndPrototype(cx, complexObject, prototypeObj))
        return NULL;
    if (!DefineConstructorAndPrototype(cx, global, protoKey,
                                       complexObject, prototypeObj))
        return NULL;

    // Set complexObject.prototype.__proto__ = Data
    if (!JS_SetPrototype(cx, prototypeObj, DataObject))
        return NULL;

    proto.set(prototypeObj);

    // Set complexObject.prototype.prototype.__proto__ = Data.prototype
    RootedObject prototypePrototypeObj(cx, JS_NewObject(cx, NULL, NULL,
                                       global));

    if (!LinkConstructorAndPrototype(cx, prototypeObj,
                                     prototypePrototypeObj))
        return NULL;

    if (!JS_SetPrototype(cx, prototypePrototypeObj, DataProto))
        return NULL;

    protoProto.set(prototypePrototypeObj);

    return complexObject;
}

static JSObject *
InitArrayType(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    RootedObject ctor(cx, global->getOrCreateArrayTypeObject(cx));
    if (!ctor)
        return NULL;

    RootedObject proto(cx);
    RootedObject protoProto(cx);
    if (!SetupComplexHeirarchy(cx, obj, JSProto_ArrayType,
                               ctor, &proto, &protoProto))
        return NULL;

    if (!JS_DefineFunction(cx, proto, "repeat", ArrayType::repeat, 1, 0))
        return NULL;

    if (!JS_DefineFunction(cx, proto, "toString", ArrayType::toString, 0, 0))
        return NULL;

    RootedObject arrayProto(cx);
    if (!FindProto(cx, &ArrayObject::class_, &arrayProto))
        return NULL;

    RootedValue forEachFunVal(cx);
    RootedAtom forEachAtom(cx, Atomize(cx, "forEach", 7));
    RootedId forEachId(cx, AtomToId(forEachAtom));
    if (!JSObject::getProperty(cx, arrayProto, arrayProto, forEachAtom->asPropertyName(), &forEachFunVal))
        return NULL;

    if (!JSObject::defineGeneric(cx, protoProto, forEachId, forEachFunVal, NULL, NULL, 0))
        return NULL;

    if (!JS_DefineFunction(cx, protoProto, "subarray",
                           BinaryArray::subarray, 1, 0))
        return NULL;

    return proto;
}

static JSObject *
InitStructType(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    RootedFunction ctor(cx,
        global->createConstructor(cx, StructType::construct,
                                  cx->names().StructType, 1));

    if (!ctor)
        return NULL;

    RootedObject proto(cx);
    RootedObject protoProto(cx);
    if (!SetupComplexHeirarchy(cx, obj, JSProto_StructType,
                               ctor, &proto, &protoProto))
        return NULL;

    if (!JS_DefineFunction(cx, proto, "toString", StructType::toString, 0, 0))
        return NULL;

    return proto;
}

template<ScalarTypeRepresentation::Type type>
static bool
DefineNumericClass(JSContext *cx,
                   HandleObject global,
                   HandleObject globalProto,
                   HandleObject obj,
                   const char *name)
{
    RootedObject numFun(
        cx,
        JS_DefineObject(cx, global, name,
                        (JSClass *) &NumericTypeClasses[type],
                        globalProto, 0));
    if (!numFun)
        return false;

    RootedObject typeReprObj(cx, ScalarTypeRepresentation::New(cx, type));
    TypeRepresentation *typeRepr = TypeRepresentation::of(typeReprObj);

    numFun->setFixedSlot(SLOT_TYPE_REPR, ObjectValue(*typeReprObj));

    RootedValue sizeVal(cx, NumberValue(typeRepr->size()));
    if (!JSObject::defineProperty(cx, numFun, cx->names().bytes,
                                  sizeVal,
                                  NULL, NULL,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return false;

    if (!JS_DefineFunction(cx, numFun, "toString",
                           NumericTypeToString<type>, 0, 0))
        return false;

    return true;
}

JSObject *
js_InitBinaryDataClasses(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->is<GlobalObject>());
    Rooted<GlobalObject *> global(cx, &obj->as<GlobalObject>());

    RootedObject globalProto(cx, JS_GetFunctionPrototype(cx, global));
#define BINARYDATA_NUMERIC_DEFINE(constant_, type_)                             \
    if (!DefineNumericClass<constant_>(cx, global, globalProto, obj, #type_))   \
        return NULL;
    JS_FOR_EACH_SCALAR_TYPE_REPR(BINARYDATA_NUMERIC_DEFINE)
#undef BINARYDATA_NUMERIC_DEFINE

    if (!InitArrayType(cx, obj))
        return NULL;

    if (!InitStructType(cx, obj))
        return NULL;

    return global;
}
