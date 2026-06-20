#pragma once

namespace das {

#if DAS_TRACK_ALLOCATIONS
    extern uint64_t    g_tracker;
    extern uint64_t    g_breakpoint;
    void das_track_breakpoint ( uint64_t id );
#endif

    #define DAS_PAGE_GC_MASK    0x8000000000000000ull
    #define DAS_MAX_SHOE_ALLOCATION     256
    #define DAS_MAX_SHOE_CUNKS          (DAS_MAX_SHOE_ALLOCATION>>4)

    template <typename T, typename = void>
    struct has_shrink_to_fit : false_type {};

    template <typename T>
    struct has_shrink_to_fit<T, void_t<decltype(declval<T&>().shrink_to_fit())>> : true_type {};


    struct LineInfo;

    struct Deck {
        // ne (entry count) and the byte totals are 64-bit: with geometric chunk
        // growth a single size-class chunk can exceed 4 GB, and `total * size` must
        // not wrap (the missed 64-bit migration that produced das_aligned_alloc16(0)).
        Deck( uint64_t ne, uint32_t es, Deck * n ) {
            total = (ne+31) & ~uint64_t(31);
            size = es;
            totalBytes = total * size;
            data = (char*) das_aligned_alloc16(totalBytes);
            bits = (uint32_t*) das_aligned_alloc16(total / 32 * 4);
            gc_bits = nullptr;
            reset();    // this reset before next
            next = n;
        }
        ~Deck ( ) {
            das_aligned_free16(data);
            das_aligned_free16(bits);
            if ( next ) delete next;
        }
        void reset() {
            memset ( bits, 0, total / 32 * 4);
            look = 0;
            allocated = 0;
            if ( next ) next->reset();
        }
        void beforeGC() {
            auto total_bytes = total / 32 * 4;
            if ( total_bytes ) {
                gc_bits = (uint32_t*) das_aligned_alloc16(total_bytes);
                memset ( gc_bits, 0, total_bytes);
            } else {
                gc_bits = nullptr;
            }
            look = 0;
            gc_allocated = 0;
            if ( next ) next->beforeGC();
        }
        void afterGC() {
            auto total_bytes = total / 32 * 4;
            if ( total_bytes ) {
                memcpy ( bits, gc_bits, total_bytes );
                das_aligned_free16 ( gc_bits );
                gc_bits = nullptr;
            }
            allocated = gc_allocated;
        }
        __forceinline bool isOwnPtr ( char * ptr ) const {
            return (ptr>=data) && (ptr<data+totalBytes);
        }
        __forceinline bool isAllocatedPtr ( char * ptr ) {
            ptrdiff_t idx = (ptr - data) / size;
            DAS_ASSERT ( idx>=0 && idx<ptrdiff_t(total) );
            uint64_t uidx = uint64_t(idx);
            uint64_t i = uidx >> 5;
            uint32_t j = uint32_t(uidx & 31);
            uint32_t b = bits[i];
            return ((b & (1u<<j))!=0);
        }
        __forceinline char * allocate ( ) {
            if ( allocated == total ) return nullptr;
            uint64_t maxt = total / 32;
            for ( uint64_t t=0; t!=maxt; ++t ) {
                uint32_t b = bits[look];
                uint32_t nb = ~b;
                if ( nb ) {
                    uint32_t j = 31 - das_clz(nb);
                    bits[look] = b | (1u<<j);
                    allocated ++;
                    uint64_t ofs = (look * 32 + j);
                    DAS_ASSERT(ofs < total);
                    return data + ofs * size;
                }
                look = look + 1;
                if ( look == maxt ) look = 0;
            }
            DAS_ASSERT(0 && "allocated reports room, but no bits are available");
            return nullptr;
        }
        __forceinline void free ( char * ptr ) {
            ptrdiff_t idx = (ptr - data) / size;
            DAS_ASSERT ( idx>=0 && idx<ptrdiff_t(total) );
            uint64_t uidx = uint64_t(idx);
            uint64_t i = uidx >> 5;
            uint32_t j = uint32_t(uidx & 31);
            uint32_t b = bits[i];
            DAS_ASSERT((b & (1u<<j))!=0 && "calling free on the pointer, which is already free");
            bits[i] = b ^ (1u<<j);
            look = i;
            allocated --;
        }
        __forceinline bool mark ( char * ptr ) {
            ptrdiff_t idx = (ptr - data) / size;
            DAS_ASSERT ( idx>=0 && idx<ptrdiff_t(total) );
            uint64_t uidx = uint64_t(idx);
            uint64_t i = uidx >> 5;
            uint32_t j = uint32_t(uidx & 31);
            uint32_t b = gc_bits[i];
            if ( !(b & (1u<<j)) ) {
                gc_bits[i] = b | (1u<<j);
                gc_allocated ++;
                return true;
            }
            return false;
        }
        char *      data = nullptr;
        uint32_t *  bits = nullptr;          // 32-bit bitset words; indices are 64-bit
        uint32_t *  gc_bits = nullptr;
        uint64_t    total = 0;               // entry count (can exceed 2^32 via growth)
        uint32_t    size = 0;                // element size in bytes (<= DAS_MAX_SHOE_ALLOCATION)
        uint64_t    totalBytes = 0;          // total * size — 64-bit so it cannot wrap
        uint64_t    look = 0;
        uint64_t    allocated = 0;
        uint64_t    gc_allocated = 0;
        Deck *      next = nullptr;
    };

