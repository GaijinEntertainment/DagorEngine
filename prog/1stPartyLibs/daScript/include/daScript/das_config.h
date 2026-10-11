#pragma once

// Pre-C++20 inclusion order note: this header is pulled in by platform.h line 94,
// before platform.h defines __forceinline / DAS_SUPPRESS_UB / NO_ASAN_INLINE.
// Any header we #include here must either not use those macros, or provide its
// own defensive fallbacks. See das_hash_map/das_hash_map.h and misc/anyhash.h.

// If set to 0, falls back to std::unordered_map. Default is our in-tree map.
#ifndef DAS_CUSTOM_HASH
#define DAS_CUSTOM_HASH 1
#endif

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <string>
#include <memory>
#include <vector>
#include <type_traits>
#include <initializer_list>
#include <functional>
#include <algorithm>
#include <functional>
#include <climits>

#include <limits.h>
#include <setjmp.h>

#include <mutex>

namespace das {using namespace std;}


// one of three identical copies - include/daScript/das_config.h, include/misc/include_fmt.h,
// cmake/das_config_eastl/daScript/das_config.h; whichever a TU includes first must stand alone
namespace das {
  void das_throw(const char * msg);
}
#if (!defined(DAS_ENABLE_EXCEPTIONS)) || (!DAS_ENABLE_EXCEPTIONS)
#ifndef DAS_FMT_THROW_DEFINED
#define DAS_FMT_THROW_DEFINED
namespace das {
  void das_stash_throw(const char * msg);
  void das_throw_stashed();
  // the panic longjmp never unwinds (POSIX never does; the JIT stack cannot be unwound);
  // the temporary dies when the stash statement ends - the jump crosses nothing live
  template <typename BuildAndStash>
  inline void das_stash_then_throw(BuildAndStash && buildAndStash) {
      buildAndStash();
      das_throw_stashed();
  }
}
#define FMT_THROW(x)    das::das_stash_then_throw([&]{ das::das_stash_throw(((x).what())); })
#endif
#endif

#if DAS_CUSTOM_HASH
#include <das_hash_map/das_hash_map.h>
namespace das {
template <typename K, typename V, typename H = das::daslang_hash<K>, typename E = das::equal_to<K>>
using das_map = das::daslang_hash_map<K,V,H,E>;
template <typename K, typename H = das::daslang_hash<K>, typename E = das::equal_to<K>>
using das_set = das::daslang_hash_set<K,H,E>;
template <typename K, typename V, typename H = das::daslang_hash<K>, typename E = das::equal_to<K>>
using das_hash_map = das::daslang_hash_map<K,V,H,E>;
template <typename K, typename H = das::daslang_hash<K>, typename E = das::equal_to<K>>
using das_hash_set = das::daslang_hash_set<K,H,E>;
// Insert-only / grow-only flavors — same API minus erase(). Saves one branch
// per probe step in insert and one in iterator skip; signals to readers that
// the table is grow-only by design.
template <typename K, typename V, typename H = das::daslang_hash<K>, typename E = das::equal_to<K>>
using das_insert_only_map = das::daslang_insert_only_hash_map<K,V,H,E>;
template <typename K, typename H = das::daslang_hash<K>, typename E = das::equal_to<K>>
using das_insert_only_set = das::daslang_insert_only_hash_set<K,H,E>;
template <typename K, typename V, typename H = das::daslang_hash<K>, typename E = das::equal_to<K>>
using das_insert_only_hash_map = das::daslang_insert_only_hash_map<K,V,H,E>;
template <typename K, typename H = das::daslang_hash<K>, typename E = das::equal_to<K>>
using das_insert_only_hash_set = das::daslang_insert_only_hash_set<K,H,E>;
template <typename K, typename V>
using das_safe_map = std::map<K,V>;
template <typename K, typename C=das::less<K>>
using das_safe_set = std::set<K,C>;
}
#else
namespace das {
template <typename K, typename V, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_map = std::unordered_map<K,V,H,E>;
template <typename K, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_set = std::unordered_set<K,H,E>;
template <typename K, typename V, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_hash_map = std::unordered_map<K,V,H,E>;
template <typename K, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_hash_set = std::unordered_set<K,H,E>;
// Insert-only variants degrade gracefully to std::unordered_* when DAS_CUSTOM_HASH=0
// (no insert-only stdlib equivalent — caller-visible API is a superset, so callers
// that don't use erase will compile both ways).
template <typename K, typename V, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_insert_only_map = std::unordered_map<K,V,H,E>;
template <typename K, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_insert_only_set = std::unordered_set<K,H,E>;
template <typename K, typename V, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_insert_only_hash_map = std::unordered_map<K,V,H,E>;
template <typename K, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_insert_only_hash_set = std::unordered_set<K,H,E>;
template <typename K, typename V>
using das_safe_map = std::map<K,V>;
template <typename K, typename C=das::less<K>>
using das_safe_set = std::set<K,C>;
}
#endif

#define DAS_STD_HAS_BIND 1

#ifndef DAS_MAX_FUNCTION_ARGUMENTS
#define DAS_MAX_FUNCTION_ARGUMENTS 32
#endif

#ifndef DAS_FUSION
  #define DAS_FUSION  0
#endif

#ifndef DAS_DEBUGGER
  #define DAS_DEBUGGER  1
#endif

#ifndef DAS_BIND_EXTERNAL
  #if defined(_WIN32) && defined(_WIN64)
    #define DAS_BIND_EXTERNAL 1
  #elif defined(__APPLE__)
    #define DAS_BIND_EXTERNAL 1
  #elif defined(__linux__)
    #define DAS_BIND_EXTERNAL 1
  #elif defined __HAIKU__
    #define DAS_BIND_EXTERNAL 1
  #else
    #define DAS_BIND_EXTERNAL 0
  #endif
#endif

#ifndef DAS_PRINT_VEC_SEPARATROR
#define DAS_PRINT_VEC_SEPARATROR ","
#endif


#ifndef DAS_USE_BASE_CRASH_HANDLER
#if defined(_WIN32) || ((defined(__linux__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__))
#define DAS_USE_BASE_CRASH_HANDLER 1
#else
#define DAS_USE_BASE_CRASH_HANDLER 0
#endif
#endif

#ifndef das_to_stdout_level_prefix_text
#define das_to_stdout_level_prefix_text(level, prefix, text) { if (level >= LogLevel::error) { fprintf(stderr, "%s", text); fflush(stderr); } else { fprintf(stdout, "%s%s", prefix, text); fflush(stdout); } }
#endif
