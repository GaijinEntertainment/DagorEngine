// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitvector.h>
#include <dag/dag_vector.h>
#include "idEnumerateView.h"


template <class EnumType, typename Allocator = EASTLAllocatorType>
struct IdIndexedFlags : eastl::bitvector<Allocator>
{
  using index_type = EnumType;
  using Container = eastl::bitvector<Allocator>;
  using Container::Container;

  typename Container::reference operator[](EnumType key)
  {
    return Container::operator[](static_cast<typename Container::size_type>(eastl::to_underlying(key)));
  }

  typename Container::const_reference operator[](EnumType key) const
  {
    return Container::operator[](static_cast<typename Container::size_type>(eastl::to_underlying(key)));
  }

  void set(EnumType key, bool value) { Container::set(static_cast<typename Container::size_type>(eastl::to_underlying(key)), value); }

  bool test(EnumType key, bool defaultValue = false) const
  {
    return Container::test(static_cast<typename Container::size_type>(eastl::to_underlying(key)), defaultValue);
  }

  bool isMapped(EnumType key) const { return eastl::to_underlying(key) < Container::size(); }

  IdEnumerateView<IdIndexedFlags> enumerate() & { return {*this}; }
  IdEnumerateView<const IdIndexedFlags> enumerate() const & { return {*this}; }
};
