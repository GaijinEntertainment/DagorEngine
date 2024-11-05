//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_hash.h>
#include <EASTL/string_view.h>
#include <generic/dag_span.h>
#include "ecsForEachWithArg.h"


#define __ENUM_TO_STR(Enum, x) \
  case Enum::x:                \
    return #x;                 \
    break;
#define __STR_TO_ENUM(Enum, x) \
  case #x##_h:                 \
    retval = Enum::x;          \
    return true;               \
    break; //_h will use mem_hash_fnv1


#define __ENUM_TO_STR_FUNC(Enum, ...)                         \
  inline constexpr const char *enum_to_str(Enum eval)         \
  {                                                           \
    switch (eval)                                             \
    {                                                         \
      ECS_FOR_EACH_WITH_ARG(__ENUM_TO_STR, Enum, __VA_ARGS__) \
      default: return "";                                     \
    }                                                         \
  }

#define __STR_TO_ENUM_FUNC(Enum, ...)                                  \
  inline bool str_to_enum(const eastl::string_view &str, Enum &retval) \
  {                                                                    \
    switch (mem_hash_fnv1(str.data(), str.size()))                     \
    {                                                                  \
      ECS_FOR_EACH_WITH_ARG(__STR_TO_ENUM, Enum, __VA_ARGS__)          \
      default: return false;                                           \
    }                                                                  \
  }

// names of registered enum values
template <typename T>
inline dag::ConstSpan<const char *> get_enum_names();

#define __ENUM_VALUE(Enum, x) Enum::x,
#define __ENUM_NAME(Enum, x)  #x,

#define __CHANGE_ENUM_FUNC(Enum, ...)                                                \
  inline void change_enum_values(Enum &retval, int enum_idx)                         \
  {                                                                                  \
    static Enum values[] = {ECS_FOR_EACH_WITH_ARG(__ENUM_VALUE, Enum, __VA_ARGS__)}; \
    retval = values[enum_idx];                                                       \
  }

#define __GET_ENUM_NAMES_FUNC(Enum, ...)                                                   \
  template <>                                                                              \
  inline dag::ConstSpan<const char *> get_enum_names<Enum>()                               \
  {                                                                                        \
    static const char *values[] = {ECS_FOR_EACH_WITH_ARG(__ENUM_NAME, Enum, __VA_ARGS__)}; \
    return make_span_const(values, sizeof(values) / sizeof(values[0]));                    \
  }

#define __FIND_ENUM_INDEX_FUNC(Enum, ...)                                            \
  inline int find_enum_index(Enum value)                                             \
  {                                                                                  \
    static Enum values[] = {ECS_FOR_EACH_WITH_ARG(__ENUM_VALUE, Enum, __VA_ARGS__)}; \
    for (int i = 0, n = sizeof(values) / sizeof(values[0]); i < n; i++)              \
      if (values[i] == value)                                                        \
        return i;                                                                    \
    return -1;                                                                       \
  }

#define ECS_DECLARE_ENUM(etype, ...)        \
  __ENUM_TO_STR_FUNC(etype, __VA_ARGS__)    \
  __STR_TO_ENUM_FUNC(etype, __VA_ARGS__)    \
  __CHANGE_ENUM_FUNC(etype, __VA_ARGS__)    \
  __GET_ENUM_NAMES_FUNC(etype, __VA_ARGS__) \
  __FIND_ENUM_INDEX_FUNC(etype, __VA_ARGS__)
