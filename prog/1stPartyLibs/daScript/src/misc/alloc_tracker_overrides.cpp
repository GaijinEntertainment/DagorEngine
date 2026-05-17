// Global operator new/delete overrides for the daslang leak tracker.
//
// Compiled into every target participating in the leak audit (daslang,
// daslang_static, daslang-live, libDaScript_runtime, libDaScriptDyn_runtime);
// see CMakeLists.txt target_sources lines. The TU body is #if DAS_TRACK_ALLOC-
// gated, so it compiles to an empty object in Release/Debug. DAS_TRACK_ALLOC
// itself is set by generator expression to $<CONFIG:RelWithDebInfo>, i.e. the
// tracker is permanently on in RelWithDebInfo and off everywhere else -- no
// user-facing CMake option.
//
// The Windows DLL model resolves operator new per-module against each module's
// CRT, so a single override living in libDaScriptDyn_runtime only covers its
// own code; the exe and every shared module need their own copy.
//
// Uses std::malloc/std::free (not _aligned_malloc) for the backing store so
// cross-module mismatches (e.g. allocated here, freed through a module that
// doesn't share our override) stay ABI-compatible with the default CRT
// allocator and don't corrupt the heap. The tracker's own map records and
// track_free_hook reports orphan frees for diagnostic visibility.
//
// Full overload set: C++14 sized delete and C++17 aligned new/delete are
// replaced alongside the classic unsized forms so every compiler-emitted
// deallocation funnels through track_free_hook. A missing overload would let
// the default runtime free the pointer directly, bypassing the hook and
// leaving a stale entry that would be reported as a (false) leak.

#include "daScript/misc/platform.h"

#if DAS_TRACK_ALLOC

#include <cstdlib>
#include <cstdint>
#include <new>

// -----------------------------------------------------------------------------
// Aligned allocation helper. std::aligned_alloc is POSIX-only and requires size
// to be a multiple of alignment. On MSVC we use _aligned_malloc / _aligned_free.
// Aligned pointers must be freed through the matching aligned API (the CRT
// stores bookkeeping before the returned pointer). We track the allocation in
// the usual map so track_free_hook fires as for any other alloc.
// -----------------------------------------------------------------------------

static void * track_new_impl(size_t size) {
    void *p = std::malloc(size ? size : 1);
    das::track_alloc_hook(p, size);
    return p;
}

static void track_delete_impl(void *p) noexcept {
    if (!p) return;
    das::track_free_hook(p);
    std::free(p);
}

#if defined(_MSC_VER)
#  include <malloc.h>
static void * track_new_aligned_impl(size_t size, size_t alignment) {
    if (size == 0) size = 1;
    if (alignment < sizeof(void*)) alignment = sizeof(void*);
    void *p = _aligned_malloc(size, alignment);
    das::track_alloc_hook(p, size);
    return p;
}
static void track_delete_aligned_impl(void *p) noexcept {
    if (!p) return;
    das::track_free_hook(p);
    _aligned_free(p);
}
#else
static void * track_new_aligned_impl(size_t size, size_t alignment) {
    if (size == 0) size = alignment;
    // posix_memalign / aligned_alloc both require alignment to be a power of
    // two and a multiple of sizeof(void*).
    if (alignment < sizeof(void*)) alignment = sizeof(void*);
    // aligned_alloc requires size to be a multiple of alignment.
    size_t rounded = (size + alignment - 1) & ~(alignment - 1);
    void *p = std::aligned_alloc(alignment, rounded);
    das::track_alloc_hook(p, size);
    return p;
}
static void track_delete_aligned_impl(void *p) noexcept {
    if (!p) return;
    das::track_free_hook(p);
    std::free(p);
}
#endif

// -----------------------------------------------------------------------------
// Plain new / delete (unsized)
// -----------------------------------------------------------------------------

void * operator new  (size_t size)                           { return track_new_impl(size); }
void * operator new[](size_t size)                           { return track_new_impl(size); }

void * operator new  (size_t size, std::nothrow_t const &) noexcept { return track_new_impl(size); }
void * operator new[](size_t size, std::nothrow_t const &) noexcept { return track_new_impl(size); }

#ifdef __APPLE__
void operator delete  (void *p) _NOEXCEPT                       { track_delete_impl(p); }
void operator delete[](void *p) _NOEXCEPT                       { track_delete_impl(p); }
void operator delete  (void *p, std::nothrow_t const &) _NOEXCEPT { track_delete_impl(p); }
void operator delete[](void *p, std::nothrow_t const &) _NOEXCEPT { track_delete_impl(p); }
#else
void operator delete  (void *p) noexcept                        { track_delete_impl(p); }
void operator delete[](void *p) noexcept                        { track_delete_impl(p); }
void operator delete  (void *p, std::nothrow_t const &) noexcept { track_delete_impl(p); }
void operator delete[](void *p, std::nothrow_t const &) noexcept { track_delete_impl(p); }
#endif

// Sized delete (C++14). MSVC emits calls to these whenever it knows the static
// size of the type being deleted; without our replacement the default CRT
// deallocator runs and bypasses track_free_hook.
void operator delete  (void *p, size_t /*size*/) noexcept       { track_delete_impl(p); }
void operator delete[](void *p, size_t /*size*/) noexcept       { track_delete_impl(p); }

// -----------------------------------------------------------------------------
// Aligned new / delete (C++17). Emitted for types whose alignof exceeds
// __STDCPP_DEFAULT_NEW_ALIGNMENT__ (16 on x64). Separate allocator because
// MSVC's _aligned_malloc stores bookkeeping before the returned pointer.
// -----------------------------------------------------------------------------

void * operator new  (size_t size, std::align_val_t al)                             { return track_new_aligned_impl(size, (size_t)al); }
void * operator new[](size_t size, std::align_val_t al)                             { return track_new_aligned_impl(size, (size_t)al); }
void * operator new  (size_t size, std::align_val_t al, std::nothrow_t const &) noexcept { return track_new_aligned_impl(size, (size_t)al); }
void * operator new[](size_t size, std::align_val_t al, std::nothrow_t const &) noexcept { return track_new_aligned_impl(size, (size_t)al); }

void operator delete  (void *p, std::align_val_t) noexcept                                  { track_delete_aligned_impl(p); }
void operator delete[](void *p, std::align_val_t) noexcept                                  { track_delete_aligned_impl(p); }
void operator delete  (void *p, std::align_val_t, std::nothrow_t const &) noexcept          { track_delete_aligned_impl(p); }
void operator delete[](void *p, std::align_val_t, std::nothrow_t const &) noexcept          { track_delete_aligned_impl(p); }
void operator delete  (void *p, size_t /*size*/, std::align_val_t) noexcept                 { track_delete_aligned_impl(p); }
void operator delete[](void *p, size_t /*size*/, std::align_val_t) noexcept                 { track_delete_aligned_impl(p); }

#endif // DAS_TRACK_ALLOC
