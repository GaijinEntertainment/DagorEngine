// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <EASTL/algorithm.h>

#include "util/type_lists.h"

namespace drv3d_vulkan
{
// WHISH LIST:
// Add extension version checking (throw out extension below expected version on load)
//
// Encode dependencies directly into extension (mutual exclusive, core since, requirements)
//
// Automatic sorting by dependencies and validation if dependency is available or not
//
// Automatic aliasing for core functions for vulkan core > 1.0
//
// Automatic deprecation mechanism for vulkan core > 1.0 (some extensions become obsolete
// when some alternatives become core, example would be amd negative viewport)
//
// Shader extension caps, not handled at all

// For each extension type there has to be an overload provided
// Has to be a function, gcc/linux linker complains about duplicate defs with variable template
// TODO: figure out how to convince gcc/linux linker to make it translation unit private so
// we can value template instead
template <typename T>
inline constexpr const char *extension_name()
{
  return T::name;
}

// TODO: roll this into extension base?
// Adapts the interface of an extension to be usable with the extension groups below
template <typename T>
struct ExtensionAdaptor
{
  static bool has(dag::ConstSpan<const char *> ext_list)
  {
    return eastl::end(ext_list) != eastl::find_if(eastl::begin(ext_list), eastl::end(ext_list),
                                     [](const char *e_name) { return 0 == strcmp(extension_name<T>(), e_name); });
  }

  static uint32_t filter(const char **ext_list, uint32_t count)
  {
    for (uint32_t i = 0; i < count; ++i)
    {
      if (0 == strcmp(extension_name<T>(), ext_list[i]))
      {
        ext_list[i] = ext_list[count - 1];
        return count - 1;
      }
    }
    return count;
  }
  static void enumerate(void (*clb)(const char *)) { clb(extension_name<T>()); }

  template <typename O>
  static bool hasExtension(O &obj)
  {
    return obj.template hasExtension<T>();
  }
};

namespace detail
{
template <typename T, typename U>
struct FilterExec
{
  static uint32_t filter(const char **ext_list, uint32_t count) { return U::filter(ext_list, count); }
};

template <typename A>
struct FilterExec<A, A>
{
  static uint32_t filter(const char **, uint32_t count) { return count; }
};

template <typename T>
struct FilterExceptThis
{
  template <typename U1, typename U2, typename... R>
  static uint32_t filter(const char **ext_list, uint32_t count, TypePack<U1, U2, R...>)
  {
    return filter(ext_list, FilterExec<T, U1>::filter(ext_list, count), TypePack<U2, R...>{});
  }
  template <typename U>
  static uint32_t filter(const char **ext_list, uint32_t count, TypePack<U>)
  {
    return FilterExec<T, U>::filter(ext_list, count);
  }
};
} // namespace detail

// A set of extensions where at least one extension has to be supported
// and if multiple are supported, the first supported is chosen.
template <typename... Extensions>
struct MutualExclusiveExtensions
{
  template <typename T1, typename T2, typename... L>
  static bool hasOneOf(dag::ConstSpan<const char *> ext_list)
  {
    return T1::has(ext_list) || hasOneOf<T2, L...>(ext_list);
  }
  template <typename T>
  static bool hasOneOf(dag::ConstSpan<const char *> ext_list)
  {
    return T::has(ext_list);
  }
  static bool has(dag::ConstSpan<const char *> ext_list) { return hasOneOf<Extensions...>(ext_list); }

  template <typename O, typename T1, typename T2, typename... L>
  static bool hasOneOfExtensions(O &obj)
  {
    return T1::hasExtension(obj) || hasOneOfExtensions<O, T2, L...>(obj);
  }
  template <typename O, typename T1>
  static bool hasOneOfExtensions(O &obj)
  {
    return T1::hasExtension(obj);
  }
  template <typename O>
  static bool hasExtensions(O &obj)
  {
    return hasOneOfExtensions<O, Extensions...>(obj);
  }

  template <typename T1, typename T2, typename... R>
  static uint32_t applyFilter(const char **ext_list, uint32_t count)
  {
    if (T1::has(dag::ConstSpan<const char *>(ext_list, count)))
      return detail::FilterExceptThis<T1>::filter(ext_list, count, TypePack<Extensions...>{});
    return applyFilter<T2, R...>(ext_list, count);
  }
  template <typename T>
  static uint32_t applyFilter(const char **ext_list, uint32_t count)
  {
    if (T::has(dag::ConstSpan<const char *>(ext_list, count)))
      return detail::FilterExceptThis<T>::filter(ext_list, count, TypePack<Extensions...>{});
    return count;
  }
  static uint32_t filter(const char **ext_list, uint32_t count) { return applyFilter<Extensions...>(ext_list, count); }

