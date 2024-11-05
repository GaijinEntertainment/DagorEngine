#pragma once

#include "daScript/misc/memory_model.h"
#include "daScript/misc/callable.h"
#include "daScript/misc/anyhash.h"

namespace das {

    class StackAllocator {
    public:
        StackAllocator(const StackAllocator &) = delete;
        StackAllocator & operator = (const StackAllocator &) = delete;

        StackAllocator(uint32_t size) {
            stackSize = size;
            stack = stackSize ? (char*)das_aligned_alloc16(stackSize) : nullptr;
            reset();
        }

        void strip () {
            if ( stack ) {
                das_aligned_free16(stack);
                stack = nullptr;
            }
        }

        virtual ~StackAllocator() {
            strip();
        }

        __forceinline void letGo () {
            stack = nullptr;
        }

        __forceinline void copy ( const StackAllocator & src ) {
            stack = src.stack;
            evalTop = src.evalTop;
            stackTop = src.stackTop;
            stackSize = src.stackSize;
        }

        __forceinline uint32_t size() const {
            return stackSize;
        }

        __forceinline bool empty() {
            return stackTop == (stack + stackSize);
        }

        __forceinline void reset() {
            evalTop = stackTop = stack + stackSize;
        }

        __forceinline bool push(uint32_t size, char * & EP, char * & SP ) {        // stack watermark
            DAS_ASSERTF(stack,"can't push on null stack");
            if (stackTop - size < stack ) {
                return false;
            }
            EP = evalTop;
            SP = stackTop;
            stackTop -= size;
            evalTop = stackTop;
            return true;
        }

        __forceinline void watermark ( char * & EP, char * & SP ) {
            EP = evalTop;
            SP = stackTop;
        }

        __forceinline void pop(char * EP, char * SP ) {    // restore stack to watermark
            evalTop = EP;
            stackTop = SP;
        }

        __forceinline void invoke(uint32_t et, char * & EP, char * & SP ) {    // pass bottom portion of the watermark
            EP = evalTop;
            SP = stackTop;
            evalTop = stack + et;
        }

        __forceinline bool push_invoke(uint32_t size, uint32_t et, char * & EP, char * & SP ) {
            DAS_ASSERTF(stack,"can't push on null stack");
            if (stackTop - size < stack ) {
                return false;
            }
            EP = evalTop;
            SP = stackTop;
            stackTop -= size;
            evalTop = stack + et;
            return true;
        }

        __forceinline char * ap() const {                // allocation stack
            return stackTop;
        }

        __forceinline char * sp() const {                // execution stack
            return evalTop;
        }

        __forceinline uint32_t spi() const {            // execution stack offset
            return uint32_t(evalTop - stack);
        }

        __forceinline uint32_t api() const {              // allocation stack offset
            return uint32_t(stackTop - stack);
        }

        __forceinline char * bottom() const {            // bottom of the stack
            return stack;
        }

        __forceinline char * top() const {                // top of the stack
            return stack + stackSize;
        }
        __forceinline bool is_stack_ptr ( char * p ) const {
            return (stack<=p) && (p<=(stack + stackSize));
        }
    public:
        char *      stack = nullptr;
        char *      evalTop = nullptr;
        char *      stackTop = nullptr;
        uint32_t    stackSize = 0;
    };

