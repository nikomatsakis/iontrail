/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "builtin/TypeRepresentation.h"

#include "mozilla/HashFunctions.h"

#include "jscntxt.h"
#include "jsnum.h"
#include "jsutil.h"

#include "js/HashTable.h"
#include "vm/Runtime.h"
#include "vm/StringBuffer.h"

#include "jsgcinlines.h"
#include "jsobjinlines.h"

using namespace js;
using namespace mozilla;

///////////////////////////////////////////////////////////////////////////
// Class def'n for the owner object

const Class TypeRepresentation::class_ = {
    "TypeRepresentation",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(JS_TYPEREPR_SLOTS),
    JS_PropertyStub,         /* addProperty */
    JS_DeletePropertyStub,   /* delProperty */
    JS_PropertyStub,         /* getProperty */
    JS_StrictPropertyStub,   /* setProperty */
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    obj_finalize,
    nullptr,        /* checkAccess */
    nullptr,        /* call        */
    nullptr,        /* hasInstance */
    nullptr,        /* construct   */
    obj_trace,
};

///////////////////////////////////////////////////////////////////////////
// Hashing

bool
TypeRepresentationHasher::match(TypeRepresentation *key1,
                                TypeRepresentation *key2)
{
    if (key1->kind() != key2->kind())
        return false;

    switch (key1->kind()) {
      case TypeRepresentation::Scalar:
        return matchScalars(key1->asScalar(), key2->asScalar());

      case TypeRepresentation::Float32x4:
        return matchFloat32x4s(key1->asFloat32x4(), key2->asFloat32x4());

      case TypeRepresentation::Int32x4:
        return matchInt32x4s(key1->asInt32x4(), key2->asInt32x4());

      case TypeRepresentation::Struct:
        return matchStructs(key1->asStruct(), key2->asStruct());

      case TypeRepresentation::Array:
        return matchArrays(key1->asArray(), key2->asArray());
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
}

bool
TypeRepresentationHasher::matchScalars(ScalarTypeRepresentation *key1,
                                       ScalarTypeRepresentation *key2)
{
    return key1->type() == key2->type();
}

bool
TypeRepresentationHasher::matchFloat32x4s(Float32x4TypeRepresentation *key1,
                                          Float32x4TypeRepresentation *key2)
{
    return true;
}

bool
TypeRepresentationHasher::matchInt32x4s(Int32x4TypeRepresentation *key1,
                                        Int32x4TypeRepresentation *key2)
{
    return true;
}

bool
TypeRepresentationHasher::matchStructs(StructTypeRepresentation *key1,
                                       StructTypeRepresentation *key2)
{
    if (key1->fieldCount() != key2->fieldCount())
        return false;

    for (size_t i = 0; i < key1->fieldCount(); i++) {
        if (key1->field(i).id != key2->field(i).id)
            return false;

        if (key1->field(i).typeRepr != key2->field(i).typeRepr)
            return false;
    }

    return true;
}

bool
TypeRepresentationHasher::matchArrays(ArrayTypeRepresentation *key1,
                                      ArrayTypeRepresentation *key2)
{
    // We assume that these pointers have been canonicalized:
    return key1->element() == key2->element() &&
           key1->length() == key2->length();
}

HashNumber
TypeRepresentationHasher::hash(TypeRepresentation *key) {
    switch (key->kind()) {
      case TypeRepresentation::Scalar:
        return hashScalar(key->asScalar());

      case TypeRepresentation::Float32x4:
        return hashFloat32x4(key->asFloat32x4());

      case TypeRepresentation::Int32x4:
        return hashInt32x4(key->asInt32x4());

      case TypeRepresentation::Struct:
        return hashStruct(key->asStruct());

      case TypeRepresentation::Array:
        return hashArray(key->asArray());
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
}

HashNumber
TypeRepresentationHasher::hashScalar(ScalarTypeRepresentation *key)
{
    return HashGeneric(key->kind(), key->type());
}

HashNumber
TypeRepresentationHasher::hashFloat32x4(Float32x4TypeRepresentation *key)
{
    return HashGeneric(key->kind());
}

HashNumber
TypeRepresentationHasher::hashInt32x4(Int32x4TypeRepresentation *key)
{
    return HashGeneric(key->kind());
}

HashNumber
TypeRepresentationHasher::hashStruct(StructTypeRepresentation *key)
{
    HashNumber hash = HashGeneric(key->kind());
    for (HashNumber i = 0; i < key->fieldCount(); i++) {
        hash = AddToHash(hash, JSID_BITS(key->field(i).id.get()));
        hash = AddToHash(hash, key->field(i).typeRepr);
    }
    return hash;
}

HashNumber
TypeRepresentationHasher::hashArray(ArrayTypeRepresentation *key)
{
    return HashGeneric(key->kind(), key->element(), key->length());
}

///////////////////////////////////////////////////////////////////////////
// Constructors

TypeRepresentation::TypeRepresentation(Kind kind, size_t size, size_t align)
  : size_(size),
    alignment_(align),
    kind_(kind)
{}

static size_t ScalarSizes[] = {
#define SCALAR_SIZE(_kind, _type, _name)                        \
    sizeof(_type),
    JS_FOR_EACH_SCALAR_TYPE_REPR(SCALAR_SIZE) 0
#undef SCALAR_SIZE
};

ScalarTypeRepresentation::ScalarTypeRepresentation(Type type)
  : TypeRepresentation(Scalar, ScalarSizes[type], ScalarSizes[type]),
    type_(type)
{
}

Float32x4TypeRepresentation::Float32x4TypeRepresentation()
  : TypeRepresentation(Float32x4, sizeof(js::Float32x4), sizeof(js::Float32x4))
{
}

Int32x4TypeRepresentation::Int32x4TypeRepresentation()
  : TypeRepresentation(Int32x4, sizeof(js::Int32x4), sizeof(js::Int32x4))
{
}

ArrayTypeRepresentation::ArrayTypeRepresentation(TypeRepresentation *element,
                                                 size_t length)
  : TypeRepresentation(Array, element->size() * length, element->alignment()),
    element_(element),
    length_(length)
{
}

static inline size_t alignTo(size_t address, size_t align) {
    JS_ASSERT(IsPowerOfTwo(align));
    return (address + align - 1) & -align;
}

StructField::StructField(size_t index,
                         jsid &id,
                         TypeRepresentation *typeRepr,
                         size_t offset)
  : index(index),
    id(id),
    typeRepr(typeRepr),
    offset(offset)
{}

StructTypeRepresentation::StructTypeRepresentation()
  : TypeRepresentation(Struct, 0, 1),
    fieldCount_(0) // see ::init() below!
{
}

bool
StructTypeRepresentation::init(JSContext *cx,
                               AutoIdVector &ids,
                               AutoObjectVector &typeReprOwners)
{
    JS_ASSERT(ids.length() == typeReprOwners.length());
    fieldCount_ = ids.length();

    // We compute alignment into the field `align_` directly in the
    // loop below, but not `size_` because we have to very careful
    // about overflow. For now, we always use a uint32_t for
    // consistency across build environments.
    uint32_t totalSize = 0;

    for (size_t i = 0; i < ids.length(); i++) {
        TypeRepresentation *fieldTypeRepr = fromOwnerObject(*typeReprOwners[i]);

        uint32_t alignedSize = alignTo(totalSize, fieldTypeRepr->alignment());
        if (alignedSize < totalSize) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_TYPEDOBJECT_TOO_BIG);
            return false;
        }

        new(fields() + i) StructField(i, ids[i], fieldTypeRepr, alignedSize);
        alignment_ = js::Max(alignment_, fieldTypeRepr->alignment());

        uint32_t incrementedSize = alignedSize + fieldTypeRepr->size();
        if (incrementedSize < alignedSize) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_TYPEDOBJECT_TOO_BIG);
            return false;
        }

        totalSize = incrementedSize;
    }

    uint32_t alignedSize = alignTo(totalSize, alignment_);
    if (alignedSize < totalSize) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_TOO_BIG);
        return false;
    }

    size_ = alignedSize;
    return true;
}

