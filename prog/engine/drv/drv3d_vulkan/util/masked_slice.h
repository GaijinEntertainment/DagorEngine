#pragma once

template <typename T, typename M>
class MaskedSlice
{
  dag::ConstSpan<T> slice;
  M mask;

public:
  MaskedSlice() = default;
  ~MaskedSlice() = default;

  MaskedSlice(const MaskedSlice &) = default;
  MaskedSlice &operator=(const MaskedSlice &) = default;

  MaskedSlice(MaskedSlice &&) = default;
  MaskedSlice &operator=(MaskedSlice &&) = default;

  template <typename U>
  MaskedSlice(dag::ConstSpan<T> s, U m) : slice(s), mask(m)
  {}

  class Iterator
  {
    dag::ConstSpan<T> slice;
    M mask;
    uint32_t pos = 0;

    void findFirst(uint32_t start)
    {
      for (pos = start; pos < slice.size(); ++pos)
        if (mask(slice, pos))
          break;
    }

    void findNext()
    {
      if (pos < slice.size())
        for (++pos; pos < slice.size(); ++pos)
          if (mask(slice, pos))
            break;
    }

  public:
    Iterator() = default;
    ~Iterator() = default;

    Iterator(const Iterator &) = default;
    Iterator &operator=(const Iterator &) = default;

    Iterator(Iterator &&) = default;
    Iterator &operator=(Iterator &&) = default;

    Iterator(dag::ConstSpan<T> s, M m, uint32_t p) : slice(s), mask(m) { findFirst(p); }

    Iterator &operator++()
    {
      findNext();
      return *this;
    }

    Iterator operator++(int)
    {
      Iterator cpy = *this;
      findNext();
      return cpy;
    }

    bool operator==(const Iterator &o) const { return pos == o.pos; }
    bool operator!=(const Iterator &o) const { return !(*this == o); }

    const T &operator*() const { return slice[pos]; }
  };

  inline Iterator begin() const { return Iterator(slice, mask, 0); }
  inline Iterator end() const { return Iterator(slice, mask, slice.size()); }
};