  template <typename T1, typename T2, typename... R>
  static uint32_t removeExtensions(const char **ext_list, uint32_t count)
  {
    return removeExtensions<T2, R...>(ext_list, T1::filter(ext_list, count));
  }
  template <typename T>
  static uint32_t removeExtensions(const char **ext_list, uint32_t count)
  {
    return T::filter(ext_list, count);
  }

  static uint32_t remove(const char **ext_list, uint32_t count) { return removeExtensions<Extensions...>(ext_list, count); }

  template <typename T1, typename T2, typename... R>
  static void enumerateCall(void (*clb)(const char *))
  {
    T1::enumerate(clb);
    enumerateCall<T2, R...>(clb);
  }

  template <typename T>
  static void enumerateCall(void (*clb)(const char *))
  {
    T::enumerate(clb);
  }

  static void enumerate(void (*clb)(const char *)) { enumerateCall<Extensions...>(clb); }
};

// A set of extensions where all extensions are required to be supported
template <typename... Extensions>
struct RequiresAllExtensions
{
  template <typename T1, typename T2, typename... L>
  static bool hasAll(dag::ConstSpan<const char *> ext_list)
  {
    return T1::has(ext_list) && hasAll<T2, L...>(ext_list);
  }
  template <typename T>
  static bool hasAll(dag::ConstSpan<const char *> ext_list)
  {
    return T::has(ext_list);
  }
  static bool has(dag::ConstSpan<const char *> ext_list) { return hasAll<Extensions...>(ext_list); }

  template <typename O, typename T1, typename T2, typename... L>
  static bool hasAllExtensions(O &obj)
  {
    return T1::hasExtension(obj) && hasAllExtensions<O, T2, L...>(obj);
  }
  template <typename O, typename T1>
  static bool hasAllExtensions(O &obj)
  {
    return T1::hasExtension(obj);
  }
  template <typename O>
  static bool hasExtensions(O &obj)
  {
    return hasAllExtensions<O, Extensions...>(obj);
  }

  static uint32_t filter(const char **, uint32_t count) { return count; }

  template <typename T1, typename T2, typename... R>
  static uint32_t removeExtensions(const char **ext_list, uint32_t count)
  {
    return removeExtensions<T2, R...>(ext_list, T1::filter(ext_list, count));
  }
  template <typename T>
  static uint32_t removeExtensions(const char **ext_list, uint32_t count)
  {
    return T::filter(ext_list, count);
  }

  static uint32_t remove(const char **ext_list, uint32_t count) { return removeExtensions<Extensions...>(ext_list, count); }

  template <typename T1, typename T2, typename... R>
  static void enumerateCall(void (*clb)(const char *))
  {
    T1::enumerate(clb);
    enumerateCall<T2, R...>(clb);
  }

  template <typename T>
  static void enumerateCall(void (*clb)(const char *))
  {
    T::enumerate(clb);
  }

  static void enumerate(void (*clb)(const char *)) { enumerateCall<Extensions...>(clb); }
};

// A set of extensions, where at least one extension must be
// supported.
template <typename... Extensions>
struct RequiresAnyExtension
{
  template <typename T1, typename T2, typename... L>
  static bool hasAtLeastOne(dag::ConstSpan<const char *> ext_list)
  {
    return T1::has(ext_list) || hasAtLeastOne<T2, L...>(ext_list);
  }
  template <typename T>
  static bool hasAtLeastOne(dag::ConstSpan<const char *> ext_list)
  {
    return T::has(ext_list);
  }
  static bool has(dag::ConstSpan<const char *> ext_list) { return hasAtLeastOne<Extensions...>(ext_list); }

  template <typename O, typename T1, typename T2, typename... L>
  static bool hasOneOfExtensions(O &obj)
  {
    return T1::hasExtension(obj) || hasOneOfExtensions<O, T2, L...>(obj);
  }
  template <typename O, typename T1>
  static bool hasOneOfExtensions(O &obj)
  {
    return T1::hasExtension(obj);
  }
  template <typename O>
  static bool hasExtensions(O &obj)
  {
    return hasOneOfExtensions<O, Extensions...>(obj);
  }

  static uint32_t filter(const char **, uint32_t count) { return count; }

  template <typename T1, typename T2, typename... R>
  static uint32_t removeExtensions(const char **ext_list, uint32_t count)
  {
    return removeExtensions<T2, R...>(ext_list, T1::filter(ext_list, count));
  }
  template <typename T>
  static uint32_t removeExtensions(const char **ext_list, uint32_t count)
  {
    return T::filter(ext_list, count);
  }

  static uint32_t remove(const char **ext_list, uint32_t count) { return removeExtensions<Extensions...>(ext_list, count); }

  template <typename T1, typename T2, typename... R>
  static void enumerateCall(void (*clb)(const char *))
  {
    T1::enumerate(clb);
    enumerateCall<T2, R...>(clb);
  }

