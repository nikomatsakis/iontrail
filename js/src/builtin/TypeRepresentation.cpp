#include "jscntxt.h"
#include "jsutil.h"
#include "vm/Runtime.h"
#include "js/HashTable.h"
#include "TypeRepresentation.h"
#include "mozilla/HashFunctions.h"

#include "jsobjinlines.h"
#include "jsgcinlines.h"

using namespace js;
using namespace mozilla;

///////////////////////////////////////////////////////////////////////////
// Class def'n for the dummy object

Class TypeRepresentation::class_ = {
    "TypeRepresentation",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,         /* addProperty */
    JS_DeletePropertyStub,   /* delProperty */
    JS_PropertyStub,         /* getProperty */
    JS_StrictPropertyStub,   /* setProperty */
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    obj_finalize,
    NULL,           /* checkAccess */
    NULL,           /* call        */
    NULL,           /* hasInstance */
    NULL,           /* construct   */
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
      case TypeRepresentation::Int8:
      case TypeRepresentation::Int16:
      case TypeRepresentation::Int32:
      case TypeRepresentation::Uint8:
      case TypeRepresentation::Uint16:
      case TypeRepresentation::Uint32:
      case TypeRepresentation::Float64:
        return true;

      case TypeRepresentation::Struct:
        return matchStructs(key1->toStruct(), key2->toStruct());

      case TypeRepresentation::Array:
        return matchArrays(key1->toArray(), key2->toArray());
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
}