    struct Shoe {
        Shoe () {
            lastChunk = nullptr;
            for ( int i=0; i!= DAS_MAX_SHOE_CUNKS; ++i ) {
                chunks[i] = nullptr;
            }
        }
        ~Shoe() {
            for ( int i=0; i!= DAS_MAX_SHOE_CUNKS; ++i ) {
                if ( chunks[i] ) delete chunks[i];
            }
        }
        void clear() {
            for ( int i=0; i!= DAS_MAX_SHOE_CUNKS; ++i ) {
                if ( chunks[i] ) delete chunks[i];
                chunks[i] = nullptr;
            }
        }
        void reset() {
            // TODO: modify watermarks
            for ( int i=0; i!= DAS_MAX_SHOE_CUNKS; ++i ) {
                if ( chunks[i] ) chunks[i]->reset();
            }
        }
        char * allocate ( uint32_t size ) {
            size = (size + 15) & ~15;
            DAS_ASSERT(size && size<=DAS_MAX_SHOE_ALLOCATION);
            uint32_t si = (size >> 4) - 1;
            for ( auto ch = chunks[si]; ch; ch=ch->next ) {
                if ( char * res = ch->allocate() ) {
                    return res;
                }
            }
            return nullptr;
        }
        void free ( char * ptr, uint32_t size ) {
            size = (size + 15) & ~15;
            DAS_ASSERT(size && size<=DAS_MAX_SHOE_ALLOCATION);
            uint32_t si = (size >> 4) - 1;
            for ( auto ch = chunks[si]; ch; ch=ch->next ) {
                if ( ch->isOwnPtr(ptr) ) {
                    ch->free(ptr);
                    return;
                }
            }
            DAS_FATAL_ERROR("deleting %p %i, which is not a chunk pointer (or chunk size mismatch)\n", (void *)ptr, size);
        }
        bool mark ( char * ptr, uint32_t size ) {
            size = (size + 15) & ~15;
            DAS_ASSERT(size && size<=DAS_MAX_SHOE_ALLOCATION);
            if ( lastChunk && lastChunk->isOwnPtr(ptr) ) {
                return lastChunk->mark(ptr);
            }
            uint32_t si = (size >> 4) - 1;
            for ( auto ch = chunks[si]; ch; ch=ch->next ) {
                if ( ch != lastChunk && ch->isOwnPtr(ptr) ) {
                    lastChunk = ch;
                    return ch->mark(ptr);
                }
            }
            return false;
        }
        void beforeGC() {
            for ( int i=0; i!=DAS_MAX_SHOE_CUNKS; ++i ) {
                if ( chunks[i] ) chunks[i]->beforeGC();
            }
        }
        bool isOwnPtr ( char * ptr, uint32_t size ) const {
            DAS_ASSERT(size && size<=DAS_MAX_SHOE_ALLOCATION);
            if ( lastChunk && lastChunk->isOwnPtr(ptr) ) {
                return true;
            }
            uint32_t si = (size >> 4) - 1;
            for ( auto ch = chunks[si]; ch; ch=ch->next ) {
                if ( ch != lastChunk && ch->isOwnPtr(ptr) ) {
                    lastChunk = ch;
                    return true;
                }
            }
            return false;
        }
        bool isAllocatedPtr ( char * ptr, uint32_t size ) const {
            DAS_ASSERT(size && size<=DAS_MAX_SHOE_ALLOCATION);
            if ( lastChunk && lastChunk->isOwnPtr(ptr) ) {
                return lastChunk->isAllocatedPtr(ptr);
            }
            uint32_t si = (size >> 4) - 1;
            for ( auto ch = chunks[si]; ch; ch=ch->next ) {
                if ( ch != lastChunk && ch->isOwnPtr(ptr) ) {
                    lastChunk = ch;
                    return ch->isAllocatedPtr(ptr);
                }
            }
            return false;
        }
        void getStats ( uint32_t & depth, uint32_t & pages, uint64_t & bytes, uint64_t & totalBytes ) const {
            depth = 0;
            bytes = 0;
            totalBytes = 0;
            pages = 0;
            for ( uint32_t si=0; si!= DAS_MAX_SHOE_CUNKS; ++si ) {
                uint32_t d = 0;
                for ( auto ch = chunks[si]; ch; ch=ch->next ) {
                    d ++;
                    pages ++;
                    bytes += uint64_t(ch->allocated) * uint64_t(ch->size);
                    totalBytes += uint64_t(ch->total) * uint64_t(ch->size) + (uint64_t(ch->total)/32*4);
                }
                depth = das::max(depth, d);
            }
        }
        uint32_t depth ( ) const {
            uint32_t d, p; uint64_t b, t;
            getStats(d, p, b, t);
            return d;
        }
        uint32_t totalChunks ( ) const {
            uint32_t d, p; uint64_t b, t;
            getStats(d, p, b, t);
            return p;
        }
        uint64_t bytesAllocated ( ) const {
            uint32_t d, p; uint64_t b, t;
            getStats(d, p, b, t);
            return b;
        }
        uint64_t totalBytesAllocated ( ) const {
            uint32_t d, p; uint64_t b, t;
            getStats(d, p, b, t);
            return t;
        }
        Deck *  chunks[DAS_MAX_SHOE_CUNKS];
        mutable Deck *  lastChunk;
    };