///////////////////////////////////////////////////////////////////////////
// Interning

JSObject *
TypeRepresentation::addToTableOrFree(JSContext *cx,
                                     TypeRepresentationHash::AddPtr &p)
{
    JS_ASSERT(!ownerObject_);

    JSCompartment *comp = cx->compartment();

    if (!comp->typeReprs.add(p, this)) {
        js_ReportOutOfMemory(cx);
        js_free(this); // do not finalize, not present in the table
        return nullptr;
    }

    // Now that the object is in the table, try to make the owner
    // object. If this succeeds, then the owner will remove from the
    // table once it is finalized. Otherwise, if this fails, we must
    // remove ourselves from the table ourselves and report an error.
    RootedObject ownerObject(cx,
        NewBuiltinClassInstance(cx,
                                &class_,
                                gc::GetGCObjectKind(&class_)));
    if (!ownerObject) {
        comp->typeReprs.remove(this);
        js_free(this);
        return nullptr;
    }

    ownerObject->setPrivate(this);

    // Assign the various reserved slots:
    ownerObject->initReservedSlot(JS_TYPEREPR_SLOT_KIND, Int32Value(kind()));
    ownerObject->initReservedSlot(JS_TYPEREPR_SLOT_SIZE, Int32Value(size()));
    ownerObject->initReservedSlot(JS_TYPEREPR_SLOT_ALIGNMENT, Int32Value(alignment()));

    switch (kind()) {
      case Array:
        ownerObject->initReservedSlot(JS_TYPEREPR_SLOT_LENGTH,
                                      Int32Value(asArray()->length()));
        break;

      case Scalar:
        ownerObject->initReservedSlot(JS_TYPEREPR_SLOT_TYPE,
                                      Int32Value(asScalar()->type()));
        break;

      case Float32x4:
        break;

      case Int32x4:
        break;

      case Struct:
        break;
    }

    ownerObject_.init(ownerObject);
    return &*ownerObject;
}

