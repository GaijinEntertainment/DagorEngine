#pragma once

#include <generic/dag_fixedVectorSet.h>
#include <dasModules/dasModulesCommon.h>
#include <dag/dag_relocatable.h>


namespace das
{

template <class TT, size_t inplace_count>
struct typeName<dag::FixedVectorSet<TT, inplace_count>>
{
  static string name() { return string("fixedVectorSet`") + typeName<TT>::name() + "`" + to_string(inplace_count); }
};

template <class VT>
void fixed_vector_set_insert(VT &set, const typename VT::value_type &value)
{
  set.insert(value);
}

template <class TT, size_t inplace_count>
struct typeFactory<dag::FixedVectorSet<TT, inplace_count>>
{
  using VT = dag::FixedVectorSet<TT, inplace_count>;

  static ___noinline TypeDeclPtr make(const ModuleLibrary &library)
  {
    das::string declN = typeName<VT>::name();
    if (library.findAnnotation(declN, nullptr).size() == 0)
    {
      auto declT = makeType<TT>(library);
      auto ann = make_smart<ManagedVectorAnnotation<VT>>(declN, const_cast<ModuleLibrary &>(library));
      ann->cppName = "dag::FixedVectorSet<" + describeCppType(declT) + ", " + to_string(inplace_count) + ">";
      auto mod = library.front();
      mod->addAnnotation(ann);

      addExtern<DAS_BIND_FUN(fixed_vector_set_insert<VT>)>(*mod, library, //
        "insert", SideEffects::modifyArgument, "fixed_vector_set_insert")
        ->generated = true;
    }
    return makeHandleType(library, declN.c_str());
  }
};
} // namespace das