    typedef function<uint64_t(uint64_t)> CustomGrowFunction;

    struct DAS_API MemoryModel : ptr_ref_count {
        enum { default_initial_size = 65536 };
        MemoryModel(const MemoryModel &) = delete;
        MemoryModel & operator = (const MemoryModel &) = delete;
        MemoryModel ();
        virtual ~MemoryModel ();
        virtual void reset();
        virtual void shrink();
        void setInitialSize ( uint64_t size );
        uint64_t grow ( uint32_t si );
        virtual void sweep();
        char * allocate ( uint64_t size );
        bool free ( char * ptr, uint64_t size );
        char * reallocate ( char * ptr, uint64_t size, uint64_t nsize );
        __forceinline int depth() const { return shoe.depth(); }
        __forceinline bool isOwnPtr( char * ptr, uint64_t size ) const {
            if ( size<=maxShoeAllocation )
                return shoe.isOwnPtr(ptr,uint32_t(size));
            return (bigStuff.find(ptr)!=bigStuff.end());
        }
        __forceinline bool isAllocatedPtr( char * ptr, uint64_t size ) const {
            if ( size<=maxShoeAllocation )
                return shoe.isAllocatedPtr(ptr,uint32_t(size));
            return (bigStuff.find(ptr)!=bigStuff.end());
        }
        uint64_t bytesAllocated() const { return totalAllocated; }
        uint64_t maxBytesAllocated() const { return maxAllocated; }
        uint64_t totalAlignedMemoryAllocated() const;
        void setTrackAllocations ( bool on );
        __forceinline bool isTrackingAllocations() const { return trackAllocations; }
        CustomGrowFunction      customGrow;
        // Mask must be uint64 — `(size + alignMask) & ~alignMask` in allocate/free/reallocate
        // would otherwise zero-extend `~alignMask` from uint32 to uint64 as 0x00000000FFFFFFF0,
        // silently truncating any allocation ≥ 4 GB to its low 32 bits.
        uint64_t                alignMask;
        uint64_t                totalAllocated;
        uint64_t                maxAllocated;
        uint64_t                initialSize = 0;
        uint32_t                maxShoeAllocation = DAS_MAX_SHOE_ALLOCATION;
        bool                    trackAllocations = false;
        Shoe                    shoe;
        das_hash_map<void *,uint64_t> bigStuff;  // note: can't use char *, some stl implementations try hashing it as string
#if DAS_SANITIZER
        das_hash_map<void *,uint64_t> deletedBigStuff;
#endif
#if DAS_TRACK_ALLOCATIONS
        das_hash_map<void *,uint64_t> bigStuffId;
        das_hash_map<void *,const LineInfo *> bigStuffAt;
        das_hash_map<void *,const char *> bigStuffComment;
        __forceinline void mark_location ( void * ptr, const LineInfo * at ) {
            if ( trackAllocations ) bigStuffAt[ptr] = at;
        }
        __forceinline void mark_comment ( void * ptr, const char * what ) {
            if ( trackAllocations ) bigStuffComment[ptr] = what;
        }
        __forceinline const char * get_comment ( void * ptr ) const {
            if ( !trackAllocations ) return nullptr;
            auto it = bigStuffComment.find(ptr);
            return it != bigStuffComment.end() ? it->second : nullptr;
        }
#else
        __forceinline void mark_location ( void *, const LineInfo * ) {}
        __forceinline void mark_comment ( void *, const char * ) {}
        __forceinline const char * get_comment ( void * ) const { return nullptr; }
#endif
    };