namespace js {
class TypeRepresentationHelper {
  public:
    template<typename T>
    static JSObject *CreateSimple(JSContext *cx, typename T::Type type) {
        JSCompartment *comp = cx->compartment();

        T sample(type);
        TypeRepresentationHash::AddPtr p = comp->typeReprs.lookupForAdd(&sample);
        if (p)
            return (*p)->ownerObject();

        // Note: cannot use cx->new_ because constructor is private.
        T *ptr = (T *) cx->malloc_(sizeof(T));
        if (!ptr)
            return nullptr;
        new(ptr) T(type);

        return ptr->addToTableOrFree(cx, p);
    }
};
} // namespace js

/*static*/
JSObject *
ScalarTypeRepresentation::Create(JSContext *cx,
                                 ScalarTypeRepresentation::Type type)
{
    return TypeRepresentationHelper::CreateSimple<ScalarTypeRepresentation>
           (cx, type);
}

/*static*/
JSObject *
Float32x4TypeRepresentation::Create(JSContext *cx)
{
        JSCompartment *comp = cx->compartment();

        Float32x4TypeRepresentation sample;
        TypeRepresentationHash::AddPtr p = comp->typeReprs.lookupForAdd(&sample);
        if (p)
            return (*p)->ownerObject();

        // Note: cannot use cx->new_ because constructor is private.
        Float32x4TypeRepresentation *ptr =
            (Float32x4TypeRepresentation *)
                cx->malloc_(sizeof(Float32x4TypeRepresentation));
        if (!ptr)
            return nullptr;
        new(ptr) Float32x4TypeRepresentation();

        return ptr->addToTableOrFree(cx, p);
}

