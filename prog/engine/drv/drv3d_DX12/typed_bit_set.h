// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitset.h>


template <typename T, size_t BIT_COUNT = static_cast<size_t>(T::COUNT)>
class TypedBitSet : private eastl::bitset<BIT_COUNT>
{
  using BaseType = eastl::bitset<BIT_COUNT>;

public:
  using BaseType::all;
  using BaseType::any;
  using BaseType::count;
  using BaseType::flip;
  using BaseType::none;
  // using BaseType::to_string;
  using BaseType::to_ulong;
  // using BaseType::to_ullong;
  using BaseType::size;
  using typename BaseType::reference;

  TypedBitSet() = default;
  TypedBitSet(const TypedBitSet &) = default;
  ~TypedBitSet() = default;

  TypedBitSet &operator=(const TypedBitSet &) = default;

  bool operator[](T index) const { return (*this)[static_cast<size_t>(index)]; }
  typename BaseType::reference operator[](T index) { return (*this)[static_cast<size_t>(index)]; }
  bool test(T index) const { return BaseType::test(static_cast<size_t>(index)); }

  TypedBitSet &set()
  {
    BaseType::set();
    return *this;
  }

  TypedBitSet &set(T index, bool value = true)
  {
    BaseType::set(static_cast<size_t>(index), value);
    return *this;
  }

  TypedBitSet &reset()
  {
    BaseType::reset();
    return *this;
  }

  TypedBitSet &reset(T index)
  {
    BaseType::reset(static_cast<size_t>(index));
    return *this;
  }

  TypedBitSet operator~() const
  {
    auto cpy = *this;
    cpy.flip();
    return cpy;
  }

  bool operator==(const TypedBitSet &other) const { return BaseType::operator==(other); }

  bool operator!=(const TypedBitSet &other) const { return BaseType::operator!=(other); }

  // extended stuff
  template <typename T0, typename T1, typename... Ts>
  TypedBitSet &set(T0 v0, T1 v1, Ts... vs)
  {
    set(v0);
    set(v1, vs...);
    return *this;
  }

  template <typename T0, typename T1, typename... Ts>
  TypedBitSet &reset(T0 v0, T1 v1, Ts... vs)
  {
    reset(v0);
    reset(v1, vs...);
    return *this;
  }

  template <typename T0, typename... Ts>
  TypedBitSet(T0 v0, Ts... vs)
  {
    set(v0, vs...);
  }
};
