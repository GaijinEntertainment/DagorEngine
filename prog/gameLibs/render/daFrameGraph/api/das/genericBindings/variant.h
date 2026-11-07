// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/variant.h>
#include <EASTL/utility.h>
#include <dasModules/dasModulesCommon.h>


namespace das
{

template <typename VariantType>
struct VariantBinding;

template <typename... Ts>
struct typeName<eastl::variant<Ts...>>
{
  static string name() { return string(VariantBinding<eastl::variant<Ts...>>::name); }
};

template <size_t I, typename VT>
bool variant_is(const VT &v)
{
  return v.index() == I;
}
template <size_t I, typename VT>
eastl::variant_alternative_t<I, VT> &variant_as(VT &v)
{
  G_ASSERT(v.index() == I);
  return eastl::get<I>(v);
}
template <size_t I, typename VT>
eastl::variant_alternative_t<I, VT> *variant_safe_as(VT &v)
{
  return eastl::get_if<I>(&v);
}
template <size_t I, typename VT>
void variant_emplace(VT &v)
{
  v.template emplace<I>();
}

template <typename VT, size_t I>
struct VariantAlternativeOps
{
  using Alternative = eastl::variant_alternative_t<I, VT>;
  static constexpr bool HAS_VALUE = !eastl::is_same_v<Alternative, eastl::monostate>;

  static bool is(const VT &v) { return variant_is<I>(v); }
  static Alternative &as(VT &v) { return variant_as<I>(v); }
  static Alternative *safeAs(VT &v) { return variant_safe_as<I>(v); }
  static void emplace(VT &v) { variant_emplace<I>(v); }
};

template <typename VariantType>
struct ManagedVariantAnnotation : TypeAnnotation
{
  static constexpr size_t COUNT = eastl::variant_size_v<VariantType>;

  ManagedVariantAnnotation(const string &n, ModuleLibrary &lib) : TypeAnnotation(n)
  {
    for (auto &valueType : valueTypes)
      valueType = nullptr;
    addValueTypes(lib, eastl::make_index_sequence<COUNT>{});
  }
  virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
  virtual int32_t getGcFlags(das_set<Structure *> &dep, das_set<Annotation *> &depA) const override
  {
    int32_t flags = 0;
    for (const auto &valueType : valueTypes)
      if (valueType)
        flags |= valueType->gcFlags(dep, depA);
    return flags;
  }
  virtual size_t getSizeOf() const override { return sizeof(VariantType); }
  virtual size_t getAlignOf() const override { return alignof(VariantType); }
  virtual bool isRefType() const override { return true; }
  virtual bool canMove() const override { return false; }
  virtual bool canCopy() const override { return false; }

  virtual void walk(DataWalker &walker, void *data) override
  {
    auto variant = reinterpret_cast<VariantType *>(data);
    if (!variant || variant->valueless_by_exception())
      return;
    if (TypeInfo *info = valueInfos[variant->index()])
      eastl::visit([&](auto &value) { walker.walk((char *)&value, info); }, *variant);
  }

  virtual void gc_collect(gc_root *target, gc_root *from) override
  {
    TypeAnnotation::gc_collect(target, from);
    for (auto &valueType : valueTypes)
      if (valueType)
        valueType->gc_collect(target, from);
  }
  virtual void visitTypeDecls(const function<void(TypeDecl *)> &callback) override
  {
    for (auto &valueType : valueTypes)
      if (valueType)
        callback(valueType);
  }

  TypeDeclPtr valueTypes[COUNT];
  TypeInfo *valueInfos[COUNT] = {};
  DebugInfoHelper helpA;

private:
  template <size_t... Is>
  void addValueTypes(ModuleLibrary &lib, eastl::index_sequence<Is...>)
  {
    (addValueType<Is>(lib), ...);
  }
  template <size_t I>
  void addValueType(ModuleLibrary &lib)
  {
    if constexpr (VariantAlternativeOps<VariantType, I>::HAS_VALUE)
    {
      valueTypes[I] = makeType<typename VariantAlternativeOps<VariantType, I>::Alternative>(lib);
      valueTypes[I]->ref = true;
      valueInfos[I] = helpA.makeTypeInfo(nullptr, valueTypes[I]);
    }
  }
};

template <typename VT, size_t I>
void add_variant_alternative_externs(Module &mod, const ModuleLibrary &lib)
{
  using Ops = VariantAlternativeOps<VT, I>;
  const string name = VariantBinding<VT>::alternatives[I];
  static_assert(I < 10);
  const string index(1, char('0' + I));

  addExtern<decltype(&Ops::is), &Ops::is>(mod, lib, ("`is`" + name).c_str(), SideEffects::none, ("variant_is<" + index + ">").c_str())
    ->generated = true;
  addExtern<decltype(&Ops::emplace), &Ops::emplace>(mod, lib, ("emplace_" + name).c_str(), SideEffects::modifyArgument,
    ("variant_emplace<" + index + ">").c_str())
    ->generated = true;
  if constexpr (Ops::HAS_VALUE)
  {
    addExtern<decltype(&Ops::as), &Ops::as, SimNode_ExtFuncCallRef>(mod, lib, ("`as`" + name).c_str(), SideEffects::none,
      ("variant_as<" + index + ">").c_str())
      ->generated = true;
    addExtern<decltype(&Ops::safeAs), &Ops::safeAs>(mod, lib, ("?as`" + name).c_str(), SideEffects::none,
      ("variant_safe_as<" + index + ">").c_str())
      ->generated = true;
  }
}

template <typename VT, size_t... Is>
void add_variant_externs(Module &mod, const ModuleLibrary &lib, eastl::index_sequence<Is...>)
{
  (add_variant_alternative_externs<VT, Is>(mod, lib), ...);
}

template <typename... Ts>
struct typeFactory<eastl::variant<Ts...>>
{
  using VT = eastl::variant<Ts...>;

  static TypeDeclPtr make(const ModuleLibrary &library)
  {
    static_assert(sizeof(VariantBinding<VT>::alternatives) / sizeof(VariantBinding<VT>::alternatives[0]) == sizeof...(Ts),
      "VariantBinding must name every alternative of the variant");
    string declN = typeName<VT>::name();
    if (library.findAnnotation(declN, nullptr).size() == 0)
    {
      auto ann = new ManagedVariantAnnotation<VT>(declN, const_cast<ModuleLibrary &>(library));
      ann->cppName = VariantBinding<VT>::cppName;
      auto mod = library.front();
      mod->addAnnotation(ann);
      add_variant_externs<VT>(*mod, library, eastl::make_index_sequence<sizeof...(Ts)>{});
    }
    return makeHandleType(library, declN.c_str());
  }
};

} // namespace das
