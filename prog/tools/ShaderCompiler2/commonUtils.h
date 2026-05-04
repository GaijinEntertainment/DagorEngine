// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <utility>
#include <util/dag_safeArg.h>
#include <util/dag_globDef.h>
#include <EASTL/string.h>
#include <EASTL/optional.h>

#define COPYABLE_TYPE(type_)           \
  type_(const type_ &other) = default; \
  type_ &operator=(const type_ &other) = default;

#define MOVABLE_TYPE(type_)       \
  type_(type_ &&other) = default; \
  type_ &operator=(type_ &&other) = default;

#define NON_COPYABLE_TYPE(type_)      \
  type_(const type_ &other) = delete; \
  type_ &operator=(const type_ &other) = delete;

#define MOVE_ONLY_TYPE(type_) MOVABLE_TYPE(type_) NON_COPYABLE_TYPE(type_)

#define PINNED_TYPE(type_)                       \
  type_(const type_ &other) = delete;            \
  type_ &operator=(const type_ &other) = delete; \
  type_(type_ &&other) = delete;                 \
  type_ &operator=(type_ &&other) = delete;

#define LITSTR_LEN(str_) (countof(str_) - 1)

template <typename T>
inline bool item_is_in(const T &item, std::initializer_list<T> items)
{
  for (const T &ref : items)
  {
    if (ref == item)
      return true;
  }
  return false;
}

template <class T>
inline T unwrap(eastl::optional<T> &&opt)
{
  G_ASSERT(opt.has_value());
  return eastl::move(opt.value());
}

template <class... TArgs>
inline eastl::string string_f(const char *fmt, TArgs &&...args)
{
  return eastl::string(eastl::string::CtorSprintf{}, fmt, eastl::forward<TArgs>(args)...);
}

inline eastl::string string_dsa(const char *fmt, const DagorSafeArg *arg, int anum)
{
  int slen = DagorSafeArg::count_len(fmt, arg, anum);
  eastl::string msg{};
  msg.resize(slen);
  DagorSafeArg::print_fmt(msg.data(), msg.length() + 1, fmt, arg, anum);
  return msg;
}

inline bool streq(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }

inline void hash_combine(size_t &seed, size_t value) { seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2); };

// @TODO: use concept for return type once we have a stably available 'concepts' header
template <typename T>
concept Hashable = requires(T a) {
  { eastl::hash<T>{}(a) };
} && eastl::is_convertible_v<decltype(eastl::hash<T>{}(eastl::declval<T>())), size_t>;

template <Hashable T>
inline void hash_into(size_t &seed, const T &val)
{
  hash_combine(seed, eastl::hash<T>{}(val));
}

inline bool path_is_abs(const char *path)
{
  G_ASSERT(path);
#if _TARGET_PC_WIN
  if (strchr(path, ':'))
    return true;
#endif
  return path[0] == '/';
}

template <class T>
inline constexpr T nbitmask(int nb)
{
  return (T{1} << nb) - T{1};
}

template <class T>
inline constexpr T nbitmask(int nb, int shift)
{
  return ((T{1} << nb) - T{1}) << shift;
}
