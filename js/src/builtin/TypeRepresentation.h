#ifndef builtin_TypeRepresentation_h
#define builtin_TypeRepresentation_h

/*

  Type Representations are the way that the compiler stores
  information about binary data types that the user defines.  They are
  always interned into a hashset in the runtime (typeReprs), meaning
  that you can compare two type representations for equality just
  using `==`.

  # Creation and canonicalization:

  Each kind of `TypeRepresentation` object includes a `New` method
  that will create a canonical instance of it. So, for example, you
  can do `ScalarTypeRepresentation::New(Uint8)` to get the canonical
  representation of uint8. The object that is returned is designed to
  be immutable, and the API permits only read access.

  # Memory management:

  TypeRepresentations are managed by the GC. To avoid adding a new
  kind of finalize kind, each TypeRepresentations is paired with a
  dummy JSObject. If you reference a TypeRepresentation*, you must
  ensure this dummy object is traced. In heap objects, this is
  typically done by invoking TypeRepresentation::trace, though another
  option is just to root the dummy object yourself.

  The canonicalization table maintains *weak references* to the
  TypeRepresentation* pointers. That is, the table is not traced.
  Instead, whenever an object is created, it is paired with its dummy
  object, and the finalizer of the dummy object removes the pointer
  from the table and then frees the pointer.

 */

#include "js/HashTable.h"
#include "jspubtd.h"
#include "jsalloc.h"

namespace js {

class ScalarTypeRepresentation;
class ArrayTypeRepresentation;
class StructTypeRepresentation;

class TypeRepresentation {
  public:
    enum Kind {
        Scalar,
        Struct,
        Array
    };

  protected:
    TypeRepresentation(Kind kind, uint32_t size, uint32_t align);

    uint32_t size_;
    uint32_t align_;
    Kind kind_;

    JSObject *makePairedObject(JSContext *cx);

    static TypeRepresentation *typeRepresentation(JSObject *obj);

  private:
    static Class class_;
    static void obj_trace(JSTracer *trace, JSObject *object);
    static void obj_finalize(js::FreeOp *fop, JSObject *object);

    HeapPtrObject pairedObject_;
    void traceFields(JSTracer *tracer);

  public:
    uint32_t size() const { return size_; }
    uint32_t align() const { return size_; }
    Kind kind() const { return kind_; }
    JSObject *object() const { return pairedObject_.get(); }

    bool convertAndCopyTo(JSContext *cx, HandleValue from, uint8_t *mem);

    bool appendString(JSContext *cx, StringBuffer &buffer);

    static bool isTypeRepresentation(HandleObject obj);
    static TypeRepresentation *of(HandleObject obj);

    bool isScalar() {
        return kind() == Scalar;
    }

    ScalarTypeRepresentation *toScalar() {
        JS_ASSERT(isScalar());
        return (ScalarTypeRepresentation*) this;
    }

    bool isArray() {
        return kind() == Array;
    }

    ArrayTypeRepresentation *toArray() {
        JS_ASSERT(isArray());
        return (ArrayTypeRepresentation*) this;
    }

    bool isStruct() {
        return kind() == Struct;
    }

    StructTypeRepresentation *toStruct() {
        JS_ASSERT(isStruct());
        return (StructTypeRepresentation*) this;
    }

    void trace(JSTracer *tracer);
};

class ScalarTypeRepresentation : public TypeRepresentation {
  public:
    enum Type {
        Int8,
        Int16,
        Int32,
        Uint8,
        Uint16,
        Uint32,
        Float32,
        Float64
    };
    static const uint32_t NumTypes = Float64 + 1;

  private:
    friend class TypeRepresentation;

    Type type_;

    ScalarTypeRepresentation(Type type);

    bool appendStringScalar(JSContext *cx, StringBuffer &buffer);

    bool convertAndCopyToScalar(JSContext *cx, HandleValue from, uint8_t *mem);

  public:
    Type type() const {
        return type_;
    }

    bool convertValue(JSContext *cx, HandleValue value, MutableHandleValue vp);

    static const char *typeName(Type type);
    static JSObject *New(JSContext *cx, Type type);
};

#define JS_FOR_EACH_SCALAR_TYPE_REPR(macro_)                                  \
    macro_(ScalarTypeRepresentation::Int8,     int8)                          \
    macro_(ScalarTypeRepresentation::Int16,    int16)                         \
    macro_(ScalarTypeRepresentation::Int32,    int32)                         \
    macro_(ScalarTypeRepresentation::Uint8,    uint8)                         \
    macro_(ScalarTypeRepresentation::Uint16,   uint16)                        \
    macro_(ScalarTypeRepresentation::Uint32,   uint32)                        \
    macro_(ScalarTypeRepresentation::Float32,  float32)                       \
    macro_(ScalarTypeRepresentation::Float64,  float64)

class ArrayTypeRepresentation : public TypeRepresentation {
  private:
    friend class TypeRepresentation;

    TypeRepresentation *element_;
    uint32_t length_;

    ArrayTypeRepresentation(TypeRepresentation *element,
                            uint32_t length);

    void traceArrayFields(JSTracer *trace);

    bool appendStringArray(JSContext *cx, StringBuffer &buffer);

    bool convertAndCopyToArray(JSContext *cx, HandleValue from, uint8_t *mem);

  public:
    TypeRepresentation *element() {
        return element_;
    }

    uint32_t length() {
        return length_;
    }

    static JSObject *New(JSContext *cx,
                         TypeRepresentation *elementTypeRepr,
                         uint32_t length);
};

struct StructFieldPair {
    RootedId id;
    TypeRepresentation *typeRepr;
};

struct StructField {
    uint32_t index;
    HeapId id;
    TypeRepresentation *typeRepr;
    uint32_t offset;
};

class StructTypeRepresentation : public TypeRepresentation {
  private:
    friend class TypeRepresentation;

    uint32_t fieldCount_;

    // The StructTypeRepresentation is allocated with extra space to
    // store the contents of this array.
    StructField fields_[];

    StructTypeRepresentation(JSContext *cx,
                             AutoIdVector &ids,
                             AutoObjectVector &typeReprObjs);

    void traceStructFields(JSTracer *trace);

    bool appendStringStruct(JSContext *cx, StringBuffer &buffer);

    bool convertAndCopyToStruct(JSContext *cx, HandleValue from, uint8_t *mem);

  public:
    uint32_t fieldCount() const {
        return fieldCount_;
    }

    const StructField &field(uint32_t i) const {
        JS_ASSERT(i < fieldCount());
        return fields_[i];
    }

    const StructField *fieldNamed(HandleId id) const;

    static JSObject *New(JSContext *cx,
                         AutoIdVector &ids,
                         AutoObjectVector &typeReprObjs);
};

struct TypeRepresentationHasher
{
    typedef TypeRepresentation *Lookup;
    static HashNumber hash(TypeRepresentation *key);
    static bool match(TypeRepresentation *key1, TypeRepresentation *key2);

  private:
    static HashNumber hashScalar(ScalarTypeRepresentation *key);
    static HashNumber hashStruct(StructTypeRepresentation *key);
    static HashNumber hashArray(ArrayTypeRepresentation *key);

    static bool matchStructs(StructTypeRepresentation *key1,
                             StructTypeRepresentation *key2);
    static bool matchArrays(ArrayTypeRepresentation *key1,
                            ArrayTypeRepresentation *key2);
};

typedef js::HashSet<TypeRepresentation *,
                    TypeRepresentationHasher,
                    SystemAllocPolicy> TypeRepresentationSet;

} // namespace js

#endif
