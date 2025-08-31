#pragma once

#include <daScript/daScript.h>

// Contains macros, that will simplify binding most of the stuff,
// however in more complex cases you can still use addExtern/addField/addProperty
// and declare everything manually
//
// When DAS_AOT_COMPILER is defined, it will bind stubs instead of functions
//
// ** FUNCTIONS **
// Simple function binding:
//  - sideEffects are from das::SideEffects namespace (only name)
//  - functionName must be relative to global namespace, but without :: prefix
// `DAS_ADD_FUNC_BIND("das_func_name", sideEffects, functionName);`
// If function returns struct by value, use this instead
// `DAS_ADD_FUNC_BIND_VALUE_RET("das_func_name", sideEffects, functionName);`
// In case there are several functions overloads, you must specify it explicitly:
// `DAS_ADD_FUNC_BIND("das_func_name", sideEffects, DAS_OVERLOAD(functionName, ArgType1, ArgType2, ...));`
//
// ** CLASS METHODS **
// Simple class method binding (mostly similar to function binding)
// - As previously ClassName must be relative to global namespace, but without :: prefix
// - Resulting binding will take ClassName & as its first argument, before others
// - DAS_ADD_METHOD_BIND_VALUE_RET must be used for methods, returning structs by value
// `DAS_ADD_METHOD_BIND("das_func_name", sideEffects, ClassName::methodName)`
// Overloads work as previously:
// `DAS_ADD_METHOD_BIND("das_func_name", sideEffects, DAS_OVERLOAD(ClassName::methodName, ArgType1, ArgType2, ...));`
// In case method have const and non-const overloads, this will resolve and bind both of them:
// - Note that if there are both const/non-const overloads AND different argument overloads
//   this will not work (although this is a rare case), see DAS_OVERLOAD_EX later
// - DAS_ADD_METHOD_BIND_CONST_VALUE_RET - alternative for return-by-value
// `DAS_ADD_METHOD_BIND_CONST("das_func_name", sideEffects, ClassName::methodName)`
//
// ** STRUCT ANNOTATIONS **
// Declare struct in module header file:
// - CppTypeName must be relative to global namespace
// `DAS_TYPE_DECL(DasTypeName, CppTypeName)`
// Annotate struct in module CPP file:
// ```
// DAS_TYPE_ANNOTATION(DasTypeName)
// {
//   // ... add fields and properties
// }
// ```
// add annotation in module body
// `addAnnotation(das::make_smart<DasTypeNameAnnotation>(lib));`
//
// ** FIELDS (inside DAS_TYPE_ANNOTATION body) **
// add field:
// `DAS_ADD_FIELD_BIND(FieldName)`
// add field with different name in DAS:
// `DAS_ADD_FIELD_BIND_EX("DasFieldName", FieldName)`
//
// ** PROPERTIES (inside DAS_TYPE_ANNOTATION body) **
// add property (property must be a method without arguments):
// - This will attempt to resolve method overload without arguments,
//   so exactly one such overload must be present
// DAS_ADD_PROP_BIND("PropertyName", getProperty)
// add property with const and non-const method overloads
// - both overloads must be present for this to compile,
//   otherwise use DAS_ADD_PROP_BIND
// DAS_ADD_PROP_BIND_CONST("PropertyName", getProperty)

// ** MODULES **
// Note that this is intended just for simple modules,
// with no extra stuff, just some bindings. For more
// complex ones, implement them yourself
// Declare a module:
// - Module must be declared in global namespace for this macro
// - NEED_MODULE(CPPModuleName) must be used to pull it
// DAS_MODULE_DECL_EX(CPPModuleName, "DasModuleName", "path/to/module/aotInclude.h")
// {
//   // all annotations go here
//   // - addAnnotation/addEnumeration
//   // - DAS_ADD_FUN_BINDING/DAS_ADD_METHOD_BINDING
//   // - ...
// }
// alternatively if CPPModuleName is 'DasModuleNameModule'
// DAS_MODULE_DECL_EX(DasModuleName, "path/to/module/aotInclude.h")


