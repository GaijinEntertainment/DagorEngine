#pragma once

#include <id/idIndexedMapping.h>
#include <dasModules/dasModulesCommon.h>
#include <dag/dag_relocatable.h>


namespace das
{

template <class K, class V>
struct typeName<IdIndexedMapping<K, V>>
{
  static string name() { return string("idIndexedMapping`") + typeName<K>::name() + "`" + typeName<V>::name(); }
};

template <class K, class V>
V &id_indexed_mapping_at(IdIndexedMapping<K, V> &map, K key)
{
  return map[key];
}

template <class K, class V>
V &id_indexed_mapping_get(IdIndexedMapping<K, V> &map, K key)
{
  return map.get(key);
}

template <class K, class V>
struct typeFactory<IdIndexedMapping<K, V>>
{
  using VT = IdIndexedMapping<K, V>;

  static ___noinline TypeDeclPtr make(const ModuleLibrary &library)
  {
    das::string declN = typeName<VT>::name();
    if (library.findAnnotation(declN, nullptr).size() == 0)
    {
      auto keyDeclT = makeType<K>(library);
      auto valDeclT = makeType<V>(library);
      auto ann = make_smart<ManagedVectorAnnotation<VT>>(declN, const_cast<ModuleLibrary &>(library));
      ann->cppName = "IdIndexedMapping<" + describeCppType(keyDeclT) + ", " + describeCppType(valDeclT) + ">";
      auto mod = library.front();
      mod->addAnnotation(ann);

      addExtern<DAS_BIND_FUN((id_indexed_mapping_get<K, V>)), SimNode_ExtFuncCallRef>(*mod, library, //
        "[]", SideEffects::modifyArgument, "id_indexed_mapping_get")
        ->generated = true;
      addExtern<DAS_BIND_FUN((id_indexed_mapping_get<K, V>))>(*mod, library, //
        "get", SideEffects::modifyArgument, "id_indexed_mapping_get")
        ->generated = true;
    }
    return makeHandleType(library, declN.c_str());
  }
};
} // namespace das