    class AnyHeapAllocator : public ptr_ref_count {
    public:
        virtual bool breakOnFree ( void *, uint32_t ) { return false; }
        virtual char * impl_allocate ( uint32_t ) = 0;
        virtual void impl_free ( char *, uint32_t ) = 0;
        virtual char * impl_reallocate ( char *, uint32_t, uint32_t ) = 0;
        virtual int depth() const = 0;
        virtual uint64_t bytesAllocated() const = 0;
        virtual uint64_t totalAlignedMemoryAllocated() const = 0;
        virtual void reset() = 0;
        virtual void report() = 0;
        virtual bool mark() = 0;
        virtual bool mark ( char * ptr, uint32_t size ) = 0;
        virtual void sweep() = 0;
        virtual bool isOwnPtr ( char * ptr, uint32_t size ) = 0;
        virtual bool isValidPtr ( char * ptr, uint32_t size ) = 0;  // only if isOwnPtr
        virtual void setInitialSize ( uint32_t size ) = 0;
        virtual int32_t getInitialSize() const = 0;
        virtual void setGrowFunction ( CustomGrowFunction && fun ) = 0;
        __forceinline void setLimit ( uint64_t l ) { limit = l; }
        __forceinline uint64_t getLimit() const { return limit; }
        __forceinline uint64_t getTotalAllocations() const { return totalAllocations; }
        __forceinline uint64_t getTotalBytesAllocated() const { return totalBytesAllocated; }
        __forceinline uint64_t getTotalBytesDeleted() const { return totalBytesDeleted; }
    public:
#if DAS_TRACK_ALLOCATIONS
        virtual void mark_location ( void *, const LineInfo * )  {}
        virtual  void mark_comment ( void *, const char * ) {}
#else
        __forceinline void mark_location ( void *, const LineInfo * ) {}
        __forceinline void mark_comment ( void *, const char * ) {}
#endif
    public:
        char * allocateName ( const string & name );
        char * impl_allocateIterator ( uint32_t size, const char * name="", const LineInfo * info=nullptr );
        void   impl_freeIterator ( char * ptr );
    protected:
        uint64_t limit = 0;
        uint64_t totalAllocations = 0;
        uint64_t totalBytesAllocated = 0;
        uint64_t totalBytesDeleted = 0;
    };

    struct StrHashEntry {
        const char * ptr;
        uint32_t     length;
        StrHashEntry() : ptr(nullptr), length(0) {}
        StrHashEntry( const char * p, uint32_t l ) : ptr(p), length(l) {}
    };

    struct StrEqPred {
        __forceinline bool operator()( const StrHashEntry & a, const StrHashEntry & b ) const {
            if ( a.ptr==b.ptr ) return true;
            else if ( a.length!=b.length ) return false;
            else return strncmp(a.ptr, b.ptr, a.length)==0;
        }
    };

    struct StrHashPred {
        __forceinline size_t operator() ( const StrHashEntry & a ) const {
            return (size_t) hash_block64((const uint8_t *)a.ptr, a.length);
        }
    };

    class Context;

    typedef das_hash_set<StrHashEntry,StrHashPred,StrEqPred> das_string_set;

    class StringHeapAllocator : public AnyHeapAllocator {
    public:
        virtual void forEachString ( const callable<void (const char *)> & fn ) = 0;
        virtual void reset() override;
    public:
        char * impl_allocateString ( Context * context, const char * text, uint32_t length, const LineInfo * at = nullptr );
        void impl_freeString ( char * text, uint32_t length );
        void setIntern ( bool on );
        bool isIntern() const { return needIntern; }
        char * intern ( const char * str, uint32_t length ) const;
        void recognize ( char * str );
    protected:
        das_string_set internMap;
        bool needIntern = false;
    };

#ifndef DAS_HEAP_DEBUGGER_BREAK_ON_FREE
#define DAS_HEAP_DEBUGGER_BREAK_ON_FREE 0
#endif

