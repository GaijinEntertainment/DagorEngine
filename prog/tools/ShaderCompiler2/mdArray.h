// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "commonUtils.h"
#include <generic/dag_carray.h>
#include <generic/dag_span.h>
#include <generic/dag_enumerate.h>
#include <debug/dag_assert.h>

// @TODO: replace eastl metafunctions

namespace detail
{

template <class T, template <class, class> class Checker, class... Ts>
static constexpr bool all_types_are = (Checker<Ts, T>::value && ...);

template <class T, class... Ts>
static constexpr bool all_types_are_convertible_to = all_types_are<T, eastl::is_convertible, Ts...>;

template <uint32_t Rank, class... Ts>
static constexpr bool are_mdarray_indices = sizeof...(Ts) == Rank && all_types_are_convertible_to<uint32_t, Ts...>;

template <uint32_t Rank>
  requires(Rank >= 1)
struct MdArrayIndex : carray<uint32_t, Rank>
{
  using BaseType = carray<uint32_t, Rank>;

  constexpr uint32_t rank() const { return BaseType::size(); }
};

template <uint32_t Rank>
struct MdArrayDimensions;

template <uint32_t Rank>
  requires(Rank == 1)
struct MdArrayDimensions<Rank> : carray<uint32_t, Rank>
{
  using BaseType = carray<uint32_t, Rank>;
  using IndexType = MdArrayIndex<Rank>;

  constexpr uint32_t rank() const { return BaseType::size(); }
  constexpr uint32_t totalSize() const { return head(); }
  constexpr uint32_t head() const { return BaseType::at(0); }
  constexpr uint32_t toFlatIndex(const IndexType &idx) const { return idx[0]; }
  constexpr IndexType toMdIndex(uint32_t idx) const { return {idx}; }
};

template <uint32_t Rank>
  requires(Rank > 1)
struct MdArrayDimensions<Rank> : carray<uint32_t, Rank>
{
  using BaseType = carray<uint32_t, Rank>;
  using IndexType = MdArrayIndex<Rank>;

  carray<uint32_t, Rank> indexConversionMultipliers{};

  template <class... TDims>
    requires(are_mdarray_indices<Rank, TDims...>)
  constexpr MdArrayDimensions(TDims... dims) : BaseType{dims...}
  {
    indexConversionMultipliers[rank() - 1] = 1;
    for (uint32_t i = 1; i < rank(); ++i)
    {
      const uint32_t dimId = rank() - i;
      indexConversionMultipliers[dimId - 1] = BaseType::at(dimId) * indexConversionMultipliers[dimId];
    }
  }

  constexpr MdArrayDimensions(const MdArrayDimensions &other) = default;
  constexpr MdArrayDimensions &operator=(const MdArrayDimensions &other) = default;

  constexpr uint32_t rank() const { return BaseType::size(); }
  constexpr uint32_t totalSize() const
  {
    uint32_t res{1};
    for (uint32_t dim : *this)
      res *= dim;
    return res;
  }

  constexpr uint32_t head() const { return BaseType::at(0); }
  constexpr MdArrayDimensions<Rank - 1> tail() const
  {
    MdArrayDimensions<Rank - 1> h{};
    for (uint32_t i = 1; i < rank(); ++i)
      h[i - 1] = BaseType::at(i);
    return h;
  }

  constexpr uint32_t toFlatIndex(const IndexType &idx) const
  {
    uint32_t res{0};
    for (uint32_t i = 0; i < rank(); ++i)
    {
      uint32_t id = rank() - i - 1;
      res += idx[id] * indexConversionMultipliers[id];
    }
    return res;
  }

  constexpr IndexType toMdIndex(uint32_t idx) const
  {
    IndexType res{};
    for (uint32_t i = 0; i < rank(); ++i)
    {
      uint32_t id = rank() - i - 1;
      res[id] = idx % indexConversionMultipliers[id];
      idx /= indexConversionMultipliers[id];
    }
    return res;
  }
};

template <uint32_t... Dims>
static constexpr MdArrayDimensions<sizeof...(Dims)> dims_from_varargs{Dims...};

template <MdArrayDimensions Dims, class... Ids>
  requires(are_mdarray_indices<Dims.rank(), Ids...>)
MdArrayIndex<Dims.rank()> ids_from_varargs(Ids... ids)
{
  // @TODO: validate indices here one by one with a fold expression?
  return {ids...};
}

// @NOTE: Sadly, we can't have both pretty initializer syntax and compile time dimensionality checks
template <class T, uint32_t Rank>
struct GetMdInitializerList;
template <class T, uint32_t Rank>
  requires(Rank > 1)
struct GetMdInitializerList<T, Rank> : eastl::type_identity<std::initializer_list<typename GetMdInitializerList<T, Rank - 1>::type>>
{};
template <class T, uint32_t Rank>
  requires(Rank == 1)
struct GetMdInitializerList<T, Rank> : eastl::type_identity<std::initializer_list<T>>
{};

template <class T, uint32_t Rank, MdArrayDimensions<Rank> Dims>
struct MdArray;

template <class T, uint32_t Rank, MdArrayDimensions<Rank> Dims>
  requires(Rank > 1)
constexpr void fill_mdarray_from_initializer(MdArray<T, Rank, Dims> &arr, typename GetMdInitializerList<T, Rank>::type initializer);
template <class T, uint32_t Rank, MdArrayDimensions<Rank> Dims>
constexpr void fill_mdarray_from_initializer(MdArray<T, Rank, Dims> &arr, std::initializer_list<T> initializer);

template <class T, uint32_t Rank, MdArrayDimensions<Rank> Dims>
  requires(Rank == 1)
struct MdArray<T, Rank, Dims> : carray<T, Dims.totalSize()>
{
  using BaseType = carray<T, Dims.totalSize()>;
  using SubElementType = T;
  using ElementType = T;
  using DimsType = MdArrayDimensions<Rank>;
  using IndexType = DimsType::IndexType;

