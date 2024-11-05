//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/ecsHash.h>
#include <daECS/core/component.h>
#include <generic/dag_smallTab.h>

namespace ecs
{

typedef dag::Vector<ChildComponent> BaseArray;
class Array : protected BaseArray
{
public:
  typedef BaseArray base_type;
  typedef base_type::iterator iterator;
  typedef base_type::const_iterator const_iterator;
  using base_type::capacity;
  using base_type::cbegin;
  using base_type::cend;
  using base_type::reserve;
  using base_type::shrink_to_fit;
  uint32_t size() const { return uint32_t(base_type::size()); }
  iterator begin()
  {
    changeGen();
    return base_type::begin();
  }
  iterator end() { return base_type::end(); }
  const_iterator begin() const { return base_type::begin(); }
  const_iterator end() const { return base_type::end(); }

  bool empty() const { return size() == 0; }
  void clear()
  {
    changeGen();
    base_type::clear();
  }
  bool operator==(const Array &a) const;

  ChildComponent &push_back(ChildComponent &&v)
  {
    changeGen();
    base_type::emplace_back(eastl::move(v));
    return base_type::back();
  }
  template <class T>
  ChildComponent &push_back(T &&v)
  {
    changeGen();
    base_type::emplace_back(eastl::move(v));
    return base_type::back();
  }
  template <class T>
  ChildComponent &push_back(const T &v)
  {
    changeGen();
    base_type::emplace_back(v);
    return base_type::back();
  }
  void pop_back()
  {
    changeGen();
    base_type::pop_back();
  }

  iterator insert(const_iterator pos, ChildComponent &&v)
  {
    changeGen();
    return base_type::insert(pos, eastl::move(v));
  }
  template <class T>
  iterator insert(const_iterator pos, T &v)
  {
    changeGen();
    return base_type::insert(pos, v);
  }

  iterator erase(const_iterator pos)
  {
    changeGen();
    return base_type::erase(pos);
  }
  iterator erase(const_iterator first, const_iterator last)
  {
    changeGen();
    return base_type::erase(first, last);
  }

  ChildComponent &operator[](size_t i)
  {
    changeGen();
    return (base_type::operator[])(i);
  }
  ChildComponent &at(size_t i)
  {
    if (i >= size())
    {
      G_ASSERTF(0, "out of bounds access %d %d", i, size());
      return emptyAttrRO;
    }
    changeGen();
    return (base_type::operator[])(i);
  }
  const ChildComponent &operator[](size_t i) const { return (base_type::operator[])(i); }

  const ChildComponent &at(size_t i) const
  {
    if (i >= size())
    {
      G_ASSERTF(0, "out of bounds access %d %d", i, size());
      return emptyAttrRO;
    }
    return (base_type::operator[])(i);
  }

  bool replicateCompare(const Array &o)
  {
    if (gen == o.gen)
    {
#if DAECS_EXTENSIVE_CHECKS
      if (*this != o)
        logerr("Array differs, while it's generation is same");
      else
        return false;
#else
      return false;
#endif
    }
    if (*this != o)
    {
      *this = o;
      gen = o.gen;
      return true;
    }
    gen = o.gen;
    return false;
  }

  Array() = default;
  Array(const Array &a) : base_type(static_cast<const base_type &>(a)) {}
  Array(Array &&a) : base_type(static_cast<base_type &&>(a)) {}
  inline Array &operator=(const Array &a)
  {
    ((base_type &)*this) = (const base_type &)a;
    changeGen();
    return *this;
  } // do not change generation
  inline Array &operator=(Array &&a)
  {
    ((base_type &)*this) = (base_type &&) eastl::move(a);
    changeGen();
    return *this;
  } // do not change generation

protected:
  static ChildComponent emptyAttrRO;
  void changeGen() { gen++; }
  uint32_t gen = 0;
};

inline bool Array::operator==(const Array &a) const
{
  if (size() != a.size())
    return false;
  for (size_t i = 0, e = size(); i < e; ++i)
    if (!((base_type::operator[])(i) == ((const base_type &)a)[i]))
      return false;
  return true;
}

}; // namespace ecs