    class PersistentHeapAllocator final : public AnyHeapAllocator {
#if DAS_HEAP_DEBUGGER_BREAK_ON_FREE
        void * breakFreeAddr = nullptr;
        uint32_t breakFreeSize = 0;
    public:
        virtual bool breakOnFree ( void * ptr, uint32_t size ) {
            size = (size + 15) & ~15;
            breakFreeAddr = ptr;
            breakFreeSize = size;
            return true;
        }
        virtual void free ( char * ptr, uint32_t size ) override {
            if ( ptr==breakFreeAddr ) os_debug_break();
            model.free(ptr,size);
        }
        virtual void sweep() override {
            model.sweep();
            if ( breakFreeAddr!=nullptr && !model.isAllocatedPtr((char *)breakFreeAddr,breakFreeSize) ) {
                printf("COLLECTED: breakFreeAddr = %p, breakFreeSize = %u\n", breakFreeAddr, breakFreeSize);
                breakFreeAddr = nullptr;
                breakFreeSize = 0;
            }
        }
#else
    public:
        virtual void impl_free ( char * ptr, uint32_t size ) override {
            totalBytesDeleted += size;
            model.free(ptr,size);
        }
        virtual void sweep() override { model.sweep(); }
#endif
    public:
        PersistentHeapAllocator() {}
        virtual char * impl_allocate ( uint32_t size ) override {
            if ( limit==0 || model.bytesAllocated()+size<=limit ) {
                totalAllocations ++;
                totalBytesAllocated += size;
                return model.allocate(size);
            } else {
                return nullptr;
            }
        }
        virtual char * impl_reallocate ( char * ptr, uint32_t oldSize, uint32_t newSize ) override {
            if ( limit==0 || model.bytesAllocated()+newSize-oldSize<=limit ) {
                totalAllocations ++;
                totalBytesAllocated += newSize-oldSize;
                return model.reallocate(ptr,oldSize,newSize);
            } else {
                return nullptr;
            }
        }
        virtual int depth() const override { return model.depth(); }
        virtual uint64_t bytesAllocated() const override { return model.bytesAllocated(); }
        virtual uint64_t totalAlignedMemoryAllocated() const override { return model.totalAlignedMemoryAllocated(); }
        virtual void reset() override { model.reset(); }
        virtual void report() override;
        virtual bool mark() override;
        virtual bool mark ( char * ptr, uint32_t size ) override;
        virtual bool isOwnPtr ( char * ptr, uint32_t size ) override { return model.isOwnPtr(ptr,size); }
        virtual bool isValidPtr ( char * ptr, uint32_t size ) override { return model.isAllocatedPtr(ptr,size); }
        virtual void setInitialSize ( uint32_t size ) override { model.setInitialSize(size); }
        virtual int32_t getInitialSize() const override { return model.initialSize; }
        virtual void setGrowFunction ( CustomGrowFunction && fun ) override { model.customGrow = fun; };
#if DAS_TRACK_ALLOCATIONS
        virtual void mark_location ( void * ptr, const LineInfo * at ) override  { model.mark_location(ptr,at); };
        virtual  void mark_comment ( void * ptr, const char * what ) override { model.mark_comment(ptr,what); };
#endif
    protected:
        MemoryModel model;
    };

    class LinearHeapAllocator final : public AnyHeapAllocator {
    public:
        LinearHeapAllocator() {}
        virtual char * impl_allocate ( uint32_t size ) override {
            if ( limit==0 || model.bytesAllocated()+size<=limit ) {
                totalAllocations ++;
                totalBytesAllocated += size;
                return model.allocate(size);
            } else {
                return nullptr;
            }
        }
        virtual void impl_free ( char * ptr, uint32_t size ) override {
            totalBytesDeleted += size;
            model.free(ptr,size);
        }
        virtual char * impl_reallocate ( char * ptr, uint32_t oldSize, uint32_t newSize ) override {
            if ( limit==0 || model.bytesAllocated()+newSize-oldSize<=limit ) {
                totalAllocations ++;
                totalBytesAllocated += newSize-oldSize;
                return model.reallocate(ptr,oldSize,newSize);
            } else {
                return nullptr;
            }
        }
        virtual int depth() const override { return model.depth(); }
        virtual uint64_t bytesAllocated() const override { return model.bytesAllocated(); }
        virtual uint64_t totalAlignedMemoryAllocated() const override { return model.totalAlignedMemoryAllocated(); }
        virtual void reset() override { model.reset(); }
        virtual void report() override;
        virtual bool mark() override { return false; }
        virtual bool mark ( char *, uint32_t ) override { DAS_ASSERT(0 && "not supported"); return false; }
        virtual void sweep() override { DAS_ASSERT(0 && "not supported"); }
        virtual bool isOwnPtr ( char * ptr, uint32_t ) override { return model.isOwnPtr(ptr); }
        virtual bool isValidPtr ( char *, uint32_t ) override { return true; }
        virtual void setInitialSize ( uint32_t size ) override { model.setInitialSize(size); }
        virtual int32_t getInitialSize() const override { return model.initialSize; }
        virtual void setGrowFunction ( CustomGrowFunction && fun ) override { model.customGrow = fun; };
    protected:
        LinearChunkAllocator model;
    };

#if DAS_TRACK_ALLOCATIONS
    extern uint64_t    g_tracker_string;
    extern uint64_t    g_breakpoint_string;
    void das_track_string_breakpoint ( uint64_t id );
#endif

