//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <EASTL/fixed_vector.h>
#include <generic/dag_span.h>
#include <math/dag_mathBase.h>
#include <memory/dag_memPtrAllocator.h>
#include <debug/dag_log.h>

template <class T>
struct InterpolatePoint
{
  typedef T Value;
  float x;
  float dxInv;
  T y;
};
DAG_DECLARE_RELOCATABLE(InterpolatePoint<float>);

template <class Cont>
class Interpolator
{
public:
  typedef Cont Container;
  typedef typename Container::value_type TabPoint;
  typedef typename TabPoint::Value Value;

  typedef const TabPoint *const_iterator;
  typedef const TabPoint &const_reference;
  typedef eastl::reverse_iterator<const_iterator> const_reverse_iterator;

  Interpolator() : cont() {}
  Interpolator(const Interpolator &) = default;
  Interpolator(Interpolator &&) = default;

  void clear() { cont.clear(); }
  bool empty() const { return cont.empty(); }

  Interpolator &operator=(const Interpolator &a)
  {
    cont = a.cont;
    return *this;
  }

  template <typename Allocator2>
  void copy(const Interpolator<Allocator2> &src)
  {
    cont.resize(src.size());
    eastl::copy(src.begin(), src.end(), cont.begin());
  }

  /// add element to end of array and check sorting
  Value *add(float x, Value y)
  {
    if (!cont.empty())
    {
      TabPoint &prev = cont.back();
      float dx = x - prev.x;
      if (dx <= VERY_SMALL_NUMBER)
        return nullptr;
      prev.dxInv = 1.f / dx;
    }

    TabPoint &p = cont.push_back();
    p.x = x;
    p.y = y;
    p.dxInv = 0.f;

    return &p.y;
  }

  Value interpolate(float val) const
  {
    float t = 0;
    const Value *v0 = nullptr;
    const Value *v1 = nullptr;
    if (findRange(val, t, v0, v1))
      return v1 ? lerp(*v0, *v1, t) : *v0;

    return ZERO<Value>();
  }

  bool findRange(float val, float &out_t, const Value *&out_0, const Value *&out_1) const
  {
    unsigned last = (unsigned)cont.size();
    if (last == 0)
      return false;

    if (last == 1 || val <= cont[0].x)
    {
      out_0 = &cont[0].y;
      out_1 = nullptr;
      return true;
    }

    if (val >= cont[last - 1].x)
    {
      out_0 = &cont[last - 1].y;
      out_1 = nullptr;
      return true;
    }

    unsigned first = 0;
    unsigned i;
    while (first < last)
    {
      i = (unsigned)(first + last) >> 1;

      if (val <= cont[i].x)
        last = i;
      else
        first = i + 1;
    }

    out_t = (val - cont[last - 1].x) * cont[last - 1].dxInv;
    out_0 = &cont[last - 1].y;
    out_1 = &cont[last].y;

    return true;
  }

  inline const Container &getContainer() const { return cont; }

  const_iterator begin() const { return cont.begin(); }
  const_iterator end() const { return cont.end(); }
  const_reverse_iterator rbegin() const { return cont.rbegin(); }
  const_reverse_iterator rend() const { return cont.rend(); }
  const_reference front() const { return cont.front(); }
  const_reference back() const { return cont.back(); }
  const_reference operator[](size_t n) const { return cont[n]; }
  size_t size() const { return cont.size(); }

protected:
  Container cont;
};

template <class Value, typename Allocator = eastl::allocator>
using InterpolateTab = Interpolator<dag::Vector<InterpolatePoint<Value>, Allocator>>;

typedef InterpolateTab<float> InterpolateTabFloat;

template <class Value>
class InterpolateTabMemPtr : public Interpolator<dag::Vector<InterpolatePoint<Value>, dag::MemPtrAllocator>>
{
public:
  typedef Interpolator<dag::Vector<InterpolatePoint<Value>, dag::MemPtrAllocator>> InterpolateBase;

  InterpolateTabMemPtr() {}
  InterpolateTabMemPtr(IMemAlloc *a) { InterpolateBase::cont.get_allocator().m = a; }

  void setMemAlloc(IMemAlloc *a) { InterpolateBase::cont.get_allocator().m = a; }
};

typedef InterpolateTabMemPtr<float> InterpolateTabMemPtrFloat;

template <class Value, size_t size>
using InterpolateFixedTab = Interpolator<eastl::fixed_vector<InterpolatePoint<Value>, size, false>>;

