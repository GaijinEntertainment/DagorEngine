// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

template <typename T, size_t N>
class ForwardRing
{
  T elements[N]{};
  T *selectedElement = &elements[0];

  ptrdiff_t getIndex(const T *pos) const { return pos - &elements[0]; }

  T *advancePosition(T *pos) { return &elements[(getIndex(pos) + 1) % N]; }

public:
  T &get() { return *selectedElement; }
  const T &get() const { return *selectedElement; }
  T &operator*() { return *selectedElement; }
  const T &operator*() const { return *selectedElement; }
  T *operator->() { return selectedElement; }
  const T *operator->() const { return selectedElement; }

  uint32_t getIndex() const { return getIndex(selectedElement); }

  constexpr size_t size() const { return N; }

  void advance() { selectedElement = advancePosition(selectedElement); }

  // Iterates all elements of the ring in a unspecified order
  template <typename C>
  void iterate(C clb)
  {
    for (auto &&v : elements)
    {
      clb(v);
    }
  }

  // Iterates all elements from last to the current element
  template <typename C>
  void walkAll(C clb)
  {
    auto at = advancePosition(selectedElement);
    for (size_t i = 0; i < N; ++i)
    {
      clb(*at);
      at = advancePosition(at);
    }
  }

  // Iterates all elements from last to the element before the current element
  template <typename C>
  void walkUnitlCurrent(C clb)
  {
    auto at = advancePosition(selectedElement);
    for (size_t i = 0; i < N - 1; ++i)
    {
      clb(at);
      at = advancePosition(at);
    }
  }
};