    class ConstStringAllocator : public LinearChunkAllocator {
    public:
        ConstStringAllocator() { alignMask = 3; }
        char * impl_allocateString ( const char * text, uint32_t length );
        __forceinline char * impl_allocateString ( const string & str ) {
            return impl_allocateString ( str.c_str(), uint32_t(str.length()) );
        }
        virtual void reset () override;
        char * intern ( const char * str, uint32_t length ) const;
    protected:
        das_string_set internMap;
    };

    class PersistentStringAllocator final : public StringHeapAllocator {
    public:
        PersistentStringAllocator() { model.alignMask = 3; }
        virtual char * impl_allocate ( uint32_t size ) override {
            if ( limit==0 || model.bytesAllocated()+size<=limit ) {
                totalAllocations ++;
                totalBytesAllocated += size;
                return model.allocate(size);
            } else {
                return nullptr;
            }
        }
        virtual void impl_free ( char * ptr, uint32_t size ) override {
            totalBytesDeleted += size;
            model.free(ptr,size);
        }
        virtual char * impl_reallocate ( char * ptr, uint32_t oldSize, uint32_t newSize ) override {
            if ( limit==0 || model.bytesAllocated()+newSize-oldSize<=limit ) {
                totalAllocations ++;
                totalBytesAllocated += newSize-oldSize;
                return model.reallocate(ptr,oldSize,newSize);
            } else {
                return nullptr;
            }
        }
        virtual int depth() const override { return model.depth(); }
        virtual uint64_t bytesAllocated() const override { return model.bytesAllocated(); }
        virtual uint64_t totalAlignedMemoryAllocated() const override { return model.totalAlignedMemoryAllocated(); }
        virtual void reset() override { model.reset(); }
        virtual void forEachString ( const callable<void (const char *)> & fn ) override ;
        virtual void report() override;
        virtual bool mark() override;
        virtual bool mark ( char * ptr, uint32_t size ) override;
        virtual void sweep() override;
        virtual bool isOwnPtr ( char * ptr, uint32_t size ) override { return model.isOwnPtr(ptr,size); }
        virtual bool isValidPtr ( char * ptr, uint32_t size ) override { return model.isAllocatedPtr(ptr,size); }
        virtual void setInitialSize ( uint32_t size ) override { model.setInitialSize(size); }
        virtual int32_t getInitialSize() const override { return model.initialSize; }
        virtual void setGrowFunction ( CustomGrowFunction && fun ) override { model.customGrow = fun; };
#if DAS_TRACK_ALLOCATIONS
        virtual void mark_location ( void * ptr, const LineInfo * at ) override { model.mark_location(ptr,at); };
        virtual  void mark_comment ( void * ptr, const char * what ) override { model.mark_comment(ptr,what); };
#endif
    protected:
        MemoryModel model;
    };

    class LinearStringAllocator final : public StringHeapAllocator {
    public:
        LinearStringAllocator() { model.alignMask = 3; }
        virtual char * impl_allocate ( uint32_t size ) override {
            if ( limit==0 || model.bytesAllocated()+size<=limit ) {
                totalAllocations ++;
                totalBytesAllocated += size;
                return model.allocate(size);
            } else {
                return nullptr;
            }
        }
        virtual void impl_free ( char * ptr, uint32_t size ) override {
            totalBytesDeleted += size;
            model.free(ptr,size);
        }
        virtual char * impl_reallocate ( char * ptr, uint32_t oldSize, uint32_t newSize ) override {
            if ( limit==0 || model.bytesAllocated()+newSize-oldSize<=limit ) {
                totalAllocations ++;
                totalBytesAllocated += newSize-oldSize;
                return model.reallocate(ptr,oldSize,newSize);
            } else {
                return nullptr;
            }
        }
        virtual int depth() const override { return model.depth(); }
        virtual uint64_t bytesAllocated() const override { return model.bytesAllocated(); }
        virtual uint64_t totalAlignedMemoryAllocated() const override { return model.totalAlignedMemoryAllocated(); }
        virtual void reset() override { model.reset(); }
        virtual void forEachString ( const callable<void (const char *)> & fn ) override;
        virtual void report() override;
        virtual bool mark() override { return false; }
        virtual bool mark ( char *, uint32_t ) override { DAS_ASSERT(0 && "not supported"); return false; }
        virtual void sweep() override { DAS_ASSERT(0 && "not supported"); }
        virtual bool isOwnPtr ( char * ptr, uint32_t ) override { return model.isOwnPtr(ptr); }
        virtual bool isValidPtr ( char *, uint32_t ) override { return true; }
        virtual void setInitialSize ( uint32_t size ) override { model.setInitialSize(size); }
        virtual int32_t getInitialSize() const override { return model.initialSize; }
        virtual void setGrowFunction ( CustomGrowFunction && fun ) override { model.customGrow = fun; };
    protected:
        LinearChunkAllocator model;
    };

