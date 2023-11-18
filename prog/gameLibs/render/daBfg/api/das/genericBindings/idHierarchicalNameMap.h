#pragma once

#include <id/idHierarchicalNameMap.h>
#include <dasModules/dasModulesCommon.h>
#include <dag/dag_relocatable.h>


namespace das
{

template <class... Ts>
struct typeName<IdHierarchicalNameMap<Ts...>>
{
  static string name()
  {
    auto result = string("idNameMap");
    ((result += "`", result += typeName<Ts>::name()), ...);
    return result;
  }
};

template <class... Ts>
auto id_hierarchical_name_map_root(const IdHierarchicalNameMap<Ts...> &map)
{
  return map.root();
}

template <class T, class... Ts>
auto id_hierarchical_name_map_get_parent(const IdHierarchicalNameMap<Ts...> &map, T id)
{
  return map.template getParent<T>(id);
}

template <class T, class... Ts>
T id_hierarchical_name_map_add_name_id(IdHierarchicalNameMap<Ts...> &map, typename ::detail::FirstOf<Ts...>::Type folder,
  const char *name)
{
  return map.template addNameId<T>(folder, name);
}

template <class T, class... Ts>
T id_hierarchical_name_map_get_name_id(IdHierarchicalNameMap<Ts...> &map, typename ::detail::FirstOf<Ts...>::Type folder,
  const char *name)
{
  return map.template getNameId<T>(folder, name);
}

template <class T, class... Ts>
const char *id_hierarchical_name_map_get_name(const IdHierarchicalNameMap<Ts...> &map, T id)
{
  return map.getName(id);
}

template <class T, class... Ts>
const char *id_hierarchical_name_map_get_short_name(const IdHierarchicalNameMap<Ts...> &map, T id)
{
  return map.getShortName(id);
}

template <class... Ts>
struct IdNameMapAnnotation final : das::TypeAnnotation
{
  using MapType = IdHierarchicalNameMap<Ts...>;

  IdNameMapAnnotation(const das::string &n) : TypeAnnotation(n) {}
  virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
  virtual size_t getSizeOf() const override { return sizeof(MapType); }
  virtual size_t getAlignOf() const override { return alignof(MapType); }
  virtual bool isRefType() const override { return true; }
};

template <class... Ts>
struct typeFactory<IdHierarchicalNameMap<Ts...>>
{
  using VT = IdHierarchicalNameMap<Ts...>;

  static ___noinline TypeDeclPtr make(const ModuleLibrary &library)
  {
    das::string declN = typeName<VT>::name();
    if (library.findAnnotation(declN, nullptr).size() == 0)
    {
      auto ann = make_smart<IdNameMapAnnotation<Ts...>>(declN);

      const string commaSeparatedCppParameters = ((", " + describeCppType(makeType<Ts>(library))) + ...).substr(2, string::npos);

      ann->cppName = "IdHierarchicalNameMap<" + commaSeparatedCppParameters + ">";
      auto mod = library.front();
      mod->addAnnotation(ann);

      addExtern<DAS_BIND_FUN((id_hierarchical_name_map_root<Ts...>))>(*mod, library, //
        "root", SideEffects::none, "id_hierarchical_name_map_root")
        ->generated = true;

      // We want to bind these function for every single T in Ts, so we
      // use a lambda + operator comma + fold expression trick to iterate
      // through Ts at compile time.
      (
        [&mod, &library, &commaSeparatedCppParameters]() {
          const auto &name = typeName<Ts>::name();
          const auto cppName = describeCppType(makeType<Ts>(library));
          addExtern<DAS_BIND_FUN((id_hierarchical_name_map_get_parent<Ts, Ts...>))>(*mod, library, //
            "getParent", SideEffects::none, "id_hierarchical_name_map_get_parent")
            ->generated = true;
          addExtern<DAS_BIND_FUN((id_hierarchical_name_map_add_name_id<Ts, Ts...>))>(*mod, library, //
            (string("addNameId`") + name).c_str(), SideEffects::modifyArgument,
            ("id_hierarchical_name_map_add_name_id<" + cppName + ">").c_str())
            ->generated = true;
          addExtern<DAS_BIND_FUN((id_hierarchical_name_map_get_name_id<Ts, Ts...>))>(*mod, library, //
            (string("getNameId`") + name).c_str(), SideEffects::none,
            ("id_hierarchical_name_map_get_name_id<" + cppName + ">").c_str())
            ->generated = true;
          addExtern<DAS_BIND_FUN((id_hierarchical_name_map_get_name<Ts, Ts...>))>(*mod, library, //
            "getName", SideEffects::none, "id_hierarchical_name_map_get_name")
            ->generated = true;
          addExtern<DAS_BIND_FUN((id_hierarchical_name_map_get_short_name<Ts, Ts...>))>(*mod, library, //
            "getShortName", SideEffects::none, "id_hierarchical_name_map_get_short_name")
            ->generated = true;
        }(),
        ...);
    }
    return makeHandleType(library, declN.c_str());
  }
};
} // namespace das
