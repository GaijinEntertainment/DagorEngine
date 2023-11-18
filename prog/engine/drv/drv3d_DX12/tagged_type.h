#pragma once

#include <util/dag_safeArg.h>
#include <value_range.h>


template <typename I, typename TAG>
class TaggedIndexType
{
  I value{};

  constexpr TaggedIndexType(I v) : value{v} {}

public:
  using ValueType = I;

  constexpr TaggedIndexType() = default;
  ~TaggedIndexType() = default;

  TaggedIndexType(const TaggedIndexType &) = default;
  TaggedIndexType &operator=(const TaggedIndexType &) = default;

  static constexpr TaggedIndexType make(I v) { return {v}; }

  constexpr I index() const { return value; }

  friend bool operator==(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value == r.value; }

  friend bool operator!=(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value != r.value; }

  friend bool operator<(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value < r.value; }

  friend bool operator>(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value > r.value; }

  friend bool operator<=(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value <= r.value; }

  friend bool operator>=(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value >= r.value; }

  friend int operator-(const TaggedIndexType &l, const TaggedIndexType &r) { return l.value - r.value; }

  friend TaggedIndexType operator+(const TaggedIndexType &l, I r) { return {I(l.value + r)}; }

  friend TaggedIndexType operator-(const TaggedIndexType &l, I r) { return {I(l.value - r)}; }

  TaggedIndexType &operator+=(I r)
  {
    value += r;
    return *this;
  }

  TaggedIndexType &operator-=(I r)
  {
    value -= r;
    return *this;
  }

  template <typename C>
  TaggedIndexType &operator+=(C r)
  {
    *this = *this + r;
    return *this;
  }

  template <typename C>
  TaggedIndexType &operator-=(C r)
  {
    *this = *this - r;
    return *this;
  }

  TaggedIndexType &operator++()
  {
    ++value;
    return *this;
  }

  TaggedIndexType &operator--()
  {
    --value;
    return *this;
  }

  TaggedIndexType operator++(int) const
  {
    auto copy = *this;
    return ++copy;
  }

  TaggedIndexType operator--(int) const
  {
    auto copy = *this;
    return --copy;
  }

  operator DagorSafeArg() const { return {index()}; }
};

template <typename IT>
class TaggedRangeType : private ValueRange<IT>
{
  using RangeType = ValueRange<IT>;

public:
  using ValueType = typename ValueRange<IT>::ValueType;
  using RangeType::begin;
  using RangeType::end;
  using RangeType::isInside;
  using RangeType::isValidRange;
  using RangeType::size;
  using RangeType::ValueRange;

  constexpr IT front() const { return RangeType::front(); }
  constexpr IT back() const { return RangeType::back(); }

  constexpr TaggedRangeType front(ValueType offset) const { return {IT(this->start + offset), this->stop}; }
  constexpr TaggedRangeType back(ValueType offset) const { return {IT(this->stop - offset), this->stop}; }

  void resize(uint32_t count) { this->stop = this->start + count; }

  constexpr TaggedRangeType subRange(IT offset, uint32_t count) const { return make(this->start + offset, count); }

  constexpr TaggedRangeType subRange(uint32_t offset, uint32_t count) const { return make(this->start + offset, count); }

  static constexpr TaggedRangeType make(IT base, uint32_t count) { return {base, base + count}; }

  static constexpr TaggedRangeType make(uint32_t base, uint32_t count) { return {IT::make(base), IT::make(base + count)}; }

  static constexpr TaggedRangeType make_invalid() { return {IT::make(1), IT::make(0)}; }
};

template <typename IT>
class TaggedCountType
{
public:
  using ValueType = IT;
  using IndexValueType = typename ValueType::ValueType;
  using RangeType = TaggedRangeType<ValueType>;

private:
  IndexValueType value{};

  constexpr TaggedCountType(IndexValueType v) : value{v} {}

public:
  struct Iterator
  {
    IndexValueType at{};