    struct NodePrefix {
        uint32_t    size = 0;
#ifdef NDEBUG
        uint32_t    magic;
#else
        uint32_t    magic = 0xdeadc0de;
#endif
        uint32_t    padd0, padd1;
        NodePrefix ( size_t sz ) : size(uint32_t(sz)) {}
    };
    static_assert(sizeof(NodePrefix)==sizeof(vec4f), "node prefix must be one alignment line");

    class NodeAllocator : public LinearChunkAllocator {
    public:
        bool prefixWithHeader = true;
        uint32_t totalNodesAllocated = 0;
    public:
        NodeAllocator() {}

        /*
        * GCC really likes the version with separate if. CLANG \ MSVC strongly prefer the one bellow with __forceinline.
        * This saves ~1.6mb of code on MSVC\CLANG as of 11/23/2020.
        */
#if defined(__GNUC__) && !defined(__clang__)
        template<typename TT, typename... Params>
        TT * makeNode(Params... args) {
            totalNodesAllocated ++;
            if ( prefixWithHeader ) {
                char * data = allocate(sizeof(TT) + sizeof(NodePrefix));
                new ((void *)data) NodePrefix(sizeof(TT));
                return new ((void *)(data + sizeof(NodePrefix))) TT(args...);
            } else {
                return new ((void *)allocate(sizeof(TT))) TT(args...);
            }
        }
#else
        template<typename TT, typename... Params>
        __forceinline TT * makeNode(Params... args) {
            totalNodesAllocated++;
            char * data = allocate(prefixWithHeader ? (sizeof(TT)+sizeof(NodePrefix)) : sizeof(TT));
            if ( prefixWithHeader ) {
                new ((void *)data) NodePrefix(sizeof(TT));
                data += sizeof(NodePrefix);
            }
            return new ((void *)data) TT(args...);
        }
#endif

        template < template <typename TT> class NodeType, typename... Params>
        SimNode * makeNumericValueNode(Type baseType, Params... args) {
            switch (baseType) {
                case Type::tInt8:       return makeNode<NodeType<int8_t>>(args...);
                case Type::tUInt8:      return makeNode<NodeType<uint8_t>>(args...);
                case Type::tInt16:      return makeNode<NodeType<int16_t>>(args...);
                case Type::tUInt16:     return makeNode<NodeType<uint16_t>>(args...);
                case Type::tInt64:      return makeNode<NodeType<int64_t>>(args...);
                case Type::tUInt64:     return makeNode<NodeType<uint64_t>>(args...);
                case Type::tInt:        return makeNode<NodeType<int32_t>>(args...);
                case Type::tUInt:       return makeNode<NodeType<uint32_t>>(args...);
                case Type::tFloat:      return makeNode<NodeType<float>>(args...);
                case Type::tDouble:     return makeNode<NodeType<double>>(args...);
                case Type::tPointer:    return makeNode<NodeType<void *>>(args...);
                case Type::tBitfield:   return makeNode<NodeType<uint32_t>>(args...);
                default:
                    DAS_ASSERTF(0, "we should not even be here. we are calling makeNumericValueNode on an uspported baseType."
                                "likely new numeric type been added.");
                    return nullptr;
            }
        }

