#pragma once

namespace das {
    struct SimNode;
    struct TypeInfo;
    struct FuncInfo;
    struct LineInfoArg;

    class JitFn {
    private:
        template <typename RetT, typename... Args>
        friend struct CallJitFn; // Call it only using this helper
        void *jitFn;

    public:
        JitFn() : jitFn(nullptr) {}
        JitFn(void *jitFn) : jitFn(jitFn) {}

        operator bool() const {
            return jitFn != nullptr;
        }
    };

    struct Block {
        uint32_t stackOffset;
        uint32_t argumentsOffset;
        SimNode *body;
        void *aotFunction;
        JitFn jitFunction;
        vec4f *functionArguments;
        FuncInfo *info;
        __forceinline bool operator==(const Block &b) const {
            return b.stackOffset == stackOffset && b.argumentsOffset == argumentsOffset && b.body == body && b.functionArguments == functionArguments;
        }
    };

    class Context;

    typedef vec4f (*JitBlockFunction)(Context *, vec4f *, void *, Block *);

    template <typename Result, typename... Args>
    struct TBlock : Block {
        TBlock() {}
        TBlock(const TBlock &) = default;
        TBlock(const Block &that) { *(Block *)this = that; }
    };

    struct Func {
        struct SimFunction *PTR;
        Func() : PTR(nullptr) {}
        Func(SimFunction *P) : PTR(P) {}
        Func(void *P) : PTR((SimFunction *)P) {}
        __forceinline operator void *() const {
            return PTR;
        }
        __forceinline explicit operator bool() const {
            return PTR != nullptr;
        }
        __forceinline bool operator==(void *ptr) const {
            return !ptr && !PTR;
        }
        __forceinline bool operator!=(void *ptr) const {
            return ptr || PTR;
        }
        __forceinline bool operator==(const Func &b) const {
            return PTR == b.PTR;
        }
        __forceinline bool operator!=(const Func &b) const {
            return PTR != b.PTR;
        }
    };
    static_assert(sizeof(Func) == sizeof(struct SimFunction *), "has to be castable");

    template <typename Result, typename... Args>
    struct TFunc : Func {
        TFunc() {}
        TFunc(const TFunc &) = default;
        TFunc(const Func &that) { *(Func *)this = that; }
    };

    struct Lambda {
        Lambda() : capture(nullptr) {}
        Lambda(void *ptr) : capture((char *)ptr) {}
        char *capture;
        __forceinline operator void *() const {
            return capture;
        }
        __forceinline TypeInfo *getTypeInfo() const {
            return capture ? *(TypeInfo **)(capture - 16) : nullptr;
        }
        __forceinline bool operator==(const Lambda &b) const {
            return capture == b.capture;
        }
        __forceinline bool operator!=(const Lambda &b) const {
            return capture != b.capture;
        }
        __forceinline bool operator==(void *ptr) const {
            return capture == ptr;
        }
        __forceinline bool operator!=(void *ptr) const {
            return capture != ptr;
        }
    };
    static_assert(sizeof(Lambda) == sizeof(void *), "has to be castable");

    template <typename Result, typename... Args>
    struct TLambda : Lambda {
        TLambda() {}
        TLambda(const TLambda &) = default;
        TLambda(const Lambda &that) { *(Lambda *)this = that; }
    };

    struct DAS_API GcRootLambda : Lambda {
        GcRootLambda() = default;
        GcRootLambda(const GcRootLambda &) = delete;
        GcRootLambda(GcRootLambda &&other) : Lambda(other.capture), context(other.context) {
            other.capture = nullptr;
            other.context = nullptr;
        }
        GcRootLambda &operator=(const GcRootLambda &) = delete;
        __forceinline GcRootLambda &operator=(GcRootLambda &&l) {
            if (this == &l)
                return *this;
            capture = l.capture;
            context = l.context;
            l.capture = nullptr;
            l.context = nullptr;
            return *this;
        }
        GcRootLambda(const Lambda &that, Context *_context);
        ~GcRootLambda();
        void reset() {
            capture = nullptr;
            context = nullptr;
        }
        Context *context = nullptr;
    };

    struct Tuple {
        Tuple() {}
    };

#pragma pack(1)
    struct Variant {
        int32_t index = 0;
        Variant() {}
    };
#pragma pack()

#define DAS_ARRAY_MAGIC 0xA11B3DA7 // magic number for array validation (alive data)

    struct Table;

    struct Array {
        char *data;
        uint32_t size;
        uint32_t capacity;
    // TArray, Table can set manually during copying.
    protected:
        // use friend helpers to lock-unlock.
        uint32_t magic;
        uint32_t lock;
    public:
        union {
            struct {
                bool shared : 1;
                bool hopeless : 1;          // needs to be deleted without fuss (exceptions)
                bool forego_lock_check : 1; // don't need to check if elements are locked
            };
            uint32_t flags;
        };
        __forceinline bool isLocked() const { return lock; }

        friend DAS_API int builtin_array_lock_count ( const Array & arr );
        friend DAS_API void array_mark_locked(Array &arr, void *data, uint32_t capacity);
        friend DAS_API void array_mark_locked(Array &arr, void *data, uint32_t size, uint32_t capacity);
        friend DAS_API void array_lock(Context &context, Array &arr, LineInfo *at);
        friend DAS_API void array_unlock(Context &context, Array &arr, LineInfo *at);
        friend DAS_API void table_lock(Context &context, Table &arr, LineInfo *at);
        friend DAS_API void table_unlock(Context &context, Table &arr, LineInfo *at);
        template <typename KeyType> friend class TableHash;
    };

