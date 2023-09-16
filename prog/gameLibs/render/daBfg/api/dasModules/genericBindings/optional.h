#pragma once

#include <EASTL/optional.h>
#include <dasModules/dasModulesCommon.h>
#include <dag/dag_relocatable.h>


namespace das
{

template <typename TT>
struct typeName<eastl::optional<TT>>
{
  static string name() { return string("optional`") + typeName<TT>::name(); }
};

template <typename OptionalType>
struct ManagedOptionalAnnotation : TypeAnnotation
{
  using OT = typename OptionalType::value_type;

  ManagedOptionalAnnotation(const string &n, ModuleLibrary &lib) : TypeAnnotation(n)
  {
    valueType = makeType<OT>(lib);
    valueType->ref = true;
    ati = helpA.makeTypeInfo(nullptr, valueType);
  }
  virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
  virtual int32_t getGcFlags(das_set<Structure *> &dep, das_set<Annotation *> &depA) const override
  {
    return valueType->gcFlags(dep, depA);
  }
  virtual size_t getSizeOf() const override { return sizeof(OptionalType); }
  virtual size_t getAlignOf() const override { return alignof(OptionalType); }
  virtual bool isRefType() const override { return true; }
  virtual bool canMove() const override { return valueType->canMove(); }
  virtual bool canCopy() const override { return valueType->canCopy(); }

  virtual void walk(DataWalker &walker, void *opt) override
  {
    auto pOpt = reinterpret_cast<OptionalType *>(opt);
    if (pOpt)
      walker.walk((char *)&*pOpt, ati);
  }

  TypeDeclPtr valueType;
  DebugInfoHelper helpA;
  TypeInfo *ati = nullptr;
};

template <typename TT>
bool optional_is_some(const TT &opt)
{
  return !!opt;
}
template <typename TT>
bool optional_is_none(const TT &opt)
{
  return !opt;
}
template <typename TT>
typename TT::value_type &optional_as_some(TT &opt)
{
  G_ASSERT(!!opt);
  return *opt;
}
template <typename TT>
void optional_emplace_some(TT &val)
{
  val.emplace();
}
template <typename TT>
void optional_emplace_none(TT &val)
{
  val = {};
}

template <typename TT>
void optional_clone(TT &opt, const TT &other)
{
  opt = other;
}

template <typename OT>
struct typeFactory<eastl::optional<OT>>
{
  using TT = eastl::optional<OT>;

  static TypeDeclPtr make(const ModuleLibrary &library)
  {
    string declN = typeName<TT>::name();
    if (library.findAnnotation(declN, nullptr).size() == 0)
    {
      auto declT = makeType<OT>(library);
      auto ann = make_smart<ManagedOptionalAnnotation<TT>>(declN, const_cast<ModuleLibrary &>(library));
      ann->cppName = "eastl::optional<" + describeCppType(declT) + ">";
      auto mod = library.front();
      mod->addAnnotation(ann);

      addExtern<DAS_BIND_FUN(optional_is_some<TT>)>(*mod, library, //
        "`is`some", SideEffects::none, "optional_is_some")
        ->generated = true;
      addExtern<DAS_BIND_FUN(optional_is_none<TT>)>(*mod, library, //
        "`is`none", SideEffects::none, "optional_is_none")
        ->generated = true;
      addExtern<DAS_BIND_FUN(optional_as_some<TT>), SimNode_ExtFuncCallRef>(*mod, library, //
        "`as`some", SideEffects::none, "optional_as_some")
        ->generated = true;

      addExtern<DAS_BIND_FUN(optional_emplace_some<TT>)>(*mod, library, //
        "emplace_some", SideEffects::modifyArgument, "optional_emplace_some")
        ->generated = true;
      addExtern<DAS_BIND_FUN(optional_emplace_none<TT>)>(*mod, library, //
        "emplace_none", SideEffects::modifyArgument, "optional_emplace_none")
        ->generated = true;

      addExtern<DAS_BIND_FUN(optional_clone<TT>)>(*mod, library, "clone", SideEffects::modifyArgument, "optional_clone")->generated =
        true;
    }
    return makeHandleType(library, declN.c_str());
  }
};

} // namespace das