/*static*/
JSObject *
Int32x4TypeRepresentation::Create(JSContext *cx)
{
    JSCompartment *comp = cx->compartment();

    Int32x4TypeRepresentation sample;
    TypeRepresentationHash::AddPtr p = comp->typeReprs.lookupForAdd(&sample);
    if (p)
        return (*p)->ownerObject();

    // Note: cannot use cx->new_ because constructor is private.
    Int32x4TypeRepresentation *ptr =
        (Int32x4TypeRepresentation *)
        cx->malloc_(sizeof(Int32x4TypeRepresentation));
    if (!ptr)
        return nullptr;
    new(ptr) Int32x4TypeRepresentation();

    return ptr->addToTableOrFree(cx, p);
}

/*static*/
JSObject *
ArrayTypeRepresentation::Create(JSContext *cx,
                                TypeRepresentation *element,
                                size_t length)
{
    JSCompartment *comp = cx->compartment();

    // Overly conservative, since we are using `size_t` to represent
    // size, but `SafeMul` operators on `int32_t` types. Still, it
    // should be good enough for now.
    int32_t temp;
    if (!SafeMul(element->size(), length, &temp)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_TOO_BIG);
        return nullptr;
    }

    ArrayTypeRepresentation sample(element, length);
    TypeRepresentationHash::AddPtr p = comp->typeReprs.lookupForAdd(&sample);
    if (p)
        return (*p)->ownerObject();

    // Note: cannot use cx->new_ because constructor is private.
    ArrayTypeRepresentation *ptr =
        (ArrayTypeRepresentation *) cx->malloc_(
            sizeof(ArrayTypeRepresentation));
    if (!ptr)
        return nullptr;
    new(ptr) ArrayTypeRepresentation(element, length);

    return ptr->addToTableOrFree(cx, p);
}

/*static*/
JSObject *
StructTypeRepresentation::Create(JSContext *cx,
                                 AutoIdVector &ids,
                                 AutoObjectVector &typeReprOwners)
{
    size_t count = ids.length();
    JSCompartment *comp = cx->compartment();

    // Note: cannot use cx->new_ because constructor is private.
    size_t size = sizeof(StructTypeRepresentation) + count * sizeof(StructField);
    StructTypeRepresentation *ptr =
        (StructTypeRepresentation *) cx->malloc_(size);
    new(ptr) StructTypeRepresentation();
    if (!ptr->init(cx, ids, typeReprOwners))
        return nullptr;

    TypeRepresentationHash::AddPtr p = comp->typeReprs.lookupForAdd(ptr);
    if (p) {
        js_free(ptr); // do not finalize, not present in the table
        return (*p)->ownerObject();
    }

    return ptr->addToTableOrFree(cx, p);
}

///////////////////////////////////////////////////////////////////////////
// Tracing

void
TypeRepresentation::mark(JSTracer *trace)
{
    // Push our owner object onto the mark stack. When our owner
    // object's trace callback is called, we will trace its
    // contents. This is the typical scheme for marking objects.  See
    // gc/Marking.cpp for more details.
    gc::MarkObject(trace, &ownerObject_, "typeRepresentation_ownerObject");
}

/*static*/ void
TypeRepresentation::obj_trace(JSTracer *trace, JSObject *object)
{
    fromOwnerObject(*object)->traceFields(trace);
}

void
TypeRepresentation::traceFields(JSTracer *trace)
{
    mark(trace); // don't forget to mark the self-reference here!

    switch (kind()) {
      case Scalar:
      case Float32x4:
      case Int32x4:
        break;

      case Struct:
        asStruct()->traceStructFields(trace);
        break;

      case Array:
        asArray()->traceArrayFields(trace);
        break;
    }
}

void
StructTypeRepresentation::traceStructFields(JSTracer *trace)
{
    for (size_t i = 0; i < fieldCount(); i++) {
        gc::MarkId(trace, &fields()[i].id, "typerepr_field_id");
        fields()[i].typeRepr->mark(trace);
    }
}

