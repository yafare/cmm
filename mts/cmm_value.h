#pragma once

#include "std_port/std_port.h"
#include "std_port/std_port_type.h"
#include "std_port/std_port_compiler.h"
#include "std_template/simple_string.h"
#include "std_template/simple_vector.h"
#include "std_template/simple_hash_map.h"
#include "cmm.h"

namespace cmm
{

// Type of values
typedef Int64   Integer;
typedef Int64   IntPtr;
typedef double  Real;

class Domain;
class Object;
class Thread;

class Value;
class String;
class Buffer;
class Array;
class Map;

class ValueList;

struct ArrayImpl;
struct BufferImpl;
struct FunctionPtrImpl;
struct MapImpl;
struct StringImpl;
struct MarkValueState;

// Base type of VM referenced  value
struct ReferenceImpl
{
friend Value;

public:
    typedef enum
    {
        CONSTANT = 0x01,    // Unchanged/freed Referenced value
        SHARED = 0x80,      // Shared in a values pool
    } Attrib;

public:
	ReferenceImpl() :
        attrib(0),
        hash_cache(0),
        owner(0),
        offset(0),
        next(0)
	{
	}

    virtual ~ReferenceImpl()
    {
        // Let derived class do destroy work
    }

public:
    // Is this value a constant value?
    bool is_constant() const
    {
        return attrib & CONSTANT;
    }

public:
    // Append this reference value @ tail of the list
    void append(ReferenceImpl **pp_list_tail)
    {
        *pp_list_tail = this;
    }

    // Bind the reference value to a domain
    void bind_to_current_domain();

    // Return hash value of me
    size_t hash_value() const
    {
        if (!hash_cache)
            hash_cache = (Uint) hash_this() + 1;

        return hash_cache;
    }

    // This value will be freed manually, unbind it if necessary
    void unbind();

public:
    // Copy to local value list
    virtual ReferenceImpl *copy_to_local(Thread *thread) = 0;

    // Return type of this referneced value
    virtual ValueType get_type() const = 0;

    // Hash this value (hash this pointer)
    virtual size_t hash_this() const { return ((size_t)this) / sizeof(void *); }

    // Mark-sweep
    virtual void mark(MarkValueState& value_map) { }

public:
    Uint attrib;             // 32bits only (Don't use enum)
    mutable Uint hash_cache; // Cache of hash value
    ValueList *owner;        // Owned by domain or thread
    size_t offset;           // Offset in owner value list
    ReferenceImpl *next;     // Next value in list
};

// Enum to values
enum ConstantValue
{
    UNDEFINED = 0,
};

// Constant value
extern StringImpl* EMPTY_STRING;
extern BufferImpl* EMPTY_BUFFER;

// VM value
// ATTENTION:
// This structure shouldn't has any virtual functions, either destuctor.
// We can simple use memcpy to duplicate this class (with updating reference
// correctly manually)
class Value
{
friend Object;
friend String;
friend Array;
friend Map;
template <typename T>
friend class TypedValue;

public:
    // Instantiated hash routine for simple::string
    struct hash_func
    {
        size_t operator()(const Value& value) const
        {
            return value.hash_value();
        }
    };

public:
    static bool init();
    static void shutdown();

private:
    // ATTENTION:
    // WHY Value() is private
    // This is to prevent declare this value as member in other container such
    // as vector<>, map<> etc...
    Value()
    {
        m_type = NIL;
#ifdef _DEBUG
        m_intptr = 0;
#endif
    }

public:
    // ATTENTION:
    // Don't add destructor, or it may cause the poor performance.
    // We will use this class in function freqencily and the destructor
    // will generated many Exception Handling relative code.

    // Raw constructor (Do nothing, just copy)
    Value(IntPtr intptr, ValueType type)
    {
        m_type = type;
        m_intptr = intptr;
    }

    // Create value from constant
    Value(ConstantValue value)
    {
        // UNDEFINED only now
        m_type = NIL;
        m_intptr = 0;
    }

    Value(const Value& value)
    {
        *this = value;
    }

    // Construct for integer 
    Value(int v)
    {
        m_type = INTEGER;
        m_int = (Integer)v;
    }