  static constexpr MdArrayDimensions DIMS = Dims;
  static constexpr uint32_t SIZE = DIMS.totalSize();
  static constexpr uint32_t RANK = Rank;

  constexpr MdArray(std::initializer_list<T> initializer)
    requires(eastl::is_default_constructible_v<T> && eastl::is_copy_assignable_v<T>)
  {
    fill_mdarray_from_initializer(*this, initializer);
  }

  constexpr MdArray() = default;
  constexpr MdArray(const MdArray &) = default;
  constexpr MdArray(MdArray &&) = default;
  constexpr MdArray &operator=(const MdArray &) = default;
  constexpr MdArray &operator=(MdArray &&) = default;

  constexpr T &operator[](uint32_t idx) { return BaseType::operator[](idx); }
  constexpr const T &operator[](uint32_t idx) const { return BaseType::operator[](idx); }

  constexpr T &operator[](IndexType idxs) { return operator[](DIMS.toFlatIndex(idxs)); }
  constexpr const T &operator[](IndexType idxs) const { return operator[](DIMS.toFlatIndex(idxs)); }

  constexpr T &at(uint32_t idx) { return BaseType::at(idx); }
  constexpr const T &at(uint32_t idx) const { return BaseType::at(idx); }

  constexpr T &at(IndexType idxs) { return at(DIMS.toFlatIndex(idxs)); }
  constexpr const T &at(IndexType idxs) const { return at(DIMS.toFlatIndex(idxs)); }

  // For one-dimensional case, these are the same
  constexpr dag::Span<T> rowView() { return make_span(*this); }
  constexpr dag::ConstSpan<T> rowView() const { return make_span_const(*this); }
  constexpr dag::Span<T> flatView() { return make_span(*this); }
  constexpr dag::ConstSpan<T> flatView() const { return make_span_const(*this); }
};

template <class T, uint32_t Rank, MdArrayDimensions<Rank> Dims>
  requires(Rank > 1)
struct MdArray<T, Rank, Dims> : carray<MdArray<T, Rank - 1, Dims.tail()>, Dims.head()>
{
  using BaseType = carray<MdArray<T, Rank - 1, Dims.tail()>, Dims.head()>;
  using SubElementType = MdArray<T, Rank - 1, Dims.tail()>;
  using ElementType = T;
  using DimsType = MdArrayDimensions<Rank>;
  using IndexType = DimsType::IndexType;

  static constexpr MdArrayDimensions DIMS = Dims;
  static constexpr uint32_t SIZE = DIMS.totalSize();
  static constexpr uint32_t RANK = Rank;

  constexpr MdArray(typename GetMdInitializerList<T, RANK>::type initializer)
    requires(eastl::is_default_constructible_v<T> && eastl::is_copy_assignable_v<T>)
  {
    fill_mdarray_from_initializer(*this, initializer);
  }
  constexpr MdArray(std::initializer_list<T> initializer)
    requires(eastl::is_default_constructible_v<T> && eastl::is_copy_assignable_v<T>)
  {
    fill_mdarray_from_initializer(*this, initializer);
  }

  constexpr MdArray() = default;
  constexpr MdArray(const MdArray &) = default;
  constexpr MdArray(MdArray &&) = default;
  constexpr MdArray &operator=(const MdArray &) = default;
  constexpr MdArray &operator=(MdArray &&) = default;

  // @NOTE: unavailable until C++23
  // template <class... Ids>
  //   requires(are_mdarray_indices<RANK, Ids...>)
  // constexpr T &operator[](Ids... ids)
  // {
  //   return operator[](ids_from_varargs<DIMS>(ids...));
  // }
  //
  // template <class... Ids>
  //   requires(are_mdarray_indices<RANK, Ids...>)
  // constexpr const T &operator[](Ids... ids) const
  // {
  //   return operator[](ids_from_varargs<DIMS>(ids...));
  // }