// Must be defined, when building AOT compiler, to generate stubs and avoid linker errors
// #define DAS_AOT_COMPILER


namespace das::detail
{

template <typename RetT, typename... ArgsT>
___noinline RetT the_great_asserting_stub(ArgsT...)
{
  DAS_ASSERTF(0, "STUB!");
  if constexpr (!das::is_same_v<RetT, void>)
  {
    void *volatile the_null = nullptr;
    // this way we avoid invoking constructor in case of returning by value, and just crash on nullptr call
    return reinterpret_cast<RetT (*)()>(the_null)();
  }
}

template <typename RetT, typename... ArgsT>
RetT the_great_empty_stub(ArgsT...)
{
  if constexpr (!das::is_same_v<RetT, void>)
    return RetT();
}

#define DAS_STUB_IMPL(ASSERT, RET)                         \
  if constexpr (!das::is_same_v<RET, void>)                \
  {                                                        \
    if constexpr (ASSERT)                                  \
      return das::detail::the_great_asserting_stub<RET>(); \
    else                                                   \
      return das::detail::the_great_empty_stub<RET>();     \
  }                                                        \
  DAS_ASSERTF(!(ASSERT), "STUB!");


template <bool Assert, typename RetT, typename... ArgsT>
constexpr auto get_stub(RetT (*)(ArgsT...))
{
  if constexpr (Assert)
    return &the_great_asserting_stub<RetT, ArgsT...>;
  else
    return &the_great_empty_stub<RetT, ArgsT...>;
}

template <bool Assert, typename ThisT, typename RetT, typename... ArgsT>
constexpr auto get_stub(RetT (ThisT::*)(ArgsT...))
{
  return get_stub<Assert>(static_cast<RetT (*)(ThisT &, ArgsT...)>(nullptr));
}

template <bool Assert, typename ThisT, typename RetT, typename... ArgsT>
constexpr auto get_stub(RetT (ThisT::*)(ArgsT...) const)
{
  return get_stub<Assert>(static_cast<RetT (*)(const ThisT &, ArgsT...)>(nullptr));
}

template <typename FnT, FnT ptr>
struct FunOverloadWrap
{
  constexpr FunOverloadWrap() = default;
  constexpr FnT operator&() const { return ptr; }
  constexpr operator FnT() const { return ptr; }
};

template <typename... Args>
struct ResolveOverload
{
  template <typename R>
  constexpr static auto find_overload(R (*fn)(Args...))
  {
    return fn;
  }
  template <typename T, typename R>
  constexpr static auto find_overload(R (T::*fn)(Args...))
  {
    return fn;
  }
  template <typename T, typename R>
  constexpr static auto find_overload(R (T::*fn)(Args...) const)
  {
    return fn;
  }
};

template <typename R, typename... Args>
struct ResolveOverload<R(Args...)>
{
  constexpr static auto find_overload(R (*fn)(Args...)) { return fn; }
  template <typename T>
  constexpr static auto find_overload(R (T::*fn)(Args...))
  {
    return fn;
  }
  template <typename T>
  constexpr static auto find_overload(R (T::*fn)(Args...) const)
  {
    return fn;
  }
};

template <typename T, typename R, typename... Args>
constexpr auto get_non_const_method(R (T::*ptr)(Args...))
{
  return ptr;
}

template <typename T, typename R, typename... Args>
constexpr auto get_const_method(R (T::*ptr)(Args...) const)
{
  return ptr;
}

template <typename T, typename R>
constexpr auto get_non_const_prop_method(R (T::*ptr)())
{
  return ptr;
}

template <typename T, typename R>
constexpr auto get_const_prop_method(R (T::*ptr)() const)
{
  return ptr;
}

template <typename T, typename R>
constexpr auto get_prop_method(R (T::*ptr)())
{
  return ptr;
}

template <typename T, typename R>
constexpr auto get_prop_method(R (T::*ptr)() const)
{
  return ptr;
}

#ifdef DAS_AOT_COMPILER
template <bool Assert, bool IsConst, typename ThisT, typename RetT>
struct CallPropertyStubImpl
{
  typedef RetT RetType;
  enum
  {
    ref = das::is_reference<RetType>::value,
    isConst = IsConst,
  };
  static RetT static_call(das::conditional_t<IsConst, const ThisT &, ThisT &>) { DAS_STUB_IMPL(Assert, RetT); }
};
#endif

#undef DAS_STUB_IMPL

template <typename T>
struct GetInvokeResult
{};
template <typename R, typename... Args>
struct GetInvokeResult<R (*)(Args...)>
{
  using type = R;
};

template <typename FunT, FunT fn>
inline auto addExternAutoSelectSimNode(Module &mod, const ModuleLibrary &lib, const char *name, SideEffects seFlags,
  const char *cppName)
{
  using ResultT = typename GetInvokeResult<FunT>::type;
  if constexpr (das::is_reference_v<ResultT>)
    return addExtern<FunT, fn, SimNode_ExtFuncCallRef>(mod, lib, name, seFlags, cppName);
  else
    return addExtern<FunT, fn, SimNode_ExtFuncCall>(mod, lib, name, seFlags, cppName);
}

template <typename ClassT, typename...>
using ResolveCtorAot = ClassT;

template <typename... CharT>
static inline void write_module_includes(das::TextWriter &tw, const CharT *...paths)
{
  (
    [&] {
      const char *path = paths;
      DAS_ASSERTF(path, "null include path");
      while (*path && *path == ' ')
        path++;
      DAS_ASSERTF(*path, "empty include path");
      if (*path != '<' && *path != '"')
        tw << "#include \"" << path << "\"\n";
      else
        tw << "#include " << path << "\n";
    }(),
    ...);
}

} // namespace das::detail