    Value(Int64 v)
    {
        m_type = INTEGER;
        m_int = (Integer)v;
    }

    Value(size_t v)
    {
        m_type = INTEGER;
        m_int = (Integer)v;
    }

    // Construct from real
    Value(Real v)
    {
        m_type = REAL;
        m_real = v;
    }

    // Construct from object
    Value(ObjectId oid)
    {
        m_type = OBJECT;
        m_oid = oid;
    }

    Value(Object* ob);

    // Contruct from reference values
    // The ReferenceImpl should be allocated without been binded
    Value(ReferenceImpl* v, ValueType type = NIL)
    {
        if (type == NIL)
            type = v->get_type();
        m_type = type;
        m_reference = v;
        // Bind if the reference value has not owner yet
        m_reference->bind_to_current_domain();
    }

    Value(StringImpl* const v) :
        Value((ReferenceImpl*)v, STRING) {}

    Value(BufferImpl* v) :
        Value((ReferenceImpl*)v, BUFFER) {}

    Value(FunctionPtrImpl* v) :
        Value((ReferenceImpl*)v, FUNCTION) {}

    Value(ArrayImpl* v) :
        Value((ReferenceImpl*)v, ARRAY) {}

    Value(MapImpl* v) :
        Value((ReferenceImpl*)v, MAPPING) {}

    // Construct for string
    Value(const char *c_str, size_t len = SIZE_MAX);
	Value(const simple::string& str);

    // Safe get values (check type)
public:
    // Return this value if they're expected type
    const Value& as_nil() const { return expect_type(NIL); }
    const Value& as_int() const { return expect_type(INTEGER); }
    const Value& as_real() const { return expect_type(REAL); }
    const Value& as_object() const { return expect_type(OBJECT); }
    const Value& as_string() const { return expect_type(STRING); }
    const Value& as_buffer() const { return expect_type(BUFFER); }
    const Value& as_function() const { return expect_type(FUNCTION); }
    const Value& as_array() const { return expect_type(ARRAY); }
    const Value& as_map() const { return expect_type(MAPPING); }

    Value& as_nil() { return expect_type(NIL); }
    Value& as_int() { return expect_type(INTEGER); }
    Value& as_real() { return expect_type(REAL); }
    Value& as_object() { return expect_type(OBJECT); }
    Value& as_string() { return expect_type(STRING); }
    Value& as_buffer() { return expect_type(BUFFER); }
    Value& as_function() { return expect_type(FUNCTION); }
    Value& as_array() { return expect_type(ARRAY); }
    Value& as_map() { return expect_type(MAPPING); }

    Integer          get_int() const { return as_int().m_int; }
    Real             get_real() const { return as_real().m_real; }
    ObjectId         get_object() const { return as_object().m_oid; }
    StringImpl      *get_string() const { return as_string().m_string; }
    BufferImpl      *get_buffer() const { return as_buffer().m_buffer; }
    FunctionPtrImpl *get_function() const { return as_function().m_function; }
    ArrayImpl       *get_array() const { return as_array().m_array; }
    MapImpl         *get_map() const { return as_map().m_map; }

    // Operation of value
public:
    bool             cast_bool() const;
    Integer          cast_int() const;
    Real             cast_real() const;
    StringImpl*      cast_string() const;
    BufferImpl*      cast_buffer() const;
    ArrayImpl*       cast_array() const;
    MapImpl*         cast_map() const;

public:
    // Type to name
    static const char *type_to_name(ValueType type)
    {
        switch (type)
        {
        case NIL:      return "nil";
        case INTEGER:  return "int";
        case REAL:     return "real";
        case OBJECT:   return "object";
        case STRING:   return "string";
        case BUFFER:   return "buffer";
        case ARRAY:    return "array";
        case MAPPING:  return "mapping";
        case FUNCTION: return "function";
        case MIXED:    return "mixed";
        default:       return "bad type";
        }
    }

