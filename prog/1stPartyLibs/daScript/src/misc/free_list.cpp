#include "daScript/misc/platform.h"

#include "daScript/misc/free_list.h"

#if DAS_FREE_LIST

namespace das {

struct ReuseChunk {
    ReuseChunk * next;
};

struct ReuseCache {
    ReuseChunk *    hold[DAS_MAX_BUCKET_COUNT];
};

DAS_THREAD_LOCAL ReuseCache * tlsReuseCache = nullptr;
DAS_THREAD_LOCAL uint32_t tlsReuseCacheCount = 0;

void reuse_cache_push() {
    if ( tlsReuseCacheCount==0 ) reuse_cache_create();
    tlsReuseCacheCount ++;
}

void reuse_cache_pop() {
    tlsReuseCacheCount --;
    if ( tlsReuseCacheCount==0 ) reuse_cache_destroy();
}

void * reuse_cache_allocate ( size_t size ) {
    if ( size==0 ) return nullptr;
    size = (size+15) & ~15;
    if ( size<=DAS_MAX_REUSE_SIZE && tlsReuseCache ) {
        auto bucket = (size >> 4) - 1;
        auto & hold = tlsReuseCache->hold[bucket];
        if ( hold ) {
            void * data = hold;
            hold = hold->next;
            return data;
        } else {
            return das_aligned_alloc16(size);
        }
    } else {
        return das_aligned_alloc16(size);
    }
}

void reuse_cache_free ( void * ptr, size_t size ) {
    size = (size+15) & ~15;
    if ( size<=DAS_MAX_REUSE_SIZE && tlsReuseCache ) {
        auto bucket = (size >> 4) - 1;
        auto & hold = tlsReuseCache->hold[bucket];
        auto next = hold;
        hold = (ReuseChunk *) ptr;
        hold->next = next;
    } else {
        return das_aligned_free16(ptr);
    }
}

void reuse_cache_free ( void * ptr ) {
    return reuse_cache_free(ptr, das_aligned_memsize(ptr));
}

void reuse_cache_create() {
    if ( !tlsReuseCache ) {
        tlsReuseCache = (ReuseCache *) das_aligned_alloc16(sizeof(ReuseCache));
        memset(tlsReuseCache, 0, sizeof(ReuseCache));
    }
}

void reuse_cache_clear() {
    if ( tlsReuseCache ) {
        for ( size_t bucket=0; bucket!=DAS_MAX_BUCKET_COUNT; ++bucket ) {
            ReuseChunk * & hold = tlsReuseCache->hold[bucket];
            while ( hold ) {
                auto ptr = hold;
                hold = hold->next;
                das_aligned_free16(ptr);
            }
        }
    }
}

void reuse_cache_destroy() {
    if ( tlsReuseCache ) {
        reuse_cache_clear();
        das_aligned_free16(tlsReuseCache);
        tlsReuseCache = nullptr;
    }
}

}

#if defined(__linux__) && !defined(DAS_NO_GLOBAL_NEW_AND_DELETE)
#define DAS_NO_GLOBAL_NEW_AND_DELETE
#endif

#if !defined(DAS_NO_GLOBAL_NEW_AND_DELETE)

void * operator new ( size_t size ) {
    return das::reuse_cache_allocate(size);
}
void * operator new[] ( size_t size ) {
    return das::reuse_cache_allocate(size);
}

#ifdef __APPLE__

void operator delete ( void * data ) _NOEXCEPT {
    if ( data ) das::reuse_cache_free(data);
}
void operator delete[] ( void * data ) _NOEXCEPT {
    if ( data ) das::reuse_cache_free(data);
}

#else

void operator delete ( void * data ) {
    if ( data ) das::reuse_cache_free(data);
}
void operator delete[] ( void * data ) {
    if ( data ) das::reuse_cache_free(data);
}

#endif


#endif

#else

namespace das {

void reuse_cache_push() {}
void reuse_cache_pop() {}

}

#endif