#ifdef DAS_AOT_COMPILER
template <typename RetT, typename ThisT, RetT (*Fn)(ThisT &)>
struct das::CallProperty<RetT (*)(ThisT &), Fn> : das::detail::CallPropertyStubImpl<true, das::is_const_v<ThisT>, ThisT, RetT>
{};
template <typename RetT, typename ThisT, typename ClassT, RetT (*Fn)(ClassT &)>
struct das::CallPropertyForType<ThisT, RetT (*)(ClassT &), Fn>
  : das::detail::CallPropertyStubImpl<true, das::is_const_v<ClassT>, ThisT, RetT>
{};
#endif

#ifdef DAS_AOT_COMPILER
#define DAS_SHOULD_USE_STUBS 1
#define DAS_FUN_DECL(...)    decltype(&__VA_ARGS__), das::detail::get_stub<true>(decltype (&__VA_ARGS__)())
#define DAS_PROP_OR_METHOD_DECL_STUB_IMPL(...) \
  decltype(das::detail::get_stub<true>(__VA_ARGS__)), das::detail::get_stub<true>(decltype(__VA_ARGS__)())
#define DAS_METHOD_DECL(...) DAS_PROP_OR_METHOD_DECL_STUB_IMPL(&__VA_ARGS__)
#define DAS_PROP_DECL(...)   DAS_PROP_OR_METHOD_DECL_STUB_IMPL(das::detail::get_prop_method(&__VA_ARGS__))
#define DAS_PROP_DECL_CONST(...)                                                           \
  DAS_PROP_OR_METHOD_DECL_STUB_IMPL(das::detail::get_non_const_prop_method(&__VA_ARGS__)), \
    DAS_PROP_OR_METHOD_DECL_STUB_IMPL(das::detail::get_const_prop_method(&__VA_ARGS__))
