#pragma once

namespace das
{
    struct SimNode;
    struct TypeInfo;
    struct FuncInfo;
    struct LineInfoArg;

    struct Block {
        uint32_t    stackOffset;
        uint32_t    argumentsOffset;
        SimNode *   body;
        void *      aotFunction;
        void *      jitFunction;
        vec4f *     functionArguments;
        FuncInfo *  info;
        __forceinline bool operator == ( const Block & b ) const {
            return b.stackOffset==stackOffset && b.argumentsOffset==argumentsOffset
                && b.body==body && b.functionArguments==functionArguments;
        }
    };

    class Context;

    typedef vec4f ( * JitBlockFunction ) ( Context * , vec4f *, void *, Block * );

    template <typename Result, typename ...Args>
    struct TBlock : Block {
        TBlock() {}
        TBlock( const TBlock & ) = default;
        TBlock( const Block & that ) { *(Block *)this = that; }
    };

    struct Func {
        struct SimFunction * PTR;
        Func() : PTR(nullptr) {}
        Func(SimFunction * P) : PTR(P) {}
        Func(void * P) : PTR((SimFunction *)P) {}
        __forceinline operator void * () const {
            return PTR;
        }
        __forceinline explicit operator bool () const {
            return PTR!=nullptr;
        }
        __forceinline bool operator == ( void * ptr ) const {
            return !ptr && !PTR;
        }
        __forceinline bool operator != ( void * ptr ) const {
            return ptr || PTR;
        }
        __forceinline bool operator == ( const Func & b ) const {
            return PTR == b.PTR;
        }
        __forceinline bool operator != ( const Func & b ) const {
            return PTR != b.PTR;
        }
    };
    static_assert(sizeof(Func)==sizeof(struct SimFunction *), "has to be castable");

    template <typename Result, typename ...Args>
    struct TFunc : Func {
        TFunc()  {}
        TFunc( const TFunc & ) = default;
        TFunc( const Func & that ) { *(Func *)this = that; }
    };

    struct Lambda {
        Lambda() : capture(nullptr) {}
        Lambda(void * ptr) : capture((char *)ptr) {}
        char *      capture;
        __forceinline operator void * () const {
            return capture;
        }
        __forceinline TypeInfo * getTypeInfo() const {
            return capture ? *(TypeInfo **)(capture-16) : nullptr;
        }
        __forceinline bool operator == ( const Lambda & b ) const {
            return capture == b.capture;
        }
        __forceinline bool operator != ( const Lambda & b ) const {
            return capture != b.capture;
        }
        __forceinline bool operator == ( void * ptr ) const {
            return capture == ptr;
        }
        __forceinline bool operator != ( void * ptr ) const {
            return capture != ptr;
        }
    };
    static_assert(sizeof(Lambda)==sizeof(void *), "has to be castable");

    template <typename Result, typename ...Args>
    struct TLambda : Lambda {
        TLambda()  {}
        TLambda( const TLambda & ) = default;
        TLambda( const Lambda & that ) { *(Lambda *)this = that; }
    };

    struct GcRootLambda : Lambda  {
        GcRootLambda() = default;
        GcRootLambda( const GcRootLambda & ) = delete;
        GcRootLambda( GcRootLambda && other) : Lambda(other.capture), context(other.context) {
            other.capture = nullptr;
            other.context = nullptr;
        }
        GcRootLambda & operator = ( const GcRootLambda & ) = delete;
        __forceinline GcRootLambda & operator = ( GcRootLambda && l ) {
            if ( this == &l ) return *this;
            capture = l.capture;
            context = l.context;
            l.capture = nullptr;
            l.context = nullptr;
            return *this;
        }
        GcRootLambda( const Lambda & that, Context * _context );
        ~GcRootLambda();
        void reset() { capture = nullptr; context = nullptr; }
        Context * context = nullptr;
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