    constexpr Iterator() = default;
    ~Iterator() = default;
    constexpr Iterator(const Iterator &) = default;
    Iterator &operator=(const Iterator &) = default;
    constexpr Iterator(IndexValueType v) : at(v) {}
    constexpr ValueType operator*() const { return ValueType::make(at); }
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
  constexpr Iterator begin() const { return {0}; }
  constexpr Iterator end() const { return {value}; }

  constexpr TaggedCountType() = default;
  ~TaggedCountType() = default;

  TaggedCountType(const TaggedCountType &) = default;
  TaggedCountType &operator=(const TaggedCountType &) = default;

  static constexpr TaggedCountType make(IndexValueType v) { return {v}; }

  constexpr IndexValueType count() const { return value; }

  constexpr RangeType asRange() const { return RangeType::make(0, value); }
  // Allow implicit conversion to range as count is a specialized range
  constexpr operator RangeType() const { return asRange(); }

  constexpr RangeType front(ValueType offset) const { return RangeType::make(offset, value - offset.index()); }
  constexpr RangeType back(ValueType offset) const { return RangeType::make(value - offset.index(), offset.index()); }

  operator DagorSafeArg() const { return {count()}; }

  friend bool operator==(const TaggedCountType &l, const TaggedCountType &r) { return l.value == r.value; }

  friend bool operator!=(const TaggedCountType &l, const TaggedCountType &r) { return l.value != r.value; }

  friend bool operator<=(const TaggedCountType &l, const TaggedCountType &r) { return l.value <= r.value; }

  friend bool operator>=(const TaggedCountType &l, const TaggedCountType &r) { return l.value >= r.value; }

  friend bool operator<(const TaggedCountType &l, const TaggedCountType &r) { return l.value < r.value; }

  friend bool operator>(const TaggedCountType &l, const TaggedCountType &r) { return l.value > r.value; }

  friend bool operator==(const RangeType &l, const TaggedCountType &r) { return 0 == l.front().index() && l.size() == r.value; }

  friend bool operator!=(const RangeType &l, const TaggedCountType &r) { return 0 != l.front().index() && l.size() != r.value; }

  friend bool operator==(const TaggedCountType &l, const RangeType &r) { return 0 == r.front().index() && r.size() == l.value; }

  friend bool operator!=(const TaggedCountType &l, const RangeType &r) { return 0 != r.front().index() && r.size() != l.value; }
};

template <typename IT>
inline constexpr bool operator==(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() == r.index();
}

template <typename IT>
inline constexpr bool operator!=(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() != r.index();
}

template <typename IT>
inline constexpr bool operator<=(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() <= r.index();
}

template <typename IT>
inline constexpr bool operator>=(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() >= r.index();
}

template <typename IT>
inline constexpr bool operator<(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() < r.index();
}

template <typename IT>
inline constexpr bool operator>(const TaggedCountType<IT> &l, const typename TaggedCountType<IT>::ValueType &r)
{
  return l.count() > r.index();
}

template <typename IT>
inline constexpr bool operator==(const typename TaggedCountType<IT>::ValueType &l, const TaggedCountType<IT> &r)
{
  return l.index() == r.count();
}

template <typename IT>
inline constexpr bool operator!=(const typename TaggedCountType<IT>::ValueType &l, const TaggedCountType<IT> &r)
{
  return l.index() != r.count();
}

template <typename IT>
inline constexpr bool operator<=(const typename TaggedCountType<IT>::ValueType &l, const TaggedCountType<IT> &r)
{
  return l.index() <= r.count();
}

template <typename IT>
inline constexpr bool operator>=(const typename TaggedCountType<IT>::ValueType &l, const TaggedCountType<IT> &r)
{
  return l.index() >= r.count();
}

template <typename IT>
inline constexpr bool operator<(const typename TaggedCountType<IT>::ValueType &l, const TaggedCountType<IT> &r)
{
  return l.index() < r.count();
}