#else
#define DAS_SHOULD_USE_STUBS 0
#define DAS_FUN_DECL(...)    decltype(&__VA_ARGS__), (&__VA_ARGS__)
#define DAS_METHOD_DECL(...) \
  decltype(das::detail::get_stub<true>(&__VA_ARGS__)), &das::das_call_member<decltype(&__VA_ARGS__), &__VA_ARGS__>::invoke
#define DAS_PROP_DECL(...) decltype(das::detail::get_prop_method(&__VA_ARGS__)), &__VA_ARGS__
#define DAS_PROP_DECL_CONST(...)                                                \
  decltype(das::detail::get_non_const_prop_method(&__VA_ARGS__)), &__VA_ARGS__, \
    decltype(das::detail::get_const_prop_method(&__VA_ARGS__)), &__VA_ARGS__
#endif

#define DAS_MACRO_STRINGIFY3(...) #__VA_ARGS__
#define DAS_MACRO_STRINGIFY2(...) DAS_MACRO_STRINGIFY3(__VA_ARGS__)
#define DAS_MACRO_STRINGIFY1(...) DAS_MACRO_STRINGIFY2(__VA_ARGS__)
#define DAS_MACRO_STRINGIFY0(...) DAS_MACRO_STRINGIFY1(__VA_ARGS__)
#define DAS_MACRO_STRINGIFY(...)  DAS_MACRO_STRINGIFY0(__VA_ARGS__)

#define DAS_OVERLOAD_EX(TYPE, ...)         das::detail::FunOverloadWrap<TYPE, __VA_ARGS__>()
#define DAS_OVERLOAD(FUNC, ...)            DAS_OVERLOAD_EX(decltype(das::detail::ResolveOverload<__VA_ARGS__>::find_overload(&::FUNC)), &::FUNC)
#define DAS_METHOD_OVERLOAD_CONST(...)     DAS_OVERLOAD_EX(decltype(das::detail::get_const_method(&__VA_ARGS__)), &__VA_ARGS__)
#define DAS_METHOD_OVERLOAD_NON_CONST(...) DAS_OVERLOAD_EX(decltype(das::detail::get_non_const_method(&__VA_ARGS__)), &__VA_ARGS__)

// fields/props
#define DAS_ADD_FIELD_BIND_EX(DAS_NAME, NAME) addField<DAS_BIND_MANAGED_FIELD(NAME)>(DAS_NAME, DAS_MACRO_STRINGIFY(NAME))
#define DAS_ADD_FIELD_BIND(NAME)              DAS_ADD_FIELD_BIND_EX(DAS_MACRO_STRINGIFY(NAME), NAME)
#define DAS_ADD_PROP_BIND(DAS_NAME, ...) \
  addPropertyForManagedType<DAS_PROP_DECL(ManagedType::__VA_ARGS__)>(DAS_NAME, DAS_MACRO_STRINGIFY(__VA_ARGS__))
#define DAS_ADD_PROP_BIND_CONST(DAS_NAME, ...) \
  addPropertyExtConstForManagedType<DAS_PROP_DECL_CONST(ManagedType::__VA_ARGS__)>(DAS_NAME, DAS_MACRO_STRINGIFY(__VA_ARGS__))

// functions
#define DAS_ADD_FUN_BIND(NAME, SIDE_EFFECTS, ...)                                                                        \
  das::detail::addExternAutoSelectSimNode<DAS_FUN_DECL(::__VA_ARGS__)>(*this, lib, NAME, das::SideEffects::SIDE_EFFECTS, \
    "::" DAS_MACRO_STRINGIFY(__VA_ARGS__))
#define DAS_ADD_FUN_BIND_VALUE_RET(NAME, SIDE_EFFECTS, ...)                                            \
  das::addExtern<DAS_FUN_DECL(::__VA_ARGS__), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, NAME, \
    das::SideEffects::SIDE_EFFECTS, "::" DAS_MACRO_STRINGIFY(__VA_ARGS__))