bool
TypeRepresentationHasher::matchStructs(StructTypeRepresentation *key1,
                                       StructTypeRepresentation *key2)
{
    if (key1->fieldCount() != key2->fieldCount())
        return false;

    for (HashNumber i = 0; i < key1->fieldCount(); i++) {
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
    return key1->element() == key2->element();
}

HashNumber
TypeRepresentationHasher::hash(TypeRepresentation *key) {
    switch (key->kind()) {
      case TypeRepresentation::Int8:
      case TypeRepresentation::Int16:
      case TypeRepresentation::Int32:
      case TypeRepresentation::Uint8:
      case TypeRepresentation::Uint16:
      case TypeRepresentation::Uint32:
      case TypeRepresentation::Float64:
        return HashGeneric(key->kind());

      case TypeRepresentation::Struct:
        return hashStruct(key->toStruct());

      case TypeRepresentation::Array:
        return hashArray(key->toArray());
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
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
    return HashGeneric(key->kind(), key->element());
}

///////////////////////////////////////////////////////////////////////////
// Constructors

TypeRepresentation::TypeRepresentation(Kind kind, uint32_t size, uint32_t align)
  : size_(size),
    align_(align),
    kind_(kind)
{}

ScalarTypeRepresentation::ScalarTypeRepresentation(Kind kind,
                                                   uint32_t size,
                                                   uint32_t align)
  : TypeRepresentation(kind, size, align)
{
}

ArrayTypeRepresentation::ArrayTypeRepresentation(TypeRepresentation *element)
  : TypeRepresentation(Array, element->size(), element->align()),
    element_(element)
{
}

static inline uint32_t alignTo(uint32_t value, uint32_t align) {
    return (value + align - 1) & -align;
}

StructTypeRepresentation::StructTypeRepresentation(JSContext *cx,
                                                   const StructFieldPair *fields,
                                                   uint32_t fieldCount)
  : TypeRepresentation(Struct, 0, 1)
{
    for (uint32_t i = 0; i < fieldCount; i++) {
        size_ = alignTo(size_, fields[i].typeRepr->align());
        fields_[i].offset = size_;
        fields_[i].id = fields[i].id;
        fields_[i].typeRepr = fields[i].typeRepr;
        align_ = js::Max(align_, fields[i].typeRepr->align());
        size_ += fields[i].typeRepr->size();
    }
    size_ = alignTo(size_, align_);
}

///////////////////////////////////////////////////////////////////////////
// Interning

void
TypeRepresentation::makePairedObject(JSContext *cx)
{
    JS_ASSERT(!pairedObject_);
    RootedObject obj(cx,
                     NewBuiltinClassInstance(cx,
                                             &class_,
                                             gc::GetGCObjectKind(&class_)));
    obj->setPrivate(this);
    pairedObject_.init(obj);
}

/*static*/
ScalarTypeRepresentation *
ScalarTypeRepresentation::New(JSContext *cx,
                              TypeRepresentation::Kind kind)
{
    JSRuntime *rt = cx->runtime();

    uint32_t size, align;
    switch (kind) {
      case Int8:
      case Uint8:
        size = align = 1;
        break;

      case Int16:
      case Uint16:
        size = align = 2;
        break;

      case Int32:
      case Uint32:
        size = align = 4;
        break;

      case Float64:
        size = align = 8;
        break;

      case Struct:
      case Array:
        MOZ_ASSUME_UNREACHABLE("Scalar type kind required");
    }

    ScalarTypeRepresentation sample(kind, size, align);
    TypeRepresentationSet::AddPtr p = rt->typeReprs.lookupForAdd(&sample);
    if (p)
        return (ScalarTypeRepresentation*) *p;

    ScalarTypeRepresentation *ptr =
        (ScalarTypeRepresentation *) cx->malloc_(
            sizeof(ScalarTypeRepresentation));
    if (!ptr)
        return NULL;
    new(ptr) ScalarTypeRepresentation(kind, size, align);
    if (!rt->typeReprs.add(p, ptr)) {
        js_ReportOutOfMemory(cx);
        js_free(ptr);
        return NULL;
    }
    ptr->makePairedObject(cx);
    return ptr;
}

/*static*/
ArrayTypeRepresentation *
ArrayTypeRepresentation::New(JSContext *cx, TypeRepresentation *element)
{
    JSRuntime *rt = cx->runtime();

    ArrayTypeRepresentation sample(element);
    TypeRepresentationSet::AddPtr p = rt->typeReprs.lookupForAdd(&sample);
    if (p)
        return (ArrayTypeRepresentation*) *p;

    ArrayTypeRepresentation *ptr =
        (ArrayTypeRepresentation *) cx->malloc_(
            sizeof(ArrayTypeRepresentation));
    if (!ptr)
        return NULL;
    new(ptr) ArrayTypeRepresentation(element);
    if (!rt->typeReprs.add(p, ptr)) {
        js_ReportOutOfMemory(cx);
        js_free(ptr);
        return NULL;
    }
    ptr->makePairedObject(cx);
    return ptr;
}

/*static*/
StructTypeRepresentation *
StructTypeRepresentation::New(JSContext *cx,
                              const StructFieldPair *fields,
                              uint32_t count)
{
    JSRuntime *rt = cx->runtime();

    size_t size = sizeof(StructTypeRepresentation) + count * sizeof(StructField);
    StructTypeRepresentation *ptr =
        (StructTypeRepresentation *) cx->malloc_(size);
    new(ptr) StructTypeRepresentation(cx, fields, count);

    TypeRepresentationSet::AddPtr p = rt->typeReprs.lookupForAdd(ptr);
    if (p) {
        js_free(ptr); // do not finalize, not present in the table
        return (StructTypeRepresentation*) *p;
    }

    if (!rt->typeReprs.add(p, ptr)) {
        js_ReportOutOfMemory(cx);
        js_free(ptr); // do not finalize, not present in the table
        return NULL;
    }
    ptr->makePairedObject(cx);
    return ptr;
}

///////////////////////////////////////////////////////////////////////////
// Tracing

void
TypeRepresentation::trace(JSTracer *trace)
{
    // Note: do not visit the fields here, just push our dummy object
    // onto the mark stack. Only when our dummy object's trace
    // callback is called do we go and trace any type respresentations
    // that we reference. This prevents us from recursing arbitrarily
    // deep during the marking phase. It also ensures that we don't
    // retrace the same type representations many times in a row.
    gc::MarkObject(trace, &pairedObject_, "typerepr_pairedObj");
}

/*static*/ void
TypeRepresentation::obj_trace(JSTracer *trace, JSObject *object)
{
    typeRepresentation(object)->traceFields(trace);
}

void
TypeRepresentation::traceFields(JSTracer *trace)
{
    switch (kind()) {
      case Int8:
      case Int16:
      case Int32:
      case Uint8:
      case Uint16:
      case Uint32:
      case Float64:
        break;

      case Struct:
        toStruct()->traceStructFields(trace);
        break;

      case Array:
        toArray()->traceArrayFields(trace);
        break;
    }
}

void
StructTypeRepresentation::traceStructFields(JSTracer *trace)
{
    for (uint32_t i = 0; i < fieldCount(); i++) {
        gc::MarkId(trace, &fields_[i].id, "typerepr_field_id");
        fields_[i].typeRepr->trace(trace);
    }
}

void
ArrayTypeRepresentation::traceArrayFields(JSTracer *trace)
{
    this->trace(trace);
    element_->trace(trace);
}

///////////////////////////////////////////////////////////////////////////
// Finalization

/*static*/ void
TypeRepresentation::obj_finalize(js::FreeOp *fop, JSObject *object)
{
    TypeRepresentation *typeRepr = typeRepresentation(object);
    fop->runtime()->typeReprs.remove(typeRepr);
    js_free(typeRepr);
}

///////////////////////////////////////////////////////////////////////////
// Misc

const StructField *
StructTypeRepresentation::fieldNamed(HandleId id) const
{
    for (uint32_t i = 0; i < fieldCount(); i++) {
        if (field(i).id.get() == id.get())
            return &field(i);
    }
    return NULL;
}

/*static*/ bool
TypeRepresentation::isTypeRepresenetation(HandleObject obj)
{
    return obj->getClass() == &class_;
}

/*static*/ TypeRepresentation *
TypeRepresentation::typeRepresentation(JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &class_);
    return (TypeRepresentation*) obj->getPrivate();
}

/*static*/ TypeRepresentation *
TypeRepresentation::typeRepresentation(HandleObject obj) {
    return typeRepresentation(obj.get());
}