  template <typename T>
  static void enumerateCall(void (*clb)(const char *))
  {
    T::enumerate(clb);
  }

  static void enumerate(void (*clb)(const char *)) { enumerateCall<Extensions...>(clb); }
};

// behaves like ExtensionAdaptor for an extension that is always supported
struct ExtensionHasAlways
{
  static bool has(dag::ConstSpan<const char *>) { return true; }

  static uint32_t filter(const char **, uint32_t count) { return count; }

  static uint32_t remove(const char **, uint32_t count) { return count; }

  static void enumerate(void (*)(const char *)) {}

  template <typename O>
  static bool hasExtension(O &)
  {
    return true;
  }
};
// behaves like ExtensionAdaptor for an extension that is never supported
struct ExtensionHasNever
{
  static bool has(dag::ConstSpan<const char *>) { return false; }

  static uint32_t filter(const char **, uint32_t count) { return count; }

  static uint32_t remove(const char **, uint32_t count) { return count; }

  static void enumerate(void (*)(const char *)) {}

  template <typename O>
  static bool hasExtension(O &)
  {
    return false;
  }
};

template <typename T>
struct ExtensionBase;

template <typename... Functions>
struct ExtensionBase<TypePack<Functions...>> : Functions...
{
private:
  template <typename U, typename T1, typename T2, typename... TN>
  void enumerateFunction(U clb)
  {
    enumerateFunction<U, T1>(clb);
    enumerateFunction<U, T2, TN...>(clb);
  }
  template <typename U, typename T>
  void enumerateFunction(U clb)
  {
    this->T::inspect(clb);
  }

  template <typename U>
  void enumerateFunction(U)
  {}

public:
  template <typename U>
  void enumerateFunctions(U clb)
  {
    enumerateFunction<U, Functions...>(clb);
  }
};

#if _TARGET_C3








#else
#define VULKAN_MAKE_CORE_FUNCTION_POINTER_MEMBER(name) PFN_##name name = nullptr
#define VULKAN_CORE_FUNCTION_POINTER_INSPECT_HELPER
#define VULKAN_GET_CORE_FUNCTION_POINTER_MEMBER(name) *reinterpret_cast<PFN_vkVoidFunction *>(&name)
#endif

#define VULKAN_MAKE_CORE_FUNCTION_DEF(name)                    \
  struct T##name                                               \
  {                                                            \
    VULKAN_MAKE_CORE_FUNCTION_POINTER_MEMBER(name);            \
    template <typename T>                                      \
    void inspect(T t)                                          \
    {                                                          \
      VULKAN_CORE_FUNCTION_POINTER_INSPECT_HELPER;             \
      t(#name, VULKAN_GET_CORE_FUNCTION_POINTER_MEMBER(name)); \
    }                                                          \
  };

#define VULKAN_EXTENSION_FUNCTION_POINTER_INSPECT_HELPER
#define VULKAN_MAKE_EXTENSION_FUNCTION_POINTER_MEMEBER(name) PFN_##name name = nullptr
#define VULKAN_GET_EXTENSION_FUNCTION_POINTER_MEMBER(name)   *reinterpret_cast<PFN_vkVoidFunction *>(&name)

#define VULKAN_MAKE_EXTENSION_FUNCTION_DEF(name)                    \
  struct T##name                                                    \
  {                                                                 \
    VULKAN_MAKE_EXTENSION_FUNCTION_POINTER_MEMEBER(name);           \
    template <typename T>                                           \
    void inspect(T t)                                               \
    {                                                               \
      VULKAN_EXTENSION_FUNCTION_POINTER_INSPECT_HELPER;             \
      t(#name, VULKAN_GET_EXTENSION_FUNCTION_POINTER_MEMBER(name)); \
    }                                                               \
  };

struct TFunctionTerminate
{
  template <typename T>
  void inspect(T)
  {}
};

#define VULKAN_BEGIN_EXTENSION_FUNCTION_PACK       typedef TypePack <
// pvs thinks that 'name' should be surrounded by parenthesis,
// because it believes the '>' is a greater than comparison,
// which is incorrect in this context.
//-V:VULKAN_END_EXTENSION_FUCTION_PACK:1003
#define VULKAN_END_EXTENSION_FUCTION_PACK(name)    TFunctionTerminate > name##Functions
#define VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(name) T##name,

#define VULKAN_DECLARE_EXTENSION(Name, StringName) \
  struct Name : ExtensionBase<Name##Functions>     \
  {};                                              \
  template <>                                      \
  constexpr const char *extension_name<Name>()     \
  {                                                \
    return VK_##StringName##_EXTENSION_NAME;       \
  }

} // namespace drv3d_vulkan