#define DAS_ADD_METHOD_BIND(NAME, SIDE_EFFECTS, ...)                                                                        \
  das::detail::addExternAutoSelectSimNode<DAS_METHOD_DECL(::__VA_ARGS__)>(*this, lib, NAME, das::SideEffects::SIDE_EFFECTS, \
    "::das::das_call_member<decltype(&::" DAS_MACRO_STRINGIFY(__VA_ARGS__) "),&::" DAS_MACRO_STRINGIFY(__VA_ARGS__) ">::invoke")
#define DAS_ADD_METHOD_BIND_VALUE_RET(NAME, SIDE_EFFECTS, ...)                                            \
  das::addExtern<DAS_METHOD_DECL(::__VA_ARGS__), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, NAME, \
    das::SideEffects::SIDE_EFFECTS,                                                                       \
    "::das::das_call_member<decltype(&::" DAS_MACRO_STRINGIFY(__VA_ARGS__) "),&::" DAS_MACRO_STRINGIFY(__VA_ARGS__) ">::invoke")
#define DAS_ADD_METHOD_BIND_CONST_EX(NAME, SIDE_EFFECTS_CONST, SIDE_EFFECTS_NON_CONST, ...) \
  DAS_ADD_METHOD_BIND(NAME, SIDE_EFFECTS_CONST, DAS_METHOD_OVERLOAD_CONST(__VA_ARGS__));    \
  DAS_ADD_METHOD_BIND(NAME, SIDE_EFFECTS_NON_CONST, DAS_METHOD_OVERLOAD_NON_CONST(__VA_ARGS__))
#define DAS_ADD_METHOD_BIND_CONST(NAME, SIDE_EFFECTS, ...) DAS_ADD_METHOD_BIND_CONST_EX(NAME, SIDE_EFFECTS, SIDE_EFFECTS, __VA_ARGS__)
#define DAS_ADD_METHOD_BIND_CONST_VALUE_RET_EX(NAME, SIDE_EFFECTS_CONST, SIDE_EFFECTS_NON_CONST, ...) \
  DAS_ADD_METHOD_BIND_VALUE_RET(NAME, SIDE_EFFECTS_CONST, DAS_METHOD_OVERLOAD_CONST(__VA_ARGS__));    \
  DAS_ADD_METHOD_BIND_VALUE_RET(NAME, SIDE_EFFECTS_NON_CONST, DAS_METHOD_OVERLOAD_NON_CONST(__VA_ARGS__))
#define DAS_ADD_METHOD_BIND_CONST_VALUE_RET(NAME, SIDE_EFFECTS, ...) \
  DAS_ADD_METHOD_BIND_CONST_VALUE_RET_EX(NAME, SIDE_EFFECTS, SIDE_EFFECTS, __VA_ARGS__)

