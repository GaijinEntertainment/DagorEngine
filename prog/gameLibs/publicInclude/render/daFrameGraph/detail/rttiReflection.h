//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string_view.h>
#include <EASTL/type_traits.h>
#include <EASTL/tuple.h>
#include <EASTL/numeric_limits.h>

#include <util/dag_string.h>
#include <vecmath/dag_vecMathDecl.h>


namespace dafg::detail
{

/*
  cycling over range in compile time;
  will generate given code with I having values from 0 to N-1;

  usage:

  ForLoop<N>::iterate([&]<size_t I>() {

    code, using parameter I

  });
*/
template <size_t N>
struct ForLoop
{
public:
  template <typename Func>
  static void iterate(Func &&lambda)
  {
    iterateImpl(eastl::forward<Func>(lambda), eastl::make_index_sequence<N>{});
  }

private:
  template <typename Func, size_t... Is>
  static void iterateImpl(Func &&lambda, eastl::index_sequence<Is...>)
  {
    (lambda.template operator()<Is>(), ...);
  }
};


consteval const char *ce_strstr(const char *haystack, const char *needle)
{
  for (; *haystack; ++haystack)
  {
    const char *h = haystack;
    const char *n = needle;
    for (; *h && *n && *h == *n; ++h, ++n)
      ;
    if (!*n)
      return haystack;
  }
  return nullptr;
}

consteval size_t ce_strlen(const char *s)
{
  size_t n = 0;
  while (*s++)
    ++n;
  return n;
}

// gets string `func_name`, containing template parameter T name like "...T = <name>...";
// and string like "T = " as `parameter_header`;
// returns string view of name of that parameter;
// see get_type_name<T>() / get_value_name<V>() for example;
consteval eastl::string_view get_template_parameter_name(const char *func_name, const char *parameter_header)
{
  const char *begin = "";
  const char *end = begin;

#if defined(__clang__) || defined(__GNUC__)
  if (const char *typeName = ce_strstr(func_name, parameter_header); typeName != nullptr)
  {
    begin = typeName + ce_strlen(parameter_header);
    for (end = begin; *end && (*end != ';') && (*end != ']'); ++end)
      ;
  }
#elif _MSC_VER
  G_UNUSED(parameter_header);
  for (end = func_name; *end; ++end)
    ;
  for (--end; (end != func_name) && *end != '>'; --end)
    ;
  int8_t braceDepth = 1;
  for (begin = end; (begin != func_name); --begin)
  {
    if (*(begin - 1) == '>')
      ++braceDepth;
    if (*(begin - 1) == '<')
      --braceDepth;
    if (braceDepth == 0)
      break;
  }
#endif

  return eastl::string_view(begin, end - begin);
}


// returns name of given struct T;
template <class T>
consteval eastl::string_view get_type_name()
{
  if constexpr (eastl::is_same_v<T, vec4f>)
    return eastl::string_view{"vec4f"};

  if constexpr (eastl::is_same_v<T, vec4i>)
    return eastl::string_view{"vec4i"};

  const char *funcName = nullptr;
#if defined(__clang__) || defined(__GNUC__)
  funcName = __PRETTY_FUNCTION__;
#elif _MSC_VER
  funcName = __FUNCSIG__;
#endif
  return get_template_parameter_name(funcName, "T = ");
}

// returns name of given value V;
template <auto V>
consteval eastl::string_view get_value_name()
{
  const char *funcName = nullptr;
#if defined(__clang__) || defined(__GNUC__)
  funcName = __PRETTY_FUNCTION__;
#elif _MSC_VER
  funcName = __FUNCSIG__;
#endif
  return get_template_parameter_name(funcName, "V = ");
}


// for passed "a::b::c::name" returns "name";
constexpr eastl::string_view crop_namespaces(eastl::string_view name)
{
  if (name.size() == 0)
    return eastl::string_view{};

  const char *end = name.data() + name.size();
  const char *begin = end;

  for (; begin != name.data() && *begin != ':' && *begin != ')'; --begin)
    ;
  if (*begin == ':' || *begin == ')')
    ++begin;

  return eastl::string_view(begin, end - begin);
}

// returns name of given enum value;
// passed enum type need to have value named MAX_VAL as max enum value;
// do not use for bit flags, use decode_flag_names() instead;
template <class EnumType>
  requires requires { EnumType::MAX_VAL; }
inline String get_enum_name(EnumType value, bool crop_ns = true)
{
  eastl::string_view enumName{"<out of range>"};
  ForLoop<size_t(EnumType::MAX_VAL) + 1>::iterate([&]<size_t I>() {
    if (size_t(value) == I)
      enumName = get_value_name<EnumType(I)>();
  });
  if (crop_ns)
    enumName = crop_namespaces(enumName);

  return String(0, "%s (%lu)", String(enumName.data(), enumName.size()), static_cast<long unsigned>(value));
}

// returns string with names of given flags;
// do not use for enums, use get_enum_name() instead;
template <class FlagType>
  requires requires { eastl::is_unsigned_v<eastl::underlying_type_t<FlagType>> == eastl::true_type::value; }
inline String decode_flag_names(FlagType value, bool crop_ns = true)
{
  constexpr static size_t BIT_COUNT = sizeof(FlagType) * CHAR_BIT;

  if (value == FlagType(0x0))
  {
    eastl::string_view flagName = get_value_name<FlagType(0x0)>();
    if (crop_ns)
      flagName = crop_namespaces(flagName);
    return String(flagName.data(), flagName.size());
  }

  String result = {};
  ForLoop<BIT_COUNT>::iterate([&]<size_t I>() {
    using U = eastl::underlying_type_t<FlagType>;
    constexpr auto bit = U(0x1) << I;
    if ((bit & eastl::to_underlying(value)) != 0)
    {
      eastl::string_view flagName = get_value_name<FlagType(bit)>();
      if (crop_ns)
        flagName = crop_namespaces(flagName);
      result.aprintf(0, "%s|", String(flagName.data(), flagName.size()));
    }
  });
  result.pop();
  return result;
}


// counting number of fields of given struct T

template <size_t I>
struct UbiqConstructor
{
  template <class Type>
  constexpr operator Type &() const noexcept;
};

template <class T, class... Args>
concept AggregateConstructibleFrom = requires(Args... args) { T{args...}; };

template <class T, size_t... Is>
constexpr size_t count_fields(eastl::index_sequence<Is...>)
{
  return sizeof...(Is) - 1;
}

template <class T, size_t... Is>
  requires AggregateConstructibleFrom<T, UbiqConstructor<Is>...>
constexpr size_t count_fields(eastl::index_sequence<Is...>)
{
  return count_fields<T>(eastl::index_sequence<0, Is...>{});
}

// generating tuple of field references from given struct T
// can handle structures with up to 64 members, what sounds quite enough

template <size_t N>
using size_t_ = eastl::integral_constant<size_t, N>;

template <class T>
constexpr auto tie_as_tuple(const T &, size_t_<0>)
{
  return eastl::tie();
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<1>)
{
  auto &[f0] = val;
  return eastl::tie(f0);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<2>)
{
  auto &[f0, f1] = val;
  return eastl::tie(f0, f1);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<3>)
{
  auto &[f0, f1, f2] = val;
  return eastl::tie(f0, f1, f2);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<4>)
{
  auto &[f0, f1, f2, f3] = val;
  return eastl::tie(f0, f1, f2, f3);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<5>)
{
  auto &[f0, f1, f2, f3, f4] = val;
  return eastl::tie(f0, f1, f2, f3, f4);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<6>)
{
  auto &[f0, f1, f2, f3, f4, f5] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<7>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<8>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<9>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<10>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<11>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<12>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<13>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<14>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<15>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<16>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<17>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<18>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<19>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<20>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<21>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<22>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<23>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<24>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<25>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<26>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<27>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26] =
    val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<28>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<29>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<30>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<31>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<32>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<33>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<34>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<35>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<36>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<37>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<38>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<39>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<40>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<41>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<42>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<43>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<44>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<45>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<46>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<47>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<48>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<49>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<50>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<51>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<52>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<53>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52] =
    val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<54>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52,
    f53] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52, f53);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<55>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52,
    f53, f54] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52, f53, f54);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<56>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52,
    f53, f54, f55] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52, f53, f54, f55);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<57>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52,
    f53, f54, f55, f56] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52, f53, f54, f55, f56);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<58>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52,
    f53, f54, f55, f56, f57] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52, f53, f54, f55, f56, f57);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<59>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52,
    f53, f54, f55, f56, f57, f58] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52, f53, f54, f55, f56, f57, f58);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<60>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52,
    f53, f54, f55, f56, f57, f58, f59] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52, f53, f54, f55, f56, f57, f58, f59);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<61>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52,
    f53, f54, f55, f56, f57, f58, f59, f60] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52, f53, f54, f55, f56, f57, f58, f59, f60);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<62>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52,
    f53, f54, f55, f56, f57, f58, f59, f60, f61] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52, f53, f54, f55, f56, f57, f58, f59, f60, f61);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<63>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52,
    f53, f54, f55, f56, f57, f58, f59, f60, f61, f62] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52, f53, f54, f55, f56, f57, f58, f59, f60, f61, f62);
}
template <class T>
constexpr auto tie_as_tuple(const T &val, size_t_<64>)
{
  auto &[f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26,
    f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52,
    f53, f54, f55, f56, f57, f58, f59, f60, f61, f62, f63] = val;
  return eastl::tie(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46, f47, f48, f49, f50,
    f51, f52, f53, f54, f55, f56, f57, f58, f59, f60, f61, f62, f63);
}

} // namespace dafg::detail
