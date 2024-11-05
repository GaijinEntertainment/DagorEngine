//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

/************************************************************************
  teplate class range
************************************************************************/

/************************************************************************
  can check value range & ranges values.
  template class T must support operators ">=", "<=" and "="

  usage:

  Range<int>  range(0, 10);

  int t = 25, s = -10, m = 5;

  range.isInRange(s) -> return false
  range.isInRange(m) -> return true

  range.rangeIt(t) -> t = 10 (upper bound)
  range.rangeIt(m) -> t = 5
  range.rangeIt(s) -> s = 0 (lower bound)

************************************************************************/

#define RANGE          Range<T>
#define RANGE_TEMPLATE template <class T>

/*********************************
 *
 * class Range
 *
 *********************************/
RANGE_TEMPLATE
class Range
{
public:
  // ctor/dtor
  inline explicit Range(const T &_min_range, const T &_max_range);
  inline ~Range();

  // setup min & max bounds
  inline void setBounds(const T &_min_range, const T &_max_range);

  // check for range (return true, if value in range [min..max])
  inline bool isInRange(const T &value) const;

  // range value (if value out of bounds) - return true, if value has been ranged
  inline bool rangeIt(T &value) const;

  // return ranged value (if value out of bounds)
  inline T getRanged(const T &value) const;

  // return min & max bounds
  inline const T &getMin(bool validate = false) const;
  inline const T &getMax(bool validate = false) const;

  // set min & max bounds
  inline void setMin(const T &v);
  inline void setMax(const T &v);

  // assigment
  inline Range(const Range &other);
  inline Range &operator=(const Range &other);

  // return true, if getMin <= getMax
  inline bool isValid() const;

  // makes getMin <= getMax
  inline void validate();

  // swap Min & Max
  inline void swap();

  // compare
  inline bool operator==(const Range &other) const;
  inline bool operator!=(const Range &other) const;

  // return length between max & min values |max - min|
  inline T getLength() const;

  // return min or max value
  inline const T &operator[](int index) const;

private:
  T min_range;
  T max_range;

  inline const T &minVal(const T &a, const T &b) const { return a < b ? a : b; };
  inline const T &maxVal(const T &a, const T &b) const { return a > b ? a : b; };
}; // class Range
//


// scale val from range (smin, smax) to range (dmin, dmax)
template <typename SRC, typename DST>
inline const DST convertRangedValue(const SRC &val, const SRC &smin, const SRC &smax, const DST &dmin, const DST &dmax)
{
  const SRC slen = smax - smin;
  if (slen == 0)
    return (DST)0;

  const DST dlen = dmax - dmin;
  if (dlen == 0)
    return (DST)0;
  return ((DST)((val - smin) / slen)) * dlen + dmin;
}


// scale val from range s to range d
template <typename SRC, typename DST>
inline const DST convertRangedValue(const SRC &val, const Range<SRC> &s, const Range<DST> &d)
{
  if (s.getLength() == 0 || d.getLength() == 0)
    return (DST)0;
  return ((DST)((val - s.getMin()) / s.getLength())) * d.getLength() + d.getMin();
}


// ctor/dtor
RANGE_TEMPLATE
inline RANGE::Range(const T &_min_range, const T &_max_range) : min_range(_min_range), max_range(_max_range) {}

RANGE_TEMPLATE
inline RANGE::~Range() {}

// setup min & max bounds
RANGE_TEMPLATE
inline void RANGE::setBounds(const T &_min_range, const T &_max_range)
{
  min_range = _min_range;
  max_range = _max_range;
}

// set min & max bounds
RANGE_TEMPLATE
inline void RANGE::setMin(const T &v) { min_range = v; }


RANGE_TEMPLATE
inline void RANGE::setMax(const T &v) { max_range = v; }


// check for range (return true, if value in range [min..max])
RANGE_TEMPLATE
inline bool RANGE::isInRange(const T &value) const { return (value >= getMin(true)) && (value <= getMax(true)); }

// range value (if value out of bounds) - return true, if value has been ranged
RANGE_TEMPLATE
inline bool RANGE::rangeIt(T &value) const
{
  if (value < getMin(true))
  {
    value = getMin(true);
    return true;
  }

  if (value > getMax(true))
  {
    value = getMax(true);
    return true;
  }

  return false;
}

// return min & max bounds
RANGE_TEMPLATE
inline const T &RANGE::getMin(bool validate) const { return validate ? minVal(min_range, max_range) : min_range; }

RANGE_TEMPLATE
inline const T &RANGE::getMax(bool validate) const { return validate ? maxVal(min_range, max_range) : max_range; }

// assigment
RANGE_TEMPLATE
inline RANGE::Range(const Range &other) { operator=(other); }


RANGE_TEMPLATE
inline RANGE &RANGE::operator=(const Range &other)
{
  setBounds(other.min_range, other.max_range);
  return *this;
}

// return true, if getMin <= getMax
RANGE_TEMPLATE
inline bool RANGE::isValid() const { return getMin() <= getMax(); }


// makes getMin <= getMax
RANGE_TEMPLATE
inline void RANGE::validate()
{
  if (!isValid())
    swap();
}


// swap Min & Max
RANGE_TEMPLATE
void RANGE::swap()
{
  const T tmp = max_range;
  max_range = min_range;
  min_range = tmp;
}


// compare
RANGE_TEMPLATE
inline bool RANGE::operator==(const Range &other) const
{
  return (getMin(true) == other.getMin(true)) && (getMax(true) == other.getMax(true));
}

RANGE_TEMPLATE
inline bool RANGE::operator!=(const Range &other) const { return !operator==(other); }

// return length between max & min values |max - min|
RANGE_TEMPLATE
inline T RANGE::getLength() const
{
  const T v = max_range - min_range;
  return v >= 0 ? v : -v;
}

// return ranged value (if value out of bounds)
RANGE_TEMPLATE
inline T RANGE::getRanged(const T &value) const
{
  T val = value;
  rangeIt(val);
  return val;
}

// return min or max value
RANGE_TEMPLATE
inline const T &RANGE::operator[](int index) const { return (index <= 0) ? min_range : max_range; }


#undef RANGE
#undef RANGE_TEMPLATE