// ctor/using
#define DAS_ADD_USING_BIND(...) das::addUsing<::__VA_ARGS__>(*this, lib, "::das::detail::ResolveCtorAot<::" #__VA_ARGS__ ">")
#define DAS_ADD_CTOR_BIND_EX(NAME, ...) \
  das::addCtor<::__VA_ARGS__>(*this, lib, NAME, "::das::detail::ResolveCtorAot<::" #__VA_ARGS__ ">")
#define DAS_ADD_CTOR_BIND(...) DAS_ADD_CTOR_BIND_EX((das::typeName<das::detail::ResolveCtorAot<::__VA_ARGS__>>::name()), __VA_ARGS__)
#define DAS_ADD_CTOR_AND_USING_BIND_EX(NAME, ...) \
  das::addCtorAndUsing<::__VA_ARGS__>(*this, lib, NAME, "::das::detail::ResolveCtorAot<::" #__VA_ARGS__ ">")
#define DAS_ADD_CTOR_AND_USING_BIND(...) \
  DAS_ADD_CTOR_AND_USING_BIND_EX((das::typeName<das::detail::ResolveCtorAot<::__VA_ARGS__>>::name()), __VA_ARGS__)

// type annotations
#define DAS_TYPE_DECL_STUB(...)                               \
  template <>                                                 \
  struct das::is_stub_type<__VA_ARGS__>                       \
  {                                                           \
    static constexpr bool value = bool(DAS_SHOULD_USE_STUBS); \
  };
#define DAS_TYPE_DECL_NAME(NAME, ...)   \
  template <>                           \
  struct das::typeName<__VA_ARGS__>     \
  {                                     \
    constexpr static const char *name() \
    {                                   \
      return NAME;                      \
    }                                   \
  };

#define DAS_TYPE_DECL_BEGIN_EX(DAS_NAME, ANN_NAME, ...)                          \
  struct ANN_NAME : das::ManagedStructureAnnotation<::__VA_ARGS__, false>        \
  {                                                                              \
    static constexpr const char *CPP_NAME = "::" #__VA_ARGS__;                   \
    void annotationBody(das::ModuleLibrary &ml);                                 \
    ANN_NAME(das::ModuleLibrary &ml) : ManagedStructureAnnotation(#DAS_NAME, ml) \
    {                                                                            \
      cppName = CPP_NAME;                                                        \
      annotationBody(ml);                                                        \
    }
#define DAS_TYPE_DECL_BEGIN(DAS_NAME, ...)    \
  MAKE_TYPE_FACTORY(DAS_NAME, ::__VA_ARGS__); \
  DAS_TYPE_DECL_STUB(::__VA_ARGS__);          \
  DAS_TYPE_DECL_BEGIN_EX(DAS_NAME, DAS_NAME##Annotation, __VA_ARGS__)
#define DAS_TYPE_DECL_END            }
#define DAS_TYPE_DECL(DAS_NAME, ...) DAS_TYPE_DECL_BEGIN(DAS_NAME, __VA_ARGS__) DAS_TYPE_DECL_END

#define DAS_TYPE_ANNOTATION_EX(ANN_NAME) void ANN_NAME::annotationBody([[maybe_unused]] das::ModuleLibrary &ml)
#define DAS_TYPE_ANNOTATION(DAS_NAME)    DAS_TYPE_ANNOTATION_EX(DAS_NAME##Annotation)

// module decl helper

#define DAS_MODULE_DECL_EX_BEGIN(MODULE_NAME, DAS_NAME, ...)          \
  struct MODULE_NAME final : public das::Module                       \
  {                                                                   \
    das::ModuleAotType aotRequire(das::TextWriter &tw) const override \
    {                                                                 \
      das::detail::write_module_includes(tw, __VA_ARGS__);            \
      return das::ModuleAotType::cpp;                                 \
    }                                                                 \
    void moduleDeclBody(das::ModuleLibrary &lib);                     \
    MODULE_NAME() : das::Module(DAS_NAME)                             \
    {                                                                 \
      das::ModuleLibrary lib(this);                                   \
      moduleDeclBody(lib);                                            \
      verifyAotReady();                                               \
    }
#define DAS_MODULE_DECL_EX_END            }
#define DAS_MODULE_DECL_BODY(MODULE_NAME) void ::MODULE_NAME::moduleDeclBody([[maybe_unused]] das::ModuleLibrary &lib)

#define DAS_MODULE_DECL_EX(MODULE_NAME, DAS_NAME, ...)                                 \
  DAS_MODULE_DECL_EX_BEGIN(MODULE_NAME, DAS_NAME, __VA_ARGS__) DAS_MODULE_DECL_EX_END; \
  REGISTER_MODULE(MODULE_NAME);                                                        \
  DAS_MODULE_DECL_BODY(MODULE_NAME)
#define DAS_MODULE_DECL(MODULE_NAME, ...) DAS_MODULE_DECL_EX(MODULE_NAME##Module, #MODULE_NAME, __VA_ARGS__)