    // Name to type
    static ValueType name_to_type(const char *name)
    {
        switch (name[0])
        {
            case 'n': return (strcmp(name, "nil") == 0)      ? NIL      : BAD_TYPE;
            case 'i': return (strcmp(name, "int") == 0)      ? INTEGER  : BAD_TYPE;
            case 'r': return (strcmp(name, "real") == 0)     ? REAL     : BAD_TYPE;
            case 'o': return (strcmp(name, "object") == 0)   ? OBJECT   : BAD_TYPE;
            case 's': return (strcmp(name, "string") == 0)   ? STRING   : BAD_TYPE;
            case 'b': return (strcmp(name, "buffer") == 0)   ? BUFFER   : BAD_TYPE;
            case 'f': return (strcmp(name, "function") == 0) ? FUNCTION : BAD_TYPE;
            case 'a': return (strcmp(name, "array") == 0)    ? ARRAY    : BAD_TYPE;
            case 'm': return (strcmp(name, "mapping") == 0)  ? MAPPING  :
                             (strcmp(name, "mixed") == 0)    ? MIXED    : BAD_TYPE;
            case 'v': return (strcmp(name, "void") == 0)     ? TVOID    : BAD_TYPE;
            default:  return BAD_TYPE;
        }
    }

    // Calculate & cache hash number of the value
    size_t hash_value() const;

	// Is this reference value?
	bool is_reference_value() const
	{
		return m_type >= REFERENCE_VALUE;
	}

    // Is this value zero?
    bool is_zero() const
    {
        return m_intptr == 0;
    }

    // Is this value non zero?
    bool is_non_zero() const
    {
        return !is_zero();
    }

public:////----private:
    // Check type & return this value
    const Value& expect_type(ValueType type) const
    {
        if (m_type != type)
            throw_error("Value type is not %s.\n", type_to_name(type));
        return *this;
    }

    Value& expect_type(ValueType type)
    {
        if (m_type != type)
            throw_error("Value type is not %s.\n", type_to_name(type));
        return (Value &)*this;
    }

    // Domain/local relative operations
public:
    // Bind this value to specified domain
    Value& bind_to(Domain *domain);

    // Copy this value to local value list if this is a reference type value
    // After copy, caller should enter another domain & transfer these values
    // to it
    Value copy_to_local(Thread *thread) const
    {
        if (m_type < REFERENCE_VALUE)
            return *this;

        // Copy reference value to a new value
        return Value((IntPtr)m_reference->copy_to_local(thread), m_type);
    }

public:
    Value& operator =(const Value& value)
    {
        m_type = value.m_type;
        m_intptr = value.m_intptr;
        return *this;
    }

    // Put value to container
    Value& operator [](const Value& value);

    // Get index from container
    const Value operator [](const Value& value) const;

public:
    ValueType m_type;
    union
    {
        Integer          m_int;
        Real             m_real;
        IntPtr           m_intptr;
        ObjectId         m_oid;
        ReferenceImpl   *m_reference;
        StringImpl      *m_string;
        BufferImpl      *m_buffer;
        FunctionPtrImpl *m_function;
        ArrayImpl       *m_array;
        MapImpl         *m_map;
    };
};

// Macro as functions
#define STRING_ALLOC(...)   StringImpl::alloc(__FILE__, __LINE__, ##__VA_ARGS__)
#define STRING_FREE(string) StringImpl::free(__FILE__, __LINE__, string)

// VM value: string
struct StringImpl : public ReferenceImpl
{
public:
    typedef simple::string::string_hash_t string_hash_t;
    typedef simple::string::string_size_t string_size_t;
    static const ValueType this_type = ValueType::STRING;

    // Instantiated hash routine for simple::string
    struct hash_func
    {
        size_t operator()(const StringImpl* impl) const
        {
            return impl->hash_value();
        }
    };

public:////----private:
    // The string can be constructed by StringImpl::new_string() only,
    // since the string data should be allocated at same time. That
    // means the operation: new StringImpl(), StringImpl xxx is invalid. We
    // must use StringImpl * to access it.
    StringImpl(size_t size)
    {
        len = (string_size_t)size;
    }

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() const { return this_type; }
    virtual size_t hash_this() const { return simple::string::hash_string(buf); }

public:
    size_t length() const { return len; }
    char_t *ptr() { return buf; }
    const char_t *ptr() const { return buf; }
    const char *c_str() const { return (const char *)buf; }

public:
    // Allocate a string with reserved size
    static StringImpl *alloc(const char *file, int line, size_t size = 0);