        template < template <typename TT> class NodeType, typename... Params>
        SimNode * makeValueNode(Type baseType, Params... args) {
            switch (baseType) {
            case Type::tBool:           return makeNode<NodeType<bool>>(args...);
            case Type::tInt8:           return makeNode<NodeType<int8_t>>(args...);
            case Type::tUInt8:          return makeNode<NodeType<uint8_t>>(args...);
            case Type::tInt16:          return makeNode<NodeType<int16_t>>(args...);
            case Type::tUInt16:         return makeNode<NodeType<uint16_t>>(args...);
            case Type::tInt64:          return makeNode<NodeType<int64_t>>(args...);
            case Type::tUInt64:         return makeNode<NodeType<uint64_t>>(args...);
            case Type::tEnumeration:    return makeNode<NodeType<int32_t>>(args...);
            case Type::tEnumeration8:   return makeNode<NodeType<int8_t>>(args...);
            case Type::tEnumeration16:  return makeNode<NodeType<int16_t>>(args...);
            case Type::tEnumeration64:  return makeNode<NodeType<int64_t>>(args...);
            case Type::tBitfield:       return makeNode<NodeType<Bitfield>>(args...);
            case Type::tInt:            return makeNode<NodeType<int32_t>>(args...);
            case Type::tInt2:           return makeNode<NodeType<int2>>(args...);
            case Type::tInt3:           return makeNode<NodeType<int3>>(args...);
            case Type::tInt4:           return makeNode<NodeType<int4>>(args...);
            case Type::tUInt:           return makeNode<NodeType<uint32_t>>(args...);
            case Type::tUInt2:          return makeNode<NodeType<uint2>>(args...);
            case Type::tUInt3:          return makeNode<NodeType<uint3>>(args...);
            case Type::tUInt4:          return makeNode<NodeType<uint4>>(args...);
            case Type::tFloat:          return makeNode<NodeType<float>>(args...);
            case Type::tFloat2:         return makeNode<NodeType<float2>>(args...);
            case Type::tFloat3:         return makeNode<NodeType<float3>>(args...);
            case Type::tFloat4:         return makeNode<NodeType<float4>>(args...);
            case Type::tRange:          return makeNode<NodeType<range>>(args...);
            case Type::tURange:         return makeNode<NodeType<urange>>(args...);
            case Type::tRange64:        return makeNode<NodeType<range64>>(args...);
            case Type::tURange64:       return makeNode<NodeType<urange64>>(args...);
            case Type::tString:         return makeNode<NodeType<char *>>(args...);
            case Type::tPointer:        return makeNode<NodeType<void *>>(args...);
            case Type::tIterator:       return makeNode<NodeType<void *>>(args...);
            case Type::tFunction:       return makeNode<NodeType<Func>>(args...);
            case Type::tLambda:         return makeNode<NodeType<Lambda>>(args...);
            case Type::tDouble:         return makeNode<NodeType<double>>(args...);
            default:
                DAS_ASSERTF(0, "we should not even be here. we are calling makeValueNode on an uspported baseType."
                              "likely new by-value builtin type been added.");
                return nullptr;
            }
        }

        template < template <typename TT> class NodeType, typename... Params>
        SimNode * makeRangeNode(Type baseType, Params... args) {
            switch (baseType) {
            case Type::tRange:          return makeNode<NodeType<range>>(args...);
            case Type::tURange:         return makeNode<NodeType<urange>>(args...);
            case Type::tRange64:        return makeNode<NodeType<range64>>(args...);
            case Type::tURange64:       return makeNode<NodeType<urange64>>(args...);
            default:
                DAS_ASSERTF(0, "we should not even be here. we are calling makeRangeNode on an uspported baseType."
                              "likely new by-value builtin range type been added.");
                return nullptr;
            }
        }

