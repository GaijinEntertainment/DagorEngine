#pragma once

#ifndef DAS_FREE_LIST
#define DAS_FREE_LIST   0
#endif

#if DAS_FREE_LIST

namespace das {
    class FreeList {
    public:
        FreeList( uint32_t os, uint32_t gs = 64 ) {
            objectSize = os;
            growSpeed = gs;
        }
        ~FreeList() {
            clear();
        }
        void clear() {
            DAS_ASSERT(totalObjects == 0);
            for ( auto chunk : chunks ) {
                das_aligned_free16(chunk);
            }
            chunks.clear();
        }
        __forceinline char * allocate () {
            if ( !freeList ) grow();
            char * result = freeList;
            freeList = *(char **)freeList;
            totalObjects ++;
            return result;
        }
        __forceinline void free ( char * ptr ) {
            *(char **)ptr = freeList;
            freeList = ptr;
            DAS_ASSERT(totalObjects > 0);
            totalObjects --;
        }
        void grow() {
            char * chunk = (char *) das_aligned_alloc16(objectSize * growSpeed);
            chunks.push_back(chunk);
            char * head = chunk;
            char * last = chunk + (growSpeed-1) * objectSize;
            while ( head != last ) {
                *((char **)head) = head + objectSize;
                head += objectSize;
            }
            *((char **)head) = freeList;
            freeList = chunk;
        }
    protected:
        char *          freeList = nullptr;
        uint32_t        objectSize = 0;
        uint32_t        growSpeed = 64;
        uint64_t        totalObjects = 0;
        vector<char *>  chunks;
    };
}

#endif

#define DAS_MAX_REUSE_SIZE      256
#define DAS_MAX_BUCKET_COUNT    (DAS_MAX_REUSE_SIZE>>4)

namespace das {
    void * reuse_cache_allocate ( size_t size );
    void reuse_cache_free ( void * ptr, size_t size );
    void reuse_cache_free ( void * ptr );
    void reuse_cache_create();
    void reuse_cache_clear();
    void reuse_cache_destroy();
    void reuse_cache_push();
    void reuse_cache_pop();

    struct ReuseCacheGuard {
        ReuseCacheGuard() { reuse_cache_push(); }
        ~ReuseCacheGuard() { reuse_cache_pop(); }
    };
}