    // Allocate a string & construct it
    static StringImpl *alloc(const char *file, int line, const char *c_str, size_t len = SIZE_MAX);
    static StringImpl *alloc(const char *file, int line, const simple::string& str);
    static StringImpl *alloc(const char *file, int line, const StringImpl *other);

    // Free a string
    static void free(const char *file, int line, StringImpl *string);

    // Compare two strings
    static int compare(const StringImpl *a, const StringImpl *b);
    static int compare(const StringImpl *a, const char *c_str);

public:
    // Concat with other
    StringImpl *concat(const StringImpl *other) const;

    // Get sub of me
    StringImpl *sub_of(size_t offset, size_t len = SIZE_MAX) const;

    // Generate string by snprintf
    static StringImpl *snprintf(const char *fmt, size_t n, ...);

public:
    inline int index_val(Integer index) const
    {
        if ((size_t)index > length())
            throw_error("Index of string is out of range.\n");
        return (int)(uchar_t)buf[index];
    }

    inline int index_val(const Value& index) const
    {
        return index_val(index.get_int());
    }

    inline bool operator <(const StringImpl& b) const
    {
        return StringImpl::compare(this, &b) < 0;
    }

    inline bool operator ==(const StringImpl& b) const
    {
        return StringImpl::compare(this, &b) == 0;
    }

    inline bool operator ==(const char *c_str) const
    {
        return StringImpl::compare(this, c_str) == 0;
    }

    inline bool operator !=(const char *c_str) const
    {
        return StringImpl::compare(this, c_str) != 0;
    }

public:////----private:
    string_size_t len;
    char_t buf[1]; // For count size of terminator '\x0'
};

// Macro as functions
#define BUFFER_ALLOC(...)   BufferImpl::alloc(__FILE__, __LINE__, ##__VA_ARGS__)
#define BUFFER_FREE(buffer) BufferImpl::free(__FILE__, __LINE__, buffer)

// VM value: buffer
STD_BEGIN_ALIGNED_STRUCT(STD_BEST_ALIGN_SIZE)
struct BufferImpl : public ReferenceImpl
{
public:
    static const ValueType this_type = ValueType::BUFFER;
    typedef enum
    {
        // The buffer contains 1 or N class
        CONTAIN_1_CLASS = 0x0001,
        CONTAIN_N_CLASS = 0x0002,
        CONTAIN_CLASS = CONTAIN_1_CLASS | CONTAIN_N_CLASS,
    } Attrib;

    enum
    {
        CLASS_1_STAMP = 0x19770531,
        CLASS_N_STAMP = 0x19801126
    };

    // Head structure for buffer to store class[] 
    STD_BEGIN_ALIGNED_STRUCT(STD_BEST_ALIGN_SIZE)
    struct ArrInfo
    {
        Uint32 n;
        Uint32 size;
        Uint32 stamp;
    }
    STD_END_ALIGNED_STRUCT(STD_BEST_ALIGN_SIZE);
    static const int RESERVE_FOR_CLASS_ARR = sizeof(ArrInfo);

    // Constructor (copy) function entry
    typedef void (*ConstructFunc)(void *ptr_class, const void *from_class);

    // Destructor function entry
    typedef void (*DestructFunc)(void *ptr_class);

public:////----private:
    BufferImpl(size_t _len)
    {
        buffer_attrib = (Attrib)0;
        constructor = 0;
        destructor = 0;
        len = _len;
    }

    virtual ~BufferImpl();

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() const { return this_type; }
    virtual size_t hash_this() const;
    virtual void mark(MarkValueState& value_map);

public:
    // Get length of me
    size_t length() const { return len; }

    // Get raw data pointer
    Uint8 *data() const { return (Uint8 *) (this + 1); }

    // Get single or class array pointer
    void *class_ptr(size_t index = 0) const
    {
        STD_ASSERT(("Buffer doesn't contain any class.", (buffer_attrib & CONTAIN_CLASS)));
        auto info = (ArrInfo *)(this + 1);
        STD_ASSERT(("Index out of range of class_ptr.", index < info->n));
        return data() + RESERVE_FOR_CLASS_ARR + index * info->size;
        throw_error("This buffer doesn't not contain any class.\n");
    }

public:
    // Concat with other
    BufferImpl *concat(const BufferImpl *other) const;