    struct Array {
        char *      data;
        uint32_t    size;
        uint32_t    capacity;
        uint32_t    lock;
        union {
            struct {
                bool    shared : 1;
                bool    hopeless : 1;           // needs to be deleted without fuss (exceptions)
                bool    forego_lock_check : 1;  // don't need to check if elements are locked
            };
            uint32_t    flags;
        };
        __forceinline bool isLocked() const { return lock; }
    };

    class Context;

    void array_lock ( Context & context, Array & arr, LineInfo * at );
    void array_unlock ( Context & context, Array & arr, LineInfo * at );
    void array_reserve ( Context & context, Array & arr, uint32_t newCapacity, uint32_t stride, LineInfo * at );
    void array_resize ( Context & context, Array & arr, uint32_t newSize, uint32_t stride, bool zero, LineInfo * at );
    void array_grow ( Context & context, Array & arr, uint32_t newSize, uint32_t stride );  // always grows
    void array_clear ( Context & context, Array & arr, LineInfo * at );

    typedef uint32_t TableHashKey;

    struct Table : Array {
        char *      keys;
        TableHashKey *  hashes;
        uint32_t    tombstones;
    };

    void table_clear ( Context & context, Table & arr, LineInfo * at );
    void table_lock ( Context & context, Table & arr, LineInfo * at );
    void table_unlock ( Context & context, Table & arr, LineInfo * at );

    struct Sequence;
    void builtin_table_keys ( Sequence & result, const Table & tab, int32_t stride, Context * __context__, LineInfoArg * at );
    void builtin_table_values ( Sequence & result, const Table & tab, int32_t stride, Context * __context__, LineInfoArg * at );

    template <typename TT>
    struct EnumStubAny  {
        typedef TT  BaseType;
        TT          value;
    };

    typedef EnumStubAny<int32_t> EnumStub;
    typedef EnumStubAny<int8_t>  EnumStub8;
    typedef EnumStubAny<int16_t> EnumStub16;
    typedef EnumStubAny<int64_t> EnumStub64;

    struct Bitfield {
        uint32_t    value;
        __forceinline Bitfield () {}
        __forceinline Bitfield ( int32_t v ) : value(uint32_t(v)) {}
        __forceinline Bitfield ( uint32_t v ) : value(v) {}
        __forceinline Bitfield ( int64_t v ) : value(uint32_t(v)) {}
        __forceinline Bitfield ( uint64_t v ) : value(uint32_t(v)) {}
        __forceinline Bitfield ( int8_t v ) : value(uint32_t(v)) {}
        __forceinline Bitfield ( uint8_t v ) : value(uint32_t(v)) {}
        __forceinline Bitfield ( int16_t v ) : value(uint32_t(v)) {}
        __forceinline Bitfield ( uint16_t v ) : value(uint32_t(v)) {}
        __forceinline Bitfield ( float v ) : value(uint32_t(v)) {}
        __forceinline Bitfield ( double v ) : value(uint32_t(v)) {}
        __forceinline operator uint32_t & () { return value; }
        __forceinline operator const uint32_t & () const { return value; }
        __forceinline operator float () const { return float(value); }
        __forceinline operator double () const { return double(value); }
        __forceinline operator int32_t () const { return int32_t(value); }
        __forceinline operator int16_t () const { return int16_t(value); }
        __forceinline operator uint16_t () const { return uint16_t(value); }
        __forceinline operator int64_t () const { return int64_t(value); }
        __forceinline operator uint64_t () const { return uint64_t(value); }
        __forceinline bool operator == ( const Bitfield & f ) const { return value==f.value; }
        __forceinline bool operator != ( const Bitfield & f ) const { return value!=f.value; }
        __forceinline bool operator == ( uint32_t f ) const { return value==f; }
        __forceinline bool operator != ( uint32_t f ) const { return value!=f; }
    };
    __forceinline bool operator == ( uint32_t f, const Bitfield & t ) { return t.value==f; }
    __forceinline bool operator != ( uint32_t f, const Bitfield & t ) { return t.value!=f; }

}

namespace std {
    template <> struct hash<das::Bitfield> {
        std::size_t operator() ( das::Bitfield b ) const {
            return hash<uint32_t>()(b.value);
        }
    };
}
