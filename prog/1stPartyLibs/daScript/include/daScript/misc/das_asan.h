#pragma once

#ifndef DAS_ASAN
    #if defined(__SANITIZE_ADDRESS__)
        #define DAS_ASAN 1
    #elif defined(__has_feature)
        #if __has_feature(address_sanitizer)
            #define DAS_ASAN 1
        #endif
    #endif
#endif
#ifndef DAS_ASAN
    #define DAS_ASAN 0
#endif

#if DAS_ASAN
    #include <sanitizer/asan_interface.h>
    #include <sanitizer/common_interface_defs.h>
    #ifndef DAS_ASAN_REDZONE
        #define DAS_ASAN_REDZONE 16
    #endif
    #define DAS_ASAN_POISON(ptr,size)   __asan_poison_memory_region((const volatile void *)(ptr), size_t(size))
    #define DAS_ASAN_UNPOISON(ptr,size) __asan_unpoison_memory_region((const volatile void *)(ptr), size_t(size))
    #define DAS_ASAN_ANNOTATE_ARRAY(arr,stride,oldSize,newSize) do { \
        if ( (arr).data && (stride) && !(arr).borrowed && (oldSize) != (newSize) ) { \
            __sanitizer_annotate_contiguous_container((const void *)(arr).data, (const void *)((arr).data + (arr).capacity*(stride)), \
                (const void *)((arr).data + (oldSize)*(stride)), (const void *)((arr).data + (newSize)*(stride))); \
        } \
    } while (0)
    #if defined(_MSC_VER) && !defined(__clang__)
        #define DAS_NO_ASAN __declspec(no_sanitize_address)
    #else
        #define DAS_NO_ASAN __attribute__((no_sanitize_address))
    #endif
#else
    #define DAS_ASAN_REDZONE 0
    #define DAS_ASAN_POISON(ptr,size)   ((void)(ptr), (void)(size))
    #define DAS_ASAN_UNPOISON(ptr,size) ((void)(ptr), (void)(size))
    #define DAS_ASAN_ANNOTATE_ARRAY(arr,stride,oldSize,newSize) ((void)(arr), (void)(stride), (void)(oldSize), (void)(newSize))
    #define DAS_NO_ASAN
#endif