template <class Interpolator1D>
class Interpolator2D
{
public:
  typedef Interpolator1D Interpolator1DL1;
  typedef typename Interpolator1D::Container Container;
  typedef typename Container::value_type::Value Interpolator1DL2;
  typedef typename Interpolator1DL2::TabPoint TabPoint;
  typedef typename TabPoint::Value Value;

  Interpolator2D() : root() {}

  void clear() { root.clear(); }
  size_t size() const { return root.size(); }

  Interpolator1DL2 *add(float x) { return root.add(x, Interpolator1DL2()); }

  bool add(float x, dag::Span<float> y_list, dag::Span<Value> z_list)
  {
    if (y_list.size() != z_list.size())
    {
      logerr("Interpolator2D: size of arrays Y and Z does not match %d != %d", y_list.size(), z_list.size());
      return false;
    }

    Interpolator1DL2 *val = root.add(x, Interpolator1DL2());
    if (!val)
      return false;

    bool done = true;
    for (int i = 0; i < y_list.size(); ++i)
      done &= val->add(y_list[i], z_list[i]) != nullptr;

    return done;
  }

  void interpolate(float val, Interpolator1DL2 &res) const
  {
    float t = 0;
    const Interpolator1DL2 *t0 = nullptr;
    const Interpolator1DL2 *t1 = nullptr;
    if (root.findRange(val, t, t0, t1))
    {
      if (t1)
      {
        for (int i = 0; i < t0->size() && i < t1->size(); ++i)
          res.add((*t0)[i].x, lerp((*t0)[i].y, t1->interpolate((*t0)[i].x), t));
      }
      else
      {
        res = *t0;
      }
    }
  }

  Value interpolate(float val_x, float val_y) const
  {
    float t = 0;
    const Interpolator1DL2 *t0 = nullptr;
    const Interpolator1DL2 *t1 = nullptr;
    if (root.findRange(val_x, t, t0, t1))
      return t1 ? lerp(t0->interpolate(val_y), t1->interpolate(val_y), t) : t0->interpolate(val_y);

    return ZERO<Value>();
  }

  bool empty() const
  {
    if (root.empty())
      return true;

    for (auto &t : root)
      if (t.y.empty())
        return true;

    return false;
  }

  const Interpolator1DL1 &getRoot() const { return root; }

protected:
  Interpolator1DL1 root;
};

template <class Value, typename Allocator = eastl::allocator>
using Interpolate2DTab = Interpolator2D<
  Interpolator<dag::Vector<InterpolatePoint<Interpolator<dag::Vector<InterpolatePoint<Value>, Allocator>>>, Allocator>>>;

typedef Interpolate2DTab<float> Interpolate2DTabFloat;

template <class Value>
class Interpolate2DTabMemPtr : public Interpolator2D<InterpolateTabMemPtr<InterpolateTabMemPtr<Value>>>
{
public:
  typedef Interpolator2D<InterpolateTabMemPtr<InterpolateTabMemPtr<Value>>> Interpolate2DBase;

  Interpolate2DTabMemPtr() {}

  Interpolate2DTabMemPtr(IMemAlloc *a) { Interpolate2DBase::root.setMemAlloc(a); }

  void setMemAlloc(IMemAlloc *a) { Interpolate2DBase::root.setMemAlloc(a); }

  typename Interpolate2DBase::Interpolator1DL2 *add(float x)
  {
    typename Interpolate2DBase::Interpolator1DL2 *val = Interpolate2DBase::root.add(x, typename Interpolate2DBase::Interpolator1DL2());
    if (val != nullptr)
      val->setMemAlloc(Interpolate2DBase::root.getContainer().get_allocator().m);
    return val;
  }

  bool add(float x, dag::Span<float> y_list, dag::Span<Value> z_list)
  {
    if (y_list.size() != z_list.size())
    {
      logerr("Interpolator2D: size of arrays Y and Z does not match %d != %d", y_list.size(), z_list.size());
      return false;
    }

    typename Interpolate2DBase::Interpolator1DL2 *val = Interpolate2DBase::root.add(x, typename Interpolate2DBase::Interpolator1DL2());
    if (!val)
      return false;
    val->setMemAlloc(Interpolate2DBase::root.getContainer().get_allocator().m);

    bool done = true;
    for (int i = 0; i < y_list.size(); ++i)
      done &= val->add(y_list[i], z_list[i]) != nullptr;

    return done;
  }
};

typedef Interpolate2DTabMemPtr<float> Interpolate2DTabMemPtrFloat;
