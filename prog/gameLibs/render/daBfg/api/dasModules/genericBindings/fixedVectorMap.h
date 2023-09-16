#pragma once

#include <generic/dag_fixedVectorMap.h>
#include <dasModules/dasModulesCommon.h>
#include <dag/dag_relocatable.h>


namespace das
{

template <class K, class V, size_t inplace_count>
struct typeName<dag::FixedVectorMap<K, V, inplace_count>>
{
  static string name()
  {
    return string("fixedVectorMap`") + typeName<K>::name() + "`" + typeName<V>::name() + "`" + to_string(inplace_count);
  }
};

template <class K, class V, size_t inplace_count>
V &fixed_vector_map_at(dag::FixedVectorMap<K, V, inplace_count> &map, const K &key)
{
  return map[key];
}

template <class K, class V, size_t inplace_count>
bool fixed_vector_map_contains(const dag::FixedVectorMap<K, V, inplace_count> &map, const K &key)
{
  return map.contains(key);
}

template <class K, class V, size_t inplace_count>
struct typeFactory<dag::FixedVectorMap<K, V, inplace_count>>
{
  using VT = dag::FixedVectorMap<K, V, inplace_count>;

  static ___noinline TypeDeclPtr make(const ModuleLibrary &library)
  {
    das::string declN = typeName<VT>::name();
    if (library.findAnnotation(declN, nullptr).size() == 0)
    {
      auto keyDeclT = makeType<K>(library);
      auto valDeclT = makeType<V>(library);
      auto ann = make_smart<ManagedVectorAnnotation<VT>>(declN, const_cast<ModuleLibrary &>(library));
      ann->cppName =
        "dag::FixedVectorMap<" + describeCppType(keyDeclT) + ", " + describeCppType(valDeclT) + ", " + to_string(inplace_count) + ">";
      auto mod = library.front();
      mod->addAnnotation(ann);

      addExtern<DAS_BIND_FUN((fixed_vector_map_at<K, V, inplace_count>)), SimNode_ExtFuncCallRef>(*mod, library, //
        "[]", SideEffects::modifyArgument, "fixed_vector_map_at")
        ->generated = true;
      addExtern<DAS_BIND_FUN((fixed_vector_map_contains<K, V, inplace_count>))>(*mod, library, //
        "key_exists", SideEffects::none, "fixed_vector_map_contains")
        ->generated = true;
    }
    return makeHandleType(library, declN.c_str());
  }
};
} // namespace das