        template < template <int TT> class NodeType, typename... Params>
        SimNode* makeNodeUnrollAny(int count, Params... args) {
            switch (count) {
            case  0: return makeNode<NodeType< 0>>(args...);
            case  1: return makeNode<NodeType< 1>>(args...);
            case  2: return makeNode<NodeType< 2>>(args...);
            case  3: return makeNode<NodeType< 3>>(args...);
            case  4: return makeNode<NodeType< 4>>(args...);
            case  5: return makeNode<NodeType< 5>>(args...);
            case  6: return makeNode<NodeType< 6>>(args...);
            case  7: return makeNode<NodeType< 7>>(args...);
            default: return makeNode<NodeType<-1>>(args...);
            }
        }

        template < template <int TT> class NodeType, typename... Params>
        SimNode * makeNodeUnroll(int count, Params... args) {
            switch (count) {
            case  0: return makeNode<NodeType< 0>>(args...);
            case  1: return makeNode<NodeType< 1>>(args...);
            case  2: return makeNode<NodeType< 2>>(args...);
            case  3: return makeNode<NodeType< 3>>(args...);
            case  4: return makeNode<NodeType< 4>>(args...);
            case  5: return makeNode<NodeType< 5>>(args...);
            case  6: return makeNode<NodeType< 6>>(args...);
            case  7: return makeNode<NodeType< 7>>(args...);
            case  8: return makeNode<NodeType< 8>>(args...);
            case  9: return makeNode<NodeType< 9>>(args...);
            case 10: return makeNode<NodeType<10>>(args...);
            case 11: return makeNode<NodeType<11>>(args...);
            case 12: return makeNode<NodeType<12>>(args...);
            case 13: return makeNode<NodeType<13>>(args...);
            case 14: return makeNode<NodeType<14>>(args...);
            case 15: return makeNode<NodeType<15>>(args...);
            case 16: return makeNode<NodeType<16>>(args...);
            case 17: return makeNode<NodeType<17>>(args...);
            case 18: return makeNode<NodeType<18>>(args...);
            case 19: return makeNode<NodeType<19>>(args...);
            case 20: return makeNode<NodeType<20>>(args...);
            case 21: return makeNode<NodeType<21>>(args...);
            case 22: return makeNode<NodeType<22>>(args...);
            case 23: return makeNode<NodeType<23>>(args...);
            case 24: return makeNode<NodeType<24>>(args...);
            case 25: return makeNode<NodeType<25>>(args...);
            case 26: return makeNode<NodeType<26>>(args...);
            case 27: return makeNode<NodeType<27>>(args...);
            case 28: return makeNode<NodeType<28>>(args...);
            case 29: return makeNode<NodeType<29>>(args...);
            case 30: return makeNode<NodeType<30>>(args...);
            case 31: return makeNode<NodeType<31>>(args...);
            case 32: return makeNode<NodeType<32>>(args...);
            default:
                DAS_ASSERTF(0, "we should not even be here. we are calling makeNodeUnroll on a large number or a negative number."
                            "if its negative, there is some issue with the logic of count."
                            "if its large, bigger specialization for unroll should be added.");
                return nullptr;
            }
        }

        template < template <int TT> class NodeType, typename... Params>
        SimNode * makeNodeUnrollNZ(int count, Params... args) {
            switch (count) {
            case  1: return makeNode<NodeType< 1>>(args...);
            case  2: return makeNode<NodeType< 2>>(args...);
            case  3: return makeNode<NodeType< 3>>(args...);
            case  4: return makeNode<NodeType< 4>>(args...);
            case  5: return makeNode<NodeType< 5>>(args...);
            case  6: return makeNode<NodeType< 6>>(args...);
            case  7: return makeNode<NodeType< 7>>(args...);
            case  8: return makeNode<NodeType< 8>>(args...);
            case  9: return makeNode<NodeType< 9>>(args...);
            case 10: return makeNode<NodeType<10>>(args...);
            case 11: return makeNode<NodeType<11>>(args...);
            case 12: return makeNode<NodeType<12>>(args...);
            case 13: return makeNode<NodeType<13>>(args...);
            case 14: return makeNode<NodeType<14>>(args...);
            case 15: return makeNode<NodeType<15>>(args...);
            case 16: return makeNode<NodeType<16>>(args...);
            case 17: return makeNode<NodeType<17>>(args...);
            case 18: return makeNode<NodeType<18>>(args...);
            case 19: return makeNode<NodeType<19>>(args...);
            case 20: return makeNode<NodeType<20>>(args...);
            case 21: return makeNode<NodeType<21>>(args...);
            case 22: return makeNode<NodeType<22>>(args...);
            case 23: return makeNode<NodeType<23>>(args...);
            case 24: return makeNode<NodeType<24>>(args...);
            case 25: return makeNode<NodeType<25>>(args...);
            case 26: return makeNode<NodeType<26>>(args...);
            case 27: return makeNode<NodeType<27>>(args...);
            case 28: return makeNode<NodeType<28>>(args...);
            case 29: return makeNode<NodeType<29>>(args...);
            case 30: return makeNode<NodeType<30>>(args...);
            case 31: return makeNode<NodeType<31>>(args...);
            case 32: return makeNode<NodeType<32>>(args...);
            default:
                DAS_ASSERTF(0, "we should not even be here. we are calling makeNodeUnroll on a large number or a negative number."
                            "if its negative, there is some issue with the logic of count."
                            "if its large, bigger specialization for unroll should be added.");
                return nullptr;
            }
        }

#define MAX_FOR_UNROLL      32

