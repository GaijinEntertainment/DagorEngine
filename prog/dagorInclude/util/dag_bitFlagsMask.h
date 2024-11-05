//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <type_traits>

/*
  BitFlagsMask is a zero-cost and type safe handler for bit-flags operations

  enum class MyFlags
  {
    A = 1,
    B = 2,
    C = 4
  };

  using MyFlagsMask = BitFlagsMask<MyFlags>;
  MyFlagsMask mask1 = MyFlags::A | MyFlags::B;
  mask1 ^= (MyFlags::C | MyFlags::B);

  BitFlagsMask is a constexpr class so you can use it in compile-time
  expressions

  switch (mask1.asInteger())
  {
    case ((MyFlags::A | MyFlags::C).asInteger():
      ...
    case ~MyFlagsMask().asInteger():
      ...
  }

  int foo(MyFlagsMask mask);

  foo(MyFlags::A | MyFlags::B); // OK
  foo(MyFlags::A | 142);        // Fail
  foo(OtherFlags::SomeFlag);    // Fail

  In order to use operator~ to invert all flags in mask you must define template
  specialization for BitFlagsTraits that join all used bit flags in one
  constant. If one day C++ will get reflexion this traits won't be needed anymore

    template<>
    struct BitFlagsTraits<MyFlags>
    {
      static constexpr auto allFlags = MyFlags::A | MyFlags::B | MyFlags::C;
    };

  if you intend to use enum class, you need to define operator| for that enum

    constexpr BitFlagsMask<MyFlags> operator|(MyFlags flag1, MyFlags flag2)
    {
      return make_bitmask(flag1) | flag2;
    }

  or just use macro BITMASK_DECLARE_FLAGS_OPERATORS(MyFlags)
*/

template <typename BitFlags>
struct BitFlagsTraits;

template <typename BitFlags>
class BitFlagsMask
{
public:
  typedef typename std::underlying_type<BitFlags>::type MaskType;

  constexpr BitFlagsMask() : mask(0) {}

  constexpr BitFlagsMask(BitFlags bit) : mask(static_cast<MaskType>(bit)) {}

  BitFlagsMask<BitFlags> &operator|=(BitFlagsMask<BitFlags> const &rhs)
  {
    mask |= rhs.mask;
    return *this;
  }

  BitFlagsMask<BitFlags> &operator&=(BitFlagsMask<BitFlags> const &rhs)
  {
    mask &= rhs.mask;
    return *this;
  }

  BitFlagsMask<BitFlags> &operator^=(BitFlagsMask<BitFlags> const &rhs)
  {
    mask ^= rhs.mask;
    return *this;
  }

  constexpr BitFlagsMask<BitFlags> operator|(BitFlagsMask<BitFlags> const &rhs) const
  {
    return BitFlagsMask<BitFlags>(mask | rhs.mask);
  }

  constexpr BitFlagsMask<BitFlags> operator&(BitFlagsMask<BitFlags> const &rhs) const
  {
    return BitFlagsMask<BitFlags>(mask & rhs.mask);
  }

  constexpr BitFlagsMask<BitFlags> operator^(BitFlagsMask<BitFlags> const &rhs) const
  {
    return BitFlagsMask<BitFlags>(mask ^ rhs.mask);
  }

  constexpr bool operator!() const { return !mask; }

  constexpr BitFlagsMask<BitFlags> operator~() const { return operator^(BitFlagsMask<BitFlags>(BitFlagsTraits<BitFlags>::allFlags)); }

  constexpr bool operator==(BitFlagsMask<BitFlags> const &rhs) const { return mask == rhs.mask; }

  constexpr bool operator!=(BitFlagsMask<BitFlags> const &rhs) const { return mask != rhs.mask; }

  void set(BitFlagsMask<BitFlags> const &val) { mask |= val.mask; }

  void clear(BitFlagsMask<BitFlags> const &val) { mask &= (~val).mask; }

  constexpr explicit operator bool() const { return !!mask; }
  constexpr explicit operator MaskType() const { return mask; }
  constexpr MaskType asInteger() const { return mask; }

private:
  constexpr explicit BitFlagsMask(MaskType new_mask) : mask(new_mask) {}

private:
  MaskType mask;
};

template <typename BitFlags>
constexpr BitFlagsMask<BitFlags> operator|(BitFlags bit, BitFlagsMask<BitFlags> const &flags)
{
  return flags | bit;
}

template <typename BitFlags>
constexpr BitFlagsMask<BitFlags> operator&(BitFlags bit, BitFlagsMask<BitFlags> const &flags)
{
  return flags & bit;
}

template <typename BitFlags>
constexpr BitFlagsMask<BitFlags> operator^(BitFlags bit, BitFlagsMask<BitFlags> const &flags)
{
  return flags ^ bit;
}

template <typename BitFlags>
constexpr BitFlagsMask<BitFlags> make_bitmask(BitFlags bit)
{
  return BitFlagsMask<BitFlags>(bit);
}

#define BITMASK_DECLARE_FLAGS_OPERATORS(BitFlags)                          \
  constexpr BitFlagsMask<BitFlags> operator|(BitFlags bit1, BitFlags bit2) \
  {                                                                        \
    return make_bitmask(bit1) | bit2;                                      \
  }                                                                        \
                                                                           \
  constexpr BitFlagsMask<BitFlags> operator&(BitFlags bit1, BitFlags bit2) \
  {                                                                        \
    return make_bitmask(bit1) & bit2;                                      \
  }                                                                        \
                                                                           \
  constexpr BitFlagsMask<BitFlags> operator^(BitFlags bit1, BitFlags bit2) \
  {                                                                        \
    return make_bitmask(bit1) ^ bit2;                                      \
  }
