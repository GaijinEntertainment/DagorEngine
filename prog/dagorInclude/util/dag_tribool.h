//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>


struct Tribool
{
  enum Type : uint8_t
  {
    Undef = 0xff,
    False = 0x00,
    True = 0x01,
  };

private:
  uint8_t mVal = Tribool::Undef;

public:
  inline constexpr Tribool() = default;
  inline constexpr Tribool(const Tribool::Type &r) : mVal(r) {}
  inline constexpr Tribool(const Tribool &r) : mVal(r.mVal) {}
  inline explicit Tribool(bool b_val) { mVal = b_val ? Tribool::True : Tribool::False; }

public:
  inline Tribool &set(const Tribool &r)
  {
    mVal = r.mVal;
    return *this;
  }
  inline Tribool &set(bool b_val)
  {
    mVal = b_val ? Tribool::True : Tribool::False;
    return *this;
  }
  inline Tribool &set(Tribool::Type t_val)
  {
    mVal = t_val;
    return *this;
  }

  inline Tribool &operator=(const Tribool &r) { return set(r); }
  inline Tribool &operator=(const Tribool::Type &r) { return set(r); }

public:
  inline bool isUndef() const { return mVal == Tribool::Undef; }
  inline bool hasResult() const { return mVal <= Tribool::True; }
  inline bool isSet() const { return mVal != Tribool::Undef; }
  inline bool hasBool() const { return mVal != Tribool::Undef; }
  inline bool hasBool(bool &out_r) const
  {
    if (mVal == Tribool::Undef)
    {
      out_r = false;
      return false;
    }
    out_r = getBoolRaw();
    return true;
  }
  inline bool isFalse() const { return mVal == Tribool::False; }
  inline bool isTrue() const { return mVal == Tribool::True; }
  inline bool maybeTrue() const { return mVal != Tribool::False; } // True or Undef
  inline bool maybeFalse() const { return mVal != Tribool::True; } // False or Undef

  inline Tribool &reset()
  {
    mVal = Tribool::Undef;
    return *this;
  }

  inline bool getBool() const { return (mVal != Tribool::False); }

  inline bool getBool(bool &b_out) const
  {
    if (isUndef())
      return false;
    b_out = (mVal != Tribool::False);
    return true;
  }
  inline bool getBoolRaw() const { return (mVal != Tribool::False); }
  inline uint8_t getRaw() const { return mVal; }

  template <typename TInt>
  inline bool setRaw(TInt r)
  {
    mVal = static_cast<uint8_t>(r);
    return hasResult();
  }

  inline bool operator==(const Tribool::Type &r) const { return mVal == r; }
  inline bool operator!=(const Tribool::Type &r) const { return mVal != r; }
  inline bool operator==(const Tribool &rhs) const { return (mVal == rhs.mVal); }
  inline bool operator!=(const Tribool &rhs) const { return (mVal != rhs.mVal); }

  inline bool isEqual(bool b_val, bool undef_default = false) const
  {
    if (isUndef())
      return undef_default;
    return getBool() == b_val;
  }

  inline bool isNEqual(bool b_val, bool undef_default = false) const
  {
    if (isUndef())
      return undef_default;
    return getBool() != b_val;
  }

  inline Tribool operator||(const Tribool &rhs) const
  {
    const Tribool &lhs = *this;
    if (lhs.mVal == Tribool::True || rhs.mVal == Tribool::True)
      return Tribool::True;
    if (lhs.mVal == Tribool::False && rhs.mVal == Tribool::False)
      return Tribool::False;
    return Tribool::Undef;
  }

}; // Tribool