  constexpr SubElementType &operator[](uint32_t idx) { return BaseType::operator[](idx); }
  constexpr const SubElementType &operator[](uint32_t idx) const { return BaseType::operator[](idx); }

  constexpr T &operator[](const IndexType &idxs) { return flatView()[DIMS.toFlatIndex(idxs)]; }
  constexpr const T &operator[](const IndexType &idxs) const { return flatView()[DIMS.toFlatIndex(idxs)]; }

  constexpr SubElementType &at(uint32_t idx) { return BaseType::at(idx); }
  constexpr const SubElementType &at(uint32_t idx) const { return BaseType::at(idx); }

  template <class... Ids>
    requires(are_mdarray_indices<RANK, Ids...>)
  constexpr T &at(Ids... ids)
  {
    return at(ids_from_varargs<DIMS>(ids...));
  }

  template <class... Ids>
    requires(are_mdarray_indices<RANK, Ids...>)
  constexpr const T &at(Ids... ids) const
  {
    return at(ids_from_varargs<DIMS>(ids...));
  }

  constexpr T &at(const IndexType &idxs) { return flatView().at(DIMS.toFlatIndex(idxs)); }
  constexpr const T &at(const IndexType &idxs) const { return flatView().at(DIMS.toFlatIndex(idxs)); }

  constexpr uint32_t size() const { return SIZE; }
  constexpr uint32_t capacity() const { return SIZE; }
  constexpr uint32_t rank() const { return RANK; }
  constexpr DimsType dimensions() const { return DIMS; }
  constexpr uint32_t dimension(uint32_t i) const { return DIMS[i]; }

  constexpr uint32_t toFlatIndex(const IndexType &idxs) const { return DIMS.toFlatIndex(idxs); }
  constexpr IndexType toMdIndex(uint32_t idx) const { return DIMS.toMdIndex(idx); }

  constexpr BaseType::reference front() = delete;
  constexpr BaseType::const_reference front() const = delete;
  constexpr BaseType::reference back() = delete;
  constexpr BaseType::const_reference back() const = delete;

  // No implicit iteration: use rowView or flatView for it
  constexpr BaseType::iterator begin() = delete;
  constexpr BaseType::const_iterator begin() const = delete;
  constexpr BaseType::const_iterator cbegin() const = delete;
  constexpr BaseType::iterator end() = delete;
  constexpr BaseType::const_iterator end() const = delete;
  constexpr BaseType::const_iterator cend() const = delete;

  constexpr dag::Span<SubElementType> rowView() { return dag::Span<SubElementType>{BaseType::begin(), dimension(0)}; }
  constexpr dag::ConstSpan<SubElementType> rowView() const { return dag::ConstSpan<SubElementType>{BaseType::begin(), dimension(0)}; }

  constexpr dag::Span<T> flatView() { return dag::Span<T>{reinterpret_cast<T *>(BaseType::begin()), static_cast<intptr_t>(size())}; }
  constexpr dag::ConstSpan<T> flatView() const
  {
    return dag::ConstSpan<T>{reinterpret_cast<const T *>(BaseType::begin()), static_cast<intptr_t>(size())};
  }
};

template <class T, uint32_t Rank, MdArrayDimensions<Rank> Dims>
  requires(Rank > 1)
constexpr void fill_mdarray_from_initializer(MdArray<T, Rank, Dims> &arr, typename GetMdInitializerList<T, Rank>::type initializer)
{
  G_FAST_ASSERT(initializer.size() == arr.dimension(0));
  for (uint32_t i = 0; i < initializer.size(); ++i)
    fill_mdarray_from_initializer(arr[i], *(initializer.begin() + i));
}

template <class T, uint32_t Rank, MdArrayDimensions<Rank> Dims>
constexpr void fill_mdarray_from_initializer(MdArray<T, Rank, Dims> &arr, std::initializer_list<T> initializer)
{
  G_FAST_ASSERT(initializer.size() == arr.size());
  for (uint32_t i = 0; i < initializer.size(); ++i)
    arr.flatView()[i] = *(initializer.begin() + i);
}

template <class T, uint32_t Rank, MdArrayDimensions<Rank> Dims, size_t... Is, class F>
static MdArray<T, Rank, Dims> mdarray_make_impl(eastl::index_sequence<Is...>, F &&make)
{
  return {((void)Is, make())...};
}

} // namespace detail

template <class T, uint32_t... Dims>
using MdArray = detail::MdArray<T, sizeof...(Dims), detail::dims_from_varargs<Dims...>>;

template <uint32_t... Dims, class F>
  requires(eastl::is_invocable_v<F>)
static MdArray<eastl::invoke_result_t<F>, Dims...> mdarray_make(F &&make)
{
  return detail::mdarray_make_impl<eastl::invoke_result_t<F>, sizeof...(Dims), detail::dims_from_varargs<Dims...>>(
    eastl::make_index_sequence<detail::dims_from_varargs<Dims...>.totalSize()>{}, eastl::forward<F>(make));
}