    // Get sub of me
    BufferImpl *sub_of(size_t offset, size_t len = SIZE_MAX) const;

public:
    inline int index_val(Integer index) const
    {
        if ((size_t)index > length())
            throw_error("Index of string is out of range.\n");
        return (int)data()[index];
    }

    inline int index_val(const Value& index) const
    {
        return index_val(index.get_int());
    }

public:
    // Allocate a string with reserved size
    static BufferImpl *alloc(const char *file, int line, size_t size);

    // Allocate a string & construct it
    static BufferImpl *alloc(const char *file, int line, const void *p, size_t len);
    static BufferImpl *alloc(const char *file, int line, const BufferImpl *other);

    // Free a buffer
    static void free(const char *file, int line, BufferImpl *buffer);

public:
    // Compare two buffers
    static int compare(const BufferImpl *a, const BufferImpl *b);

public:
    Attrib buffer_attrib;
    ConstructFunc constructor;
    DestructFunc destructor;
    size_t len;
}
STD_END_ALIGNED_STRUCT(STD_BEST_ALIGN_SIZE);

// VM value: function
struct FunctionPtrImpl : public ReferenceImpl
{
public:
    static const ValueType this_type = ValueType::FUNCTION;

public:
    FunctionPtrImpl()
    {
    }

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() const { return this_type; }
    virtual void mark(MarkValueState& value_map);
};

// Define Value class for container use only
class ValueInContainer : public Value
{
public:
    ValueInContainer() :
        Value(UNDEFINED)
    {
    }
};

// VM value: array
struct ArrayImpl : public ReferenceImpl
{
public:
    typedef simple::unsafe_vector<ValueInContainer> DataType;
    static const ValueType this_type = ValueType::ARRAY;

public:
    ArrayImpl(size_t size_hint = 8) :
        a(size_hint)
    {
    }

    ArrayImpl(const DataType& data)
    {
        a = data;
    }

    ArrayImpl(DataType&& data)
    {
        a = data;
    }

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() const { return this_type; }
    virtual void mark(MarkValueState& value_map);

public:
    // Concat with other
    ArrayImpl *concat(const ArrayImpl *other) const;

    // Get sub of me
    ArrayImpl *sub_of(size_t offset, size_t len = SIZE_MAX) const;

public:
    Value& index_ptr(Integer index) const
    {
        if (index < 0 || index >= (Integer)a.size())
            throw_error("Index out of range to array, got %lld.\n", (Int64)index);

        if (sizeof(index) > sizeof(size_t))
        {
            // m_int is longer than size_t
            if (index > SIZE_MAX)
                throw_error("Index out of range to array, got %lld.", (Int64)index);
        }

        return a[index];
    }

    Value& index_ptr(const Value& index) const
    {
        return index_ptr(index.get_int());
    }

    const Value index_val(Integer index) const
    {
        return index_ptr(index);
    }

    const Value index_val(const Value& index) const
    {
        return index_ptr(index.get_int());
    }

    // Append an element
    void push_back(const Value& value)
    {
        a.push_back((ValueInContainer&)value);
    }

    // Append an element
    void push_back_array(const Value* value_arr, size_t count)
    {
        a.push_back_array((const ValueInContainer*)value_arr, count);
    }

    // Get size
    size_t size() const { return a.size(); }

public:
    DataType a;
};

// VM value: map
struct MapImpl : public ReferenceImpl
{
public:
    static const ValueType this_type = ValueType::MAPPING;

public:
    typedef simple::hash_map<ValueInContainer, ValueInContainer, Value::hash_func> DataType;

public:
    MapImpl(size_t size_hint = 4) :
        m(size_hint)
    {
    }

    MapImpl(const DataType& data)
    {
        m = data;
    }

    MapImpl(DataType&& data)
    {
        m = data;
    }

public:
    virtual ReferenceImpl *copy_to_local(Thread *thread);
    virtual ValueType get_type() const { return this_type; }
    virtual void mark(MarkValueState& value_map);

public:
    // Concat with other
    MapImpl *concat(const MapImpl *other) const;

public:
    Value& index_ptr(const Value& index)
    {
        return m[(const ValueInContainer&)index];
    }

