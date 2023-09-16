#pragma once

// helper type to carry around type lists
template <typename...>
struct TypePack
{};

template <typename, typename>
struct TypeIndexOf
{};

template <typename T, typename... A>
struct TypeIndexOf<T, TypePack<T, A...>>
{
  static constexpr size_t value = 0;
};

template <typename T, typename U, typename... A>
struct TypeIndexOf<T, TypePack<U, A...>>
{
  static constexpr size_t value = 1 + TypeIndexOf<T, TypePack<A...>>::value;
};

template <bool C, typename T, typename F>
struct TypeSelect
{
  typedef T Type;
};
template <typename T, typename F>
struct TypeSelect<false, T, F>
{
  typedef F Type;
};

template <typename>
struct SizeOfLargestType;

template <typename T, typename... U>
struct SizeOfLargestType<TypePack<T, U...>>
{
  static constexpr size_t Value = sizeof(T) > SizeOfLargestType<TypePack<U...>>::Value ? sizeof(T)
                                                                                       : SizeOfLargestType<TypePack<U...>>::Value;
};

template <>
struct SizeOfLargestType<TypePack<>>
{
  static constexpr size_t Value = 1;
};

template <size_t, typename>
struct TypeFromIndex;

template <typename T, typename... U>
struct TypeFromIndex<0, TypePack<T, U...>>
{
  typedef T Type;
};

template <size_t I, typename T, typename... U>
struct TypeFromIndex<I, TypePack<T, U...>>
{
  typedef typename TypeFromIndex<I - 1, TypePack<U...>>::Type Type;
};
