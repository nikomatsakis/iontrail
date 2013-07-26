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

class ArrayTypeRepresentation;
class StructTypeRepresentation;

class TypeRepresentation {
  public:
    enum Kind {
        Int8,
        Int16,
        Int32,
        Uint8,
        Uint16,
        Uint32,
        Float64,
        Struct,
        Array
    };

  protected:
    TypeRepresentation(Kind kind, uint32_t size, uint32_t align);

    uint32_t size_;
    uint32_t align_;
    Kind kind_;

    void makePairedObject(JSContext *cx);

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

    static bool isTypeRepresenetation(HandleObject obj);
    static TypeRepresentation *typeRepresentation(HandleObject obj);

    TypeRepresentation *of(HandleObject typeReprObj);

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
  private:
    ScalarTypeRepresentation(Kind kind, uint32_t size, uint32_t align);

  public:
    static ScalarTypeRepresentation *New(JSContext *cx, TypeRepresentation::Kind kind);
};

class ArrayTypeRepresentation : public TypeRepresentation {
  private:
    friend class TypeRepresentation;

    TypeRepresentation *element_;

    ArrayTypeRepresentation(TypeRepresentation *element);

    void traceArrayFields(JSTracer *trace);

  public:
    TypeRepresentation *element() {
        return element_;
    }

    static ArrayTypeRepresentation *New(JSContext *cx,
                                        TypeRepresentation *elementTypeRepr);
};

struct StructFieldPair {
    RootedId id;
    TypeRepresentation *typeRepr;
};

struct StructField {
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
                             const StructFieldPair *fields,
                             uint32_t fieldCount);

    void traceStructFields(JSTracer *trace);

  public:
    uint32_t fieldCount() const {
        return fieldCount_;
    }

    const StructField &field(uint32_t i) const {
        JS_ASSERT(i < fieldCount());
        return fields_[i];
    }

    const StructField *fieldNamed(HandleId id) const;

    static StructTypeRepresentation *New(JSContext *cx,
                                         const StructFieldPair *fields,
                                         uint32_t count);
};

struct TypeRepresentationHasher
{
    typedef TypeRepresentation *Lookup;
    static HashNumber hash(TypeRepresentation *key);
    static bool match(TypeRepresentation *key1, TypeRepresentation *key2);

  private:
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
