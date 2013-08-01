#include "jscntxt.h"
#include "jsutil.h"
#include "vm/Runtime.h"
#include "vm/StringBuffer.h"
#include "js/HashTable.h"
#include "TypeRepresentation.h"
#include "mozilla/HashFunctions.h"

#include "jsobjinlines.h"
#include "jsgcinlines.h"

using namespace js;
using namespace mozilla;

typedef float float32_t;
typedef double float64_t;

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
      case TypeRepresentation::Scalar:
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
    return key1->element() == key2->element() &&
           key1->length() == key2->length();
}

HashNumber
TypeRepresentationHasher::hash(TypeRepresentation *key) {
    switch (key->kind()) {
      case TypeRepresentation::Scalar:
        return hashScalar(key->toScalar());

      case TypeRepresentation::Struct:
        return hashStruct(key->toStruct());

      case TypeRepresentation::Array:
        return hashArray(key->toArray());
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
}

HashNumber
TypeRepresentationHasher::hashScalar(ScalarTypeRepresentation *key)
{
    return HashGeneric(key->kind(), key->type());
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

TypeRepresentation::TypeRepresentation(Kind kind, uint32_t size, uint32_t align)
  : size_(size),
    align_(align),
    kind_(kind)
{}

ScalarTypeRepresentation::ScalarTypeRepresentation(Type type)
  : TypeRepresentation(Scalar, 0, 1),
    type_(type)
{
    switch (type) {
      case Int8:
      case Uint8:
        size_ = align_ = 1;
        break;

      case Int16:
      case Uint16:
        size_ = align_ = 2;
        break;

      case Int32:
      case Uint32:
      case Float32:
        size_ = align_ = 4;
        break;

      case Float64:
        size_ = align_ = 8;
        break;
    }
}

ArrayTypeRepresentation::ArrayTypeRepresentation(TypeRepresentation *element,
                                                 uint32_t length)
  : TypeRepresentation(Array, element->size() * length, element->align()),
    element_(element),
    length_(length)
{
}

static inline uint32_t alignTo(uint32_t value, uint32_t align) {
    return (value + align - 1) & -align;
}

StructTypeRepresentation::StructTypeRepresentation(JSContext *cx,
                                                   AutoIdVector &ids,
                                                   AutoObjectVector &typeReprObjs)
  : TypeRepresentation(Struct, 0, 1)
{
    JS_ASSERT(ids.length() == typeReprObjs.length());
    fieldCount_ = ids.length();
    for (uint32_t i = 0; i < ids.length(); i++) {
        TypeRepresentation *fieldTypeRepr = typeRepresentation(typeReprObjs[i]);
        size_ = alignTo(size_, fieldTypeRepr->align());
        fields_[i].index = i;
        fields_[i].offset = size_;
        fields_[i].id.init(ids[i]);
        fields_[i].typeRepr = fieldTypeRepr;
        align_ = js::Max(align_, fieldTypeRepr->align());
        size_ += fieldTypeRepr->size();
    }
    size_ = alignTo(size_, align_);
}

///////////////////////////////////////////////////////////////////////////
// Interning

JSObject *
TypeRepresentation::makePairedObject(JSContext *cx)
{
    JS_ASSERT(!pairedObject_);
    RootedObject obj(cx,
                     NewBuiltinClassInstance(cx,
                                             &class_,
                                             gc::GetGCObjectKind(&class_)));
    obj->setPrivate(this);
    pairedObject_.init(obj);
    return obj.get();
}

/*static*/
JSObject *
ScalarTypeRepresentation::New(JSContext *cx,
                              ScalarTypeRepresentation::Type type)
{
    JSRuntime *rt = cx->runtime();

    ScalarTypeRepresentation sample(type);
    TypeRepresentationSet::AddPtr p = rt->typeReprs.lookupForAdd(&sample);
    if (p)
        return (*p)->object();

    ScalarTypeRepresentation *ptr =
        (ScalarTypeRepresentation *) cx->malloc_(
            sizeof(ScalarTypeRepresentation));
    if (!ptr)
        return NULL;
    new(ptr) ScalarTypeRepresentation(type);
    if (!rt->typeReprs.add(p, ptr)) {
        js_ReportOutOfMemory(cx);
        js_free(ptr);
        return NULL;
    }
    return ptr->makePairedObject(cx);
}

/*static*/
JSObject *
ArrayTypeRepresentation::New(JSContext *cx,
                             TypeRepresentation *element,
                             uint32_t length)
{
    JSRuntime *rt = cx->runtime();

    ArrayTypeRepresentation sample(element, length);
    TypeRepresentationSet::AddPtr p = rt->typeReprs.lookupForAdd(&sample);
    if (p)
        return (*p)->object();

    ArrayTypeRepresentation *ptr =
        (ArrayTypeRepresentation *) cx->malloc_(
            sizeof(ArrayTypeRepresentation));
    if (!ptr)
        return NULL;
    new(ptr) ArrayTypeRepresentation(element, length);
    if (!rt->typeReprs.add(p, ptr)) {
        js_ReportOutOfMemory(cx);
        js_free(ptr);
        return NULL;
    }
    return ptr->makePairedObject(cx);
}

/*static*/
JSObject *
StructTypeRepresentation::New(JSContext *cx,
                              AutoIdVector &ids,
                              AutoObjectVector &typeReprObjs)
{
    uint32_t count = ids.length();
    JSRuntime *rt = cx->runtime();

    size_t size = sizeof(StructTypeRepresentation) + count * sizeof(StructField);
    StructTypeRepresentation *ptr =
        (StructTypeRepresentation *) cx->malloc_(size);
    new(ptr) StructTypeRepresentation(cx, ids, typeReprObjs);

    TypeRepresentationSet::AddPtr p = rt->typeReprs.lookupForAdd(ptr);
    if (p) {
        js_free(ptr); // do not finalize, not present in the table
        return (*p)->object();
    }

    if (!rt->typeReprs.add(p, ptr)) {
        js_ReportOutOfMemory(cx);
        js_free(ptr); // do not finalize, not present in the table
        return NULL;
    }
    return ptr->makePairedObject(cx);
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
      case Scalar:
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
// To string

bool
TypeRepresentation::appendString(JSContext *cx, StringBuffer &contents)
{
    switch (kind()) {
      case Scalar:
        return toScalar()->appendStringScalar(cx, contents);

      case Array:
        return toArray()->appendStringArray(cx, contents);

      case Struct:
        return toStruct()->appendStringStruct(cx, contents);
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
    return false;
}

/*static*/ const char *
ScalarTypeRepresentation::typeName(Type type)
{
    switch (type) {
#define NUMERIC_TYPE_TO_STRING(constant_, type_) case constant_: return #type_;
        JS_FOR_EACH_SCALAR_TYPE_REPR(NUMERIC_TYPE_TO_STRING)
    }
    MOZ_ASSUME_UNREACHABLE("Invalid type");
}

bool
ScalarTypeRepresentation::appendStringScalar(JSContext *cx, StringBuffer &contents)
{
    switch (type()) {
#define NUMERIC_TYPE_APPEND_STRING(constant_, type_) \
        case constant_: return contents.append(#type_);
        JS_FOR_EACH_SCALAR_TYPE_REPR(NUMERIC_TYPE_APPEND_STRING)
    }
    MOZ_ASSUME_UNREACHABLE("Invalid type");
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

    for (uint32_t i = 0; i < fieldCount(); i++) {
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
StructTypeRepresentation::fieldNamed(HandleId id) const
{
    for (uint32_t i = 0; i < fieldCount(); i++) {
        if (field(i).id.get() == id.get())
            return &field(i);
    }
    return NULL;
}

/*static*/ bool
TypeRepresentation::isTypeRepresentation(HandleObject obj)
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
TypeRepresentation::of(HandleObject obj) {
    return typeRepresentation(obj.get());
}


