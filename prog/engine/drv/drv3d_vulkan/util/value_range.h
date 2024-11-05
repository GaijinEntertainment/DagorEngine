// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>

template <typename T>
struct ValueRange
{
  struct Iterator
  {
    T at = 0;

    constexpr Iterator() = default;
    ~Iterator() = default;
    constexpr Iterator(const Iterator &) = default;
    Iterator &operator=(const Iterator &) = default;
    constexpr Iterator(T v) : at(v) {}
    constexpr T operator*() const { return at; }
    Iterator &operator++()
    {
      ++at;
      return *this;
    }
    Iterator operator++(int) { return at++; }
    Iterator &operator--()
    {
      --at;
      return *this;
    }
    Iterator operator--(int) { return at--; }
    friend constexpr bool operator==(Iterator l, Iterator r) { return l.at == r.at; }
    friend constexpr bool operator!=(Iterator l, Iterator r) { return l.at != r.at; }
  };
  // inclusive
  T start = 0;
  // exclusive
  T stop = 0;

  constexpr ValueRange() = default;
  ~ValueRange() = default;

  constexpr ValueRange(const ValueRange &) = default;
  ValueRange &operator=(const ValueRange &) = default;

  constexpr ValueRange(T s, T e) : start(s), stop(e) {}
  // allow base type conversion
  template <typename U>
  constexpr ValueRange(const ValueRange<U> &o) : start(T(o.start)), stop(T(o.stop))
  {}

  void clear() { start = stop = 0; }

  constexpr T front() const { return start; }
  constexpr T back() const { return stop; }
  constexpr Iterator begin() const { return {start}; }
  constexpr Iterator end() const { return {stop}; }

  constexpr T length() const { return stop - start; }
  constexpr T size() const { return length(); }
  void reset(T s, T e)
  {
    start = s;
    stop = e;
  }
  constexpr bool empty() const { return start == stop; }

  template <typename U>
  constexpr bool isAdjacent(U v) const
  {
    return (v + 1) == start || v == stop;
  }
  template <typename U>
  constexpr bool isInside(U v) const
  {
    return v >= start && v < stop;
  }
  template <typename U>
  constexpr bool isSubRangeOf(ValueRange<U> o) const
  {
    return (o.isInside(start) && o.isInside(stop)) || o.stop == stop;
  }
  template <typename U>
  constexpr bool overlaps(ValueRange<U> o) const
  {
    // no overlap if
    // - this comes before o
    // - this comes after o
    return !((stop <= o.start) || (start >= o.stop));
  }

  friend constexpr bool operator==(ValueRange l, ValueRange r) { return l.start == r.start && l.stop == r.stop; }

  friend constexpr bool operator!=(ValueRange l, ValueRange r) { return !(l == r); }

  constexpr ValueRange merge(ValueRange o) const { return {o.start < start ? o.start : start, o.stop > stop ? o.stop : stop}; }

  constexpr bool isContigous(ValueRange o) const { return stop == o.start || o.stop == start; }

  // returns start-at, turns this into at-stop
  ValueRange cut(T at)
  {
    G_ASSERT(at >= start);
    G_ASSERT(at <= stop);
    ValueRange newRange(start, at);
    start = at;
    return newRange;
  }

  // may returns a second range if o cuts this into two parts
  // first part always remains in this
  ValueRange cutOut(ValueRange o)
  {
    ValueRange other;
    if (o.start <= start && o.stop > start)
    {
      if (o.stop >= stop)
      {
        // this is a subset of o, so cutting o from this results in a empty range
        start = stop = 0;
      }
      else
      {
        start = o.stop;
      }
    }
    else if (o.start < stop)
    {
      if (o.stop < stop)
      {
        other.start = o.stop;
        other.stop = stop;
      }
      stop = o.start;
    }
    return other;
  }

  constexpr ValueRange getOverlap(ValueRange o) const
  {
    ValueRange result;
    if (o.start < start && o.stop > start)
    {
      result.start = start;

      if (o.stop < stop)
        result.stop = o.stop;
      else
        result.stop = stop;
    }
    else if (o.start < stop)
    {
      result.start = o.start;

      if (o.stop < stop)
        result.stop = o.stop;
      else
        result.stop = stop;
    }
    return result;
  }

  void pop_front() { ++start; }
  void pop_back() { --stop; }
  void push_front() { --start; }
  void push_back() { ++stop; }

  constexpr ValueRange front(T offset) const { return {T(start + offset), stop}; }
  constexpr ValueRange back(T offset) const { return {T(stop - offset), stop}; }

  constexpr T operator[](T offset) const { return start + offset; }
};

template <typename T, typename U>
inline constexpr ValueRange<T> make_value_range(T from, U sz)
{
  return {from, T(from + sz)};
}

// terminator only takes one value
template <typename P>
constexpr P c_max(P p)
{
  return p;
}
// max with arbitrary amount of values to pick from
template <typename P, typename... PP>
constexpr P c_max(P p, PP... pp)
{
  return c_max(pp...) > p ? c_max(pp...) : p;
}