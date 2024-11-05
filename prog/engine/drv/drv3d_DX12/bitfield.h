// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>


// duplicated in vulkan backend...
// taken from https://github.com/preshing/cpp11-on-multicore/blob/master/common/

template <typename T, int Offset, int Bits>
struct BitFieldMember
{
  T value;

  enum
  {
    BitOffset = Offset,
    BitCount = Bits
  };

  static const T Maximum = (T(1) << T(Bits)) - ((typename eastl::make_signed<T>::type)(1));
  static const T Mask = Maximum << T(Offset);
  inline T maximum() const
  {
    G_STATIC_ASSERT(Offset + Bits <= (int)sizeof(T) * 8);
    G_STATIC_ASSERT(Bits < (int)sizeof(T) * 8);
    return Maximum;
  }
  inline T one() const { return T(1) << Offset; }

  inline operator T() const { return (value >> Offset) & Maximum; }

  inline BitFieldMember &operator=(T v)
  {
    G_ASSERTF(v <= Maximum, "%d <= %d", v, maximum()); // v must fit inside the bitfield member
    value = (value & ~Mask) | (v << T(Offset));
    return *this;
  }

  inline BitFieldMember &operator+=(T v)
  {
    G_ASSERT(T(*this) + v <= Maximum); // result must fit inside the bitfield member
    value += v << Offset;
    return *this;
  }

  inline BitFieldMember &operator-=(T v)
  {
    G_ASSERT(T(*this) >= v); // result must not underflow
    value -= v << Offset;
    return *this;
  }

  inline BitFieldMember &operator++() { return *this += 1; }
  inline T operator++(int)
  {
    T c = *this;
    ++(*this);
    return c;
  } // postfix form
  inline BitFieldMember &operator--() { return *this -= 1; }
  inline T operator--(int)
  {
    T c = *this;
    --(*this);
    return c;
  } // postfix form
};

template <typename T, int BaseOffset, int BitsPerItem, int NumItems>
struct BitFieldArray
{
  T value;

  static const T Maximum = (T(1) << BitsPerItem) - 1;
  inline T maximum() const
  {
    G_STATIC_ASSERT(BaseOffset + BitsPerItem * NumItems <= (int)sizeof(T) * 8);
    G_STATIC_ASSERT(BitsPerItem < (int)sizeof(T) * 8);
    return Maximum;
  }
  inline int numItems() const { return NumItems; }

  class Element
  {
  private:
    T &value;
    int offset;

  public:
    inline Element(T &value, int offset) : value(value), offset(offset) {}
    inline T mask() const { return Maximum << offset; }

    inline operator T() const { return (value >> offset) & Maximum; }

    inline Element &operator=(T v)
    {
      G_ASSERT(v <= Maximum); // v must fit inside the bitfield member
      value = (value & ~mask()) | (v << offset);
      return *this;
    }

    inline Element &operator+=(T v)
    {
      G_ASSERT(T(*this) + v <= Maximum); // result must fit inside the bitfield member
      value += v << offset;
      return *this;
    }

    inline Element &operator-=(T v)
    {
      G_ASSERT(T(*this) >= v); // result must not underflow
      value -= v << offset;
      return *this;
    }

    inline Element &operator++() { return *this += 1; }
    inline T operator++(int)
    {
      T cpy = *this;
      ++(*this);
      return cpy;
    } // postfix form
    inline Element &operator--() { return *this -= 1; }
    inline T operator--(int)
    {
      T cpy = *this;
      --(*this);
      return cpy;
    } // postfix form
  };

  inline Element operator[](int i)
  {
    G_ASSERT(i >= 0 && i < NumItems); // array index must be in range
    return Element(value, BaseOffset + BitsPerItem * i);
  }

  inline const Element operator[](int i) const
  {
    G_ASSERT(i >= 0 && i < NumItems); // array index must be in range
    return Element(const_cast<T &>(value), BaseOffset + BitsPerItem * i);
  }
};

#define BEGIN_BITFIELD_TYPE(typeName, T) \
  union typeName                         \
  {                                      \
    struct Wrapper                       \
    {                                    \
      T value;                           \
    };                                   \
    Wrapper wrapper;                     \
    inline typeName()                    \
    {                                    \
      wrapper.value = 0;                 \
    }                                    \
    inline explicit typeName(T v)        \
    {                                    \
      wrapper.value = v;                 \
    }                                    \
    inline operator T &()                \
    {                                    \
      return wrapper.value;              \
    }                                    \
    inline operator T() const            \
    {                                    \
      return wrapper.value;              \
    }                                    \
    typedef T StorageType;

#define ADD_BITFIELD_MEMBER(memberName, offset, bits) BitFieldMember<StorageType, offset, bits> memberName;

#define ADD_BITFIELD_ARRAY(memberName, offset, bits, numItems) BitFieldArray<StorageType, offset, bits, numItems> memberName;

#define END_BITFIELD_TYPE() \
  }                         \
  ;

// TODO: this should be replaced with a constexpr function. It will compile a lot faster.
template <uint32_t I>
struct BitsNeeded
{
  static constexpr int VALUE = BitsNeeded<I / 2>::VALUE + 1;
};
template <>
struct BitsNeeded<0>
{
  static constexpr int VALUE = 1;
};
template <>
struct BitsNeeded<1>
{
  static constexpr int VALUE = 1;
};