    const Value index_val(const Value& index) const
    {
        ValueInContainer value;
        m.try_get((const ValueInContainer&)index, &value);
        return value;
    }

    void put(const Value& key, const Value& value)
    {
        index_ptr(key) = value;
    }

    // Contains key?
    bool contains_key(const Value& key) const
    {
        return m.contains_key((const ValueInContainer&)key);
    }

    // Get keys
    Value keys() const
    {
        auto vec = m.keys();
        return XNEW(ArrayImpl, (ArrayImpl::DataType&&)simple::move(vec));
    }

    // Get size
    size_t size() const { return m.size(); }

    // Get values
    Value values() const
    {
        auto vec = m.values();
        return XNEW(ArrayImpl, (ArrayImpl::DataType&&)simple::move(vec));
    }

public:
    DataType m;
};

template <typename T>
class TypedValue : public Value
{
public:
    // Other constructor
    template <typename... Types>
    TypedValue(Types&&... args) :
        Value(UNDEFINED)
    {
        Value value(simple::forward<Types>(args)...);
        m_type = T::this_type;
        *this = value;
    }

    TypedValue& operator =(const Value& other)
    {
        if (other.m_type != T::this_type)
            // Bad type
            throw_error("Expect %s for value type, got %s.\n",
                          Value::type_to_name(T::this_type),
                          Value::type_to_name(other.m_type));
        m_reference = other.m_reference;
        return *this;
    }

    TypedValue& operator =(const TypedValue& other)
    {
        m_reference = other.m_reference;
        return *this;
    }

    TypedValue& operator =(const T *other_impl)
    {
        m_reference = (T *)other_impl;
        return *this;
    }

    bool operator ==(const TypedValue& other) const
    {
        return *(T *)m_reference == *(const T *)other.m_reference;
    }

    bool operator <(const TypedValue& other) const
    {
        return *(T *)m_reference < *(const T *)other.m_reference;
    }

    bool operator !=(const TypedValue& other) const
    {
        return !(*(T *)m_reference == *(const T *)other.m_reference);
    }

    bool operator >(const TypedValue& other) const
    {
        return *(const T *)other.m_reference < *(T *)m_reference;
    }

    bool operator <=(const TypedValue& other) const
    {
        return !(*(T *)m_reference > *(const T *)other.m_reference);
    }

    bool operator >=(const TypedValue& other) const
    {
        return !(*(T *)m_reference < *(const T *)other.m_reference);
    }

public:
    T &impl()
    {
        return *(T *)m_reference;
    }

    const T &impl() const
    {
        return *(T *)m_reference;
    }

    T *ptr()
    {
        return (T *)m_reference;
    }

    T *const ptr() const
    {
        return (T *)m_reference;
    }

};

class String : public TypedValue<StringImpl>
{
private:
    // WHY PRIVATE? See comment of Value()
    String() :
        TypedValue(EMPTY_STRING)
    {
    }

    // 
    // Don't define String()
public:
    String(const Value& value) :
        TypedValue(value)
    {
    }

    String(StringImpl *const impl) :
        TypedValue(impl)
    {
    }

    String(const char *c_str, size_t len = SIZE_MAX) :
        TypedValue(c_str, len)
    {
    }

    String(const simple::string& str) :
        TypedValue(str)
    {
    }

    // Redirect function to impl()
    // ATTENTION:
    // Why not override -> or * (return T *) for redirection call?
    // Because the override may cause confusing, I would rather to write
    // more codes to make them easy to understand.
public:
    String operator +(const String& other) const
        { return impl().concat(&other.impl()); }

    String operator +=(const String& other)
        { return (*this = impl().concat(&other.impl())); }

    bool operator ==(const String& other)
        { return this->impl() == other.impl(); }

    bool operator !=(const String& other)
        { return !(this->impl() == other.impl()); }

    int index_val(Integer index) const
        { return impl().index_val(index); }

    const char *c_str() const
        { return impl().c_str(); }

    size_t length() const
        { return impl().length(); }

