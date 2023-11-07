#pragma once

#ifndef DAS_SKA_HASH
#define DAS_SKA_HASH    1
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

#if DAS_SKA_HASH
#ifdef _MSC_VER
#pragma warning(disable:4503)    // decorated name length exceeded, name was truncated
#endif
#include <ska/flat_hash_map.hpp>
namespace das {
template <typename K, typename V, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_map = das_ska::flat_hash_map<K,V,H,E>;
template <typename K, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_set = das_ska::flat_hash_set<K,H,E>;
template <typename K, typename V, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_hash_map = das_ska::flat_hash_map<K,V,H,E>;
template <typename K, typename H = das::hash<K>, typename E = das::equal_to<K>>
using das_hash_set = das_ska::flat_hash_set<K,H,E>;
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
template <typename K, typename V>
using das_safe_map = std::map<K,V>;
template <typename K, typename C=das::less<K>>
using das_safe_set = std::set<K,C>;
}
#endif

#define DAS_STD_HAS_BIND 1

// if enabled, the generated interop will be marginally slower
// the upside is that it well generate significantly less templated code, thus reducing compile time (and binary size)
#ifndef DAS_SLOW_CALL_INTEROP
  #define DAS_SLOW_CALL_INTEROP 0
#endif

#ifndef DAS_MAX_FUNCTION_ARGUMENTS
#define DAS_MAX_FUNCTION_ARGUMENTS 32
#endif

#ifndef DAS_FUSION
  #define DAS_FUSION  0
#endif

#if DAS_SLOW_CALL_INTEROP
  #undef DAS_FUSION
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


#ifndef das_to_stdout
#define das_to_stdout(...) { fprintf(stdout, __VA_ARGS__); fflush(stdout); }
#endif

#ifndef das_to_stderr
#define das_to_stderr(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); }
#endif
