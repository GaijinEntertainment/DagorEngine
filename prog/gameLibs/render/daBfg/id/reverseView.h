// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/iterator.h>


template <class Container>
class ReverseView
{
public:
  template <class T>
  ReverseView(T &&cont) : container_{eastl::forward<T>(cont)}
  {}

  auto begin() const { return eastl::reverse_iterator(eastl::end(container_)); }
  auto end() const { return eastl::reverse_iterator(eastl::begin(container_)); }

private:
  Container container_;
};

// For lvalue, we want a reference to the original container,
// while for an rvalue we actually want to store it inside the view.
template <class T>
ReverseView(T &&) -> ReverseView<T>;