    struct HeapChunk {
        __forceinline HeapChunk ( uint32_t s, HeapChunk * n ) {
            s = (s + 15) & ~15;
            data = (char *) das_aligned_alloc16(s);
            size = s;
            offset = 0;
            next = n;
        }
        ~HeapChunk() {
            das_aligned_free16(data);
            while (next) {
                HeapChunk * toDelete = next;
                next = toDelete->next;
                toDelete->next = nullptr;
                delete toDelete;
            }
        }
        __forceinline char * allocate ( uint32_t s ) {
            if ( uint64_t(offset) + s > size ) return nullptr;
            char * res = data + offset;
            offset += s;
            return res;
        }
        __forceinline void free ( char * ptr, uint32_t s ) {
            if ( ptr + s == data + offset ) {
                offset -= s;
            }
        }
        __forceinline bool isOwnPtr ( const char * ptr ) const {
            return (ptr>=data) && (ptr<data+size);
        }
        char *      data;
        uint32_t    size;
        uint32_t    offset;
        HeapChunk * next;
    };

    class DAS_API LinearChunkAllocator {
        enum { default_initial_size = 65536 };
    public:
        LinearChunkAllocator() { }
        virtual ~LinearChunkAllocator () { if ( chunk ) delete chunk; }
        char * allocate ( uint64_t s );
        void free ( char * ptr, uint64_t s );
        char * reallocate ( char * ptr, uint64_t size, uint64_t nsize );
        virtual void reset ();
        virtual void shrink ();
        char * allocateName ( const string & name );
        __forceinline bool isOwnPtrQnD ( const char * ptr ) const {
           return chunk!=nullptr && chunk->isOwnPtr(ptr);
        }
        __forceinline bool isOwnPtr ( const char * ptr ) const {
            for ( auto ch=chunk; ch; ch=ch->next ) {
                if ( ch->isOwnPtr(ptr) ) return true;
            }
            return false;
        }
        uint32_t depth() const;
        uint64_t bytesAllocated() const;
        uint64_t totalAlignedMemoryAllocated() const;
        __forceinline void setInitialSize ( uint64_t size ) {
            // HeapChunk allocation is uint32-bounded; LinearChunkAllocator never
            // serves >4GB single allocations (those go through PersistentHeapAllocator
            // / MemoryModel::bigStuff). Setting a larger initial size would silently
            // truncate when the first chunk is created — fail loud instead.
            DAS_VERIFYF(size <= UINT32_MAX, "LinearChunkAllocator::setInitialSize(%llu) exceeds the per-chunk uint32 cap", (unsigned long long)size);
            unadjustedInitialSize = size;
            initialSize = size;
        }
        virtual uint32_t grow ( uint32_t si );
    protected:
        void getStats ( uint32_t & depth, uint64_t & bytes, uint64_t & total ) const;
    public:
        CustomGrowFunction  customGrow;
        uint64_t    unadjustedInitialSize = 0;
        uint64_t    initialSize = 0;
        // uint64 — see MemoryModel::alignMask. `~alignMask` must be uint64 so the
        // `DAS_VERIFYF(s <= UINT32_MAX)` cap check in allocate() actually fires on
        // >4 GB requests instead of seeing a silently-truncated low-32-bit size.
        uint64_t    alignMask = 15;
        HeapChunk * chunk = nullptr;
    };

}
