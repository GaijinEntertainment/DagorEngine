#pragma once

namespace rc {
namespace seq {

/// STL iterator for `Seq`.
template <typename T>
class SeqIterator {
public:
  using iterator_category = std::input_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = T *;
  using reference = T &;

  /// Creates a new past-the-end `SeqIterator`.
  SeqIterator() = default;

  /// Creates a new iterator pointing to the beginning of the given `Seq`.
  explicit SeqIterator(Seq<T> seq);

  bool operator==(const SeqIterator<T> &rhs) const;
  T &operator*();
  const T &operator*() const;
  SeqIterator &operator++();
  SeqIterator operator++(int);

private:
  Seq<T> m_seq;
  Maybe<T> m_current;
};

template <typename T>
bool operator!=(const SeqIterator<T> &lhs, const SeqIterator<T> &rhs);

} // namespace seq

template <typename T>
seq::SeqIterator<T> begin(Seq<T> seq);

template <typename T>
seq::SeqIterator<T> end(const Seq<T> &seq);

} // namespace rc

#include "SeqIterator.hpp"