void
ArrayTypeRepresentation::traceArrayFields(JSTracer *trace)
{
    this->mark(trace);
    element_->mark(trace);
}

///////////////////////////////////////////////////////////////////////////
// Finalization

/*static*/ void
TypeRepresentation::obj_finalize(js::FreeOp *fop, JSObject *object)
{
    JSCompartment *comp = object->compartment();
    TypeRepresentation *typeRepr = fromOwnerObject(*object);
    comp->typeReprs.remove(typeRepr);
    js_free(typeRepr);
}

///////////////////////////////////////////////////////////////////////////
// To string

bool
TypeRepresentation::appendString(JSContext *cx, StringBuffer &contents)
{
    switch (kind()) {
      case Scalar:
        return asScalar()->appendStringScalar(cx, contents);

      case Float32x4:
        return asFloat32x4()->appendStringFloat32x4(cx, contents);

      case Int32x4:
        return asInt32x4()->appendStringInt32x4(cx, contents);

      case Array:
        return asArray()->appendStringArray(cx, contents);

      case Struct:
        return asStruct()->appendStringStruct(cx, contents);
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
    return false;
}

/*static*/ const char *
ScalarTypeRepresentation::typeName(Type type)
{
    switch (type) {
#define NUMERIC_TYPE_TO_STRING(constant_, type_, name_) \
        case constant_: return #name_;
        JS_FOR_EACH_SCALAR_TYPE_REPR(NUMERIC_TYPE_TO_STRING)
    }
    MOZ_ASSUME_UNREACHABLE("Invalid type");
}

bool
ScalarTypeRepresentation::appendStringScalar(JSContext *cx, StringBuffer &contents)
{
    switch (type()) {
#define NUMERIC_TYPE_APPEND_STRING(constant_, type_, name_)                   \
        case constant_: return contents.append(#name_);
        JS_FOR_EACH_SCALAR_TYPE_REPR(NUMERIC_TYPE_APPEND_STRING)
    }
    MOZ_ASSUME_UNREACHABLE("Invalid type");
}

bool
Float32x4TypeRepresentation::appendStringFloat32x4(JSContext *cx, StringBuffer &contents)
{
    return contents.append("float32x4");
}

bool
Int32x4TypeRepresentation::appendStringInt32x4(JSContext *cx, StringBuffer &contents)
{
    return contents.append("int32x4");
}

bool
ArrayTypeRepresentation::appendStringArray(JSContext *cx, StringBuffer &contents)
{
    if (!contents.append("ArrayType("))
        return false;

    if (!element()->appendString(cx, contents))
        return false;

    if (!contents.append(", "))
        return false;

    Value len = NumberValue(length());
    if (!contents.append(JS_ValueToString(cx, len)))
        return false;

    if (!contents.append(")"))
        return false;

    return true;
}

bool
StructTypeRepresentation::appendStringStruct(JSContext *cx, StringBuffer &contents)
{
    if (!contents.append("StructType({"))
        return false;

    for (size_t i = 0; i < fieldCount(); i++) {
        const StructField &fld = field(i);

        if (i > 0)
            contents.append(", ");

        RootedString idString(cx, IdToString(cx, fld.id));
        if (!idString)
            return false;

        if (!contents.append(idString))
            return false;

        if (!contents.append(": "))
            return false;

        if (!fld.typeRepr->appendString(cx, contents))
            return false;
    }

    if (!contents.append("})"))
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////
// Misc

const StructField *
StructTypeRepresentation::fieldNamed(jsid id) const
{
    for (size_t i = 0; i < fieldCount(); i++) {
        if (field(i).id.get() == id)
            return &field(i);
    }
    return nullptr;
}

/*static*/ bool
TypeRepresentation::isOwnerObject(JSObject &obj)
{
    return obj.getClass() == &class_;
}

/*static*/ TypeRepresentation *
TypeRepresentation::fromOwnerObject(JSObject &obj)
{
    JS_ASSERT(obj.getClass() == &class_);
    return (TypeRepresentation*) obj.getPrivate();
}