    // Generate string by snprintf
    template <typename... Types>
    static String snprintf(const char *fmt, size_t n, Types&&... args)
        { return StringImpl::snprintf(fmt, n, simple::forward<Types>(args)...); }

    String sub_string(size_t offset, size_t len = SIZE_MAX) const
        { return impl().sub_of(offset, len); }
};

class Buffer : public TypedValue<BufferImpl>
{
public:////----private:
    // WHY PRIVATE? See comment of Value()
    Buffer() :
        TypedValue(EMPTY_BUFFER)
    {
    }

public:
    Buffer(const Value& value) :
        TypedValue(value)
    {
    }

    Buffer(BufferImpl *impl) :
        TypedValue(impl)
    {
    }

    // Redirect function to impl()
    // ATTENTION:
    // Why not override -> or * (return T *) for redirection call?
    // Because the override may cause confusing, I would rather to write
    // more codes to make them easy to understand.
public:
    Buffer operator +(const Buffer& other) const
    {
        return impl().concat(&other.impl());
    }

    Buffer operator +=(const Buffer& other)
    {
        return (*this = impl().concat(&other.impl()));
    }

    int operator[](Integer index) const
    {
        return impl().index_val(index);
    }
};

class Array : public TypedValue<ArrayImpl>
{
public:////----private:
    // WHY PRIVATE? See comment of Value()
    Array() :
        Array(8)
    {
    };

public:
    Array(const Value& value) :
          TypedValue(value)
    {
    }

    Array(ArrayImpl *impl) :
          TypedValue(impl)
    {
    }

    Array(size_t size_hint);

public:
    Value& index_ptr(const Value& index) const
        { return impl().index_ptr(index); }

    Value& index_ptr(Integer index) const
        { return impl().index_ptr(index); }

    const Value index_val(const Value& index) const
    {
        return impl().index_ptr(index);
    }

    const Value index_val(Integer index) const
    {
        return impl().index_ptr(index);
    }

    void push_back(const Value& value)
        { impl().push_back(value); }

    void push_back_array(const Value* value_arr, size_t count)
        { impl().push_back_array(value_arr, count); }

    size_t size() const { return impl().size(); }

    auto begin() -> decltype(impl().a.begin())
        { return impl().a.begin(); }

    auto end() -> decltype(impl().a.begin())
        { return impl().a.end(); }
};

class Map : public TypedValue<MapImpl>
{
public:////----private:
    // WHY PRIVATE? See comment of Value()
    Map() :
        Map(8)
    {
    }

public:
    Map(const Value& value) :
        TypedValue(value)
    {
    }

    Map(MapImpl *impl) :
        TypedValue(impl)
    {
    }

    Map(size_t size_hint);

public:
    Value& index_ptr(const Value& index)
        { return impl().index_ptr(index); }

    const Value index_val(const Value& index) const
        { return impl().index_val(index); }

    void put(const Value& key, const Value& value)
        { impl().put(key, value); }

    Value keys() const
        { return impl().keys(); }

    size_t size() const
        { return impl().size(); }

    Value values() const
        { return impl().values(); }

    auto begin() -> decltype(impl().m.begin())
        { return impl().m.begin(); }

    auto end() -> decltype(impl().m.begin())
        { return impl().m.end(); }
};

// Common operators for convenience
Value operator +(const Value& a, const Value& b);
Value operator -(const Value& a, const Value& b);
Value operator *(const Value& a, const Value& b);
Value operator /(const Value& a, const Value& b);
Value operator %(const Value& a, const Value& b);
Value operator &(const Value& a, const Value& b);
Value operator |(const Value& a, const Value& b);
Value operator ^(const Value& a, const Value& b);
Value operator ~(const Value& a);
Value operator -(const Value& a);
Value operator <<(const Value& a, const Value& b);
Value operator >>(const Value& a, const Value& b);

// Compare operators
bool operator !(const Value& a);
bool operator ==(const Value& a, const Value& b);
bool operator !=(const Value& a, const Value& b);
bool operator <(const Value& a, const Value& b);
bool operator >(const Value& a, const Value& b);
bool operator <=(const Value& a, const Value& b);
bool operator >=(const Value& a, const Value& b);

} // End namespace: cmm
