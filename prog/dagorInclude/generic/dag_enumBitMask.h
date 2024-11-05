//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>

/*
 * Usage:
 *
 * enum class Foo { A, B, C};
 * DAGOR_ENABLE_ENUM_BITMASK(Foo);
 * void do_stuff() { Foo foo = Foo::A|Foo::B; ... }
 *
 * For classes/structs DAGOR_ENABLE_ENUM_BITMASK() statement should be outside of class declaration:
 *
 * class Bar { public: enum class Foo { A, B, C}; };
 * DAGOR_ENABLE_ENUM_BITMASK(Bar::Foo);
 *
 */

/*
 * Note on implementation:
 * pure template-based approach was choosen because MSVC (15 at least) does not supports expression like:
 * std::enable_if<decltype(has_some_property(declval<E>())::value),E>::type
 * which required for function-based implementation
 * And struct specialization approach doesn't supports non global enums.
 */

#define DAGOR_ENABLE_ENUM_BITMASK(Enum)                                                                                               \
  static_assert(eastl::is_enum<Enum>::value, #Enum " is not enum!");                                                                  \
  inline Enum operator|(Enum lhs, Enum rhs)                                                                                           \
  {                                                                                                                                   \
    return static_cast<Enum>(                                                                                                         \
      static_cast<eastl::underlying_type<Enum>::type>(lhs) | static_cast<eastl::underlying_type<Enum>::type>(rhs));                   \
  }                                                                                                                                   \
  inline Enum operator&(Enum lhs, Enum rhs)                                                                                           \
  {                                                                                                                                   \
    return static_cast<Enum>(                                                                                                         \
      static_cast<eastl::underlying_type<Enum>::type>(lhs) & static_cast<eastl::underlying_type<Enum>::type>(rhs));                   \
  }                                                                                                                                   \
  inline Enum operator^(Enum lhs, Enum rhs)                                                                                           \
  {                                                                                                                                   \
    return static_cast<Enum>(                                                                                                         \
      static_cast<eastl::underlying_type<Enum>::type>(lhs) ^ static_cast<eastl::underlying_type<Enum>::type>(rhs));                   \
  }                                                                                                                                   \
  inline Enum operator~(Enum rhs)                                                                                                     \
  {                                                                                                                                   \
    return static_cast<Enum>(~static_cast<eastl::underlying_type<Enum>::type>(rhs));                                                  \
  }                                                                                                                                   \
  inline Enum &operator|=(Enum &lhs, Enum rhs)                                                                                        \
  {                                                                                                                                   \
    lhs =                                                                                                                             \
      static_cast<Enum>(static_cast<eastl::underlying_type<Enum>::type>(lhs) | static_cast<eastl::underlying_type<Enum>::type>(rhs)); \
                                                                                                                                      \
    return lhs;                                                                                                                       \
  }                                                                                                                                   \
  inline Enum &operator&=(Enum &lhs, Enum rhs)                                                                                        \
  {                                                                                                                                   \
    lhs =                                                                                                                             \
      static_cast<Enum>(static_cast<eastl::underlying_type<Enum>::type>(lhs) & static_cast<eastl::underlying_type<Enum>::type>(rhs)); \
                                                                                                                                      \
    return lhs;                                                                                                                       \
  }                                                                                                                                   \
  inline Enum &operator^=(Enum &lhs, Enum rhs)                                                                                        \
  {                                                                                                                                   \
    lhs =                                                                                                                             \
      static_cast<Enum>(static_cast<eastl::underlying_type<Enum>::type>(lhs) ^ static_cast<eastl::underlying_type<Enum>::type>(rhs)); \
                                                                                                                                      \
    return lhs;                                                                                                                       \
  }