    class Context;

    DAS_API void array_mark_locked(Array &arr, void *data, uint32_t capacity);
    DAS_API void array_mark_locked(Array &arr, void *data, uint32_t size, uint32_t capacity);
    DAS_API void array_lock(Context &context, Array &arr, LineInfo *at);
    DAS_API void array_unlock(Context &context, Array &arr, LineInfo *at);
    DAS_API void array_reserve(Context &context, Array &arr, uint32_t newCapacity, uint32_t stride, LineInfo *at);
    DAS_API void array_resize(Context &context, Array &arr, uint32_t newSize, uint32_t stride, bool zero, LineInfo *at);
    DAS_API void array_grow(Context &context, Array &arr, uint32_t newSize, uint32_t stride); // always grows
    DAS_API void array_clear(Context &context, Array &arr, LineInfo *at);

    typedef uint32_t TableHashKey;

#define DAS_TABLE_MAGIC 0xA11B37AB // magic number for table validation (alive table)

    struct Table : Array {
        char *keys;
        TableHashKey *hashes;
        uint32_t tombstones;
    };

    DAS_API void table_clear(Context &context, Table &arr, LineInfo *at);
    DAS_API void table_lock(Context &context, Table &arr, LineInfo *at);
    DAS_API void table_unlock(Context &context, Table &arr, LineInfo *at);
    DAS_API void table_reserve_impl(Context &context, Table &arr, int32_t baseType, uint32_t newCapacity, uint32_t valueTypeSize, LineInfo *at);

    struct Sequence;
    DAS_API void builtin_table_keys(Sequence &result, const Table &tab, int32_t stride, Context *__context__, LineInfoArg *at);
    DAS_API void builtin_table_values(Sequence &result, const Table &tab, int32_t stride, Context *__context__, LineInfoArg *at);
    DAS_API void builtin_table_get_key(void * result, const Table & tab, const void * value_ptr, int32_t value_stride, int32_t key_stride, Context * __context__, LineInfoArg * at);

    template <typename TT>
    struct EnumStubAny {
        typedef TT BaseType;
        TT value;
    };

    typedef EnumStubAny<int32_t> EnumStub;
    typedef EnumStubAny<int8_t> EnumStub8;
    typedef EnumStubAny<int16_t> EnumStub16;
    typedef EnumStubAny<int64_t> EnumStub64;

    template <typename ST>
    struct BitfieldAny {
        ST value;
        __forceinline BitfieldAny() {}
        __forceinline BitfieldAny(int32_t v) : value(ST(v)) {}
        __forceinline BitfieldAny(uint32_t v) : value(ST(v)) {}
        __forceinline BitfieldAny(int64_t v) : value(ST(v)) {}
        __forceinline BitfieldAny(uint64_t v) : value(ST(v)) {}
        __forceinline BitfieldAny(int8_t v) : value(ST(v)) {}
        __forceinline BitfieldAny(uint8_t v) : value(ST(v)) {}
        __forceinline BitfieldAny(int16_t v) : value(ST(v)) {}
        __forceinline BitfieldAny(uint16_t v) : value(ST(v)) {}
        __forceinline BitfieldAny(float v) : value(ST(v)) {}
        __forceinline BitfieldAny(double v) : value(ST(v)) {}
        __forceinline operator ST &() { return value; }
        __forceinline operator float() const { return float(value); }
        __forceinline operator double() const { return double(value); }
        __forceinline operator int8_t() const { return int8_t(value); }
        __forceinline operator uint8_t() const { return uint8_t(value); }
        __forceinline operator uint32_t() const { return uint32_t(value); }
        __forceinline operator int32_t() const { return int32_t(value); }
        __forceinline operator int16_t() const { return int16_t(value); }
        __forceinline operator uint16_t() const { return uint16_t(value); }
        __forceinline operator int64_t() const { return int64_t(value); }
        __forceinline operator uint64_t() const { return uint64_t(value); }
        __forceinline bool operator==(const BitfieldAny<ST> &f) const { return value == f.value; }
        __forceinline bool operator!=(const BitfieldAny<ST> &f) const { return value != f.value; }
        __forceinline bool operator==(ST f) const { return value == f; }
        __forceinline bool operator!=(ST f) const { return value != f; }
    };

    typedef BitfieldAny<uint8_t> Bitfield8;
    typedef BitfieldAny<uint16_t> Bitfield16;
    typedef BitfieldAny<uint32_t> Bitfield;
    typedef BitfieldAny<uint64_t> Bitfield64;

    __forceinline bool operator==(uint8_t f, const Bitfield8 &t) { return t.value == f; }
    __forceinline bool operator!=(uint8_t f, const Bitfield8 &t) { return t.value != f; }
    __forceinline bool operator==(uint16_t f, const Bitfield16 &t) { return t.value == f; }
    __forceinline bool operator!=(uint16_t f, const Bitfield16 &t) { return t.value != f; }
    __forceinline bool operator==(uint32_t f, const Bitfield &t) { return t.value == f; }
    __forceinline bool operator!=(uint32_t f, const Bitfield &t) { return t.value != f; }
    __forceinline bool operator==(uint64_t f, const Bitfield64 &t) { return t.value == f; }
    __forceinline bool operator!=(uint64_t f, const Bitfield64 &t) { return t.value != f; }
}

namespace std {
    template <>
    struct hash<das::Bitfield> {
        size_t operator()(das::Bitfield b) const {
            return hash<uint32_t>()(b.value);
        }
    };
}