        template < template <int TT> class NodeType, typename... Params>
        SimNode* makeNodeUnrollNZ_FOR(int count, Params... args) {
            switch (count) {
            case  1: return makeNode<NodeType< 1>>(args...);
            case  2: return makeNode<NodeType< 2>>(args...);
            case  3: return makeNode<NodeType< 3>>(args...);
            case  4: return makeNode<NodeType< 4>>(args...);
            case  5: return makeNode<NodeType< 5>>(args...);
            case  6: return makeNode<NodeType< 6>>(args...);
            case  7: return makeNode<NodeType< 7>>(args...);
            case  8: return makeNode<NodeType< 8>>(args...);
            case  9: return makeNode<NodeType< 9>>(args...);
            case 10: return makeNode<NodeType<10>>(args...);
            case 11: return makeNode<NodeType<11>>(args...);
            case 12: return makeNode<NodeType<12>>(args...);
            case 13: return makeNode<NodeType<13>>(args...);
            case 14: return makeNode<NodeType<14>>(args...);
            case 15: return makeNode<NodeType<15>>(args...);
            case 16: return makeNode<NodeType<16>>(args...);
            case 17: return makeNode<NodeType<17>>(args...);
            case 18: return makeNode<NodeType<18>>(args...);
            case 19: return makeNode<NodeType<19>>(args...);
            case 20: return makeNode<NodeType<20>>(args...);
            case 21: return makeNode<NodeType<21>>(args...);
            case 22: return makeNode<NodeType<22>>(args...);
            case 23: return makeNode<NodeType<23>>(args...);
            case 24: return makeNode<NodeType<24>>(args...);
            case 25: return makeNode<NodeType<25>>(args...);
            case 26: return makeNode<NodeType<26>>(args...);
            case 27: return makeNode<NodeType<27>>(args...);
            case 28: return makeNode<NodeType<28>>(args...);
            case 29: return makeNode<NodeType<29>>(args...);
            case 30: return makeNode<NodeType<30>>(args...);
            case 31: return makeNode<NodeType<31>>(args...);
            case 32: return makeNode<NodeType<32>>(args...);
            default:
                DAS_ASSERTF(0, "we should not even be here. we are calling makeNodeUnrollNZ8 on a large number or a negative number."
                    "if its negative, there is some issue with the logic of count."
                    "if its large, bigger specialization for unroll should be added.");
                return nullptr;
            }
        }
    };

    class DebugInfoAllocator final : public NodeAllocator {
    public:
        DebugInfoAllocator() {
            prefixWithHeader = false;
            initialSize = 1024;
        }
        virtual uint32_t grow ( uint32_t size ) override {
            return size;
        }
        char * allocateCachedName ( const string & name );
        das_hash_map<uint64_t,TypeInfo *>    lookup;
        das_hash_map<string, char *>         stringLookup;
        das_hash_map<string, wchar_t *>      stringWideLookup;
        uint64_t                             stringBytes = 0;
    };
}
