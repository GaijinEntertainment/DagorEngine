#pragma once

#include <id/idNameMap.h>
#include <dasModules/dasModulesCommon.h>
#include <dag/dag_relocatable.h>


namespace das
{

template <class T>
struct typeName<IdNameMap<T>>
{
  static string name() { return string("idNameMap`") + typeName<T>::name(); }
};

template <class T>
T id_name_map_add_name_id(IdNameMap<T> &map, const char *name)
{
  return map.addNameId(name);
}

template <class T>
T id_name_map_get_name_id(IdNameMap<T> &map, const char *name)
{
  return map.getNameId(name);
}

template <class T>
const char *id_name_map_get_name(const IdNameMap<T> &map, T id)
{
  return map.getNameCstr(id);
}

template <class T>
struct IdNameMapAnnotation final : das::TypeAnnotation
{
  using MapType = IdNameMap<T>;

  IdNameMapAnnotation(const das::string &n) : TypeAnnotation(n) {}
  virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
  virtual size_t getSizeOf() const override { return sizeof(MapType); }
  virtual size_t getAlignOf() const override { return alignof(MapType); }
  virtual bool isRefType() const override { return true; }
};

template <class T>
struct typeFactory<IdNameMap<T>>
{
  using VT = IdNameMap<T>;

  static ___noinline TypeDeclPtr make(const ModuleLibrary &library)
  {
    das::string declN = typeName<VT>::name();
    if (library.findAnnotation(declN, nullptr).size() == 0)
    {
      auto declT = makeType<T>(library);
      auto ann = make_smart<IdNameMapAnnotation<T>>(declN);
      ann->cppName = "IdNameMap<" + describeCppType(declT) + ">";
      auto mod = library.front();
      mod->addAnnotation(ann);

      addExtern<DAS_BIND_FUN(id_name_map_add_name_id<T>)>(*mod, library, //
        "addNameId", SideEffects::modifyArgument, "id_name_map_add_name_id")
        ->generated = true;
      addExtern<DAS_BIND_FUN(id_name_map_get_name_id<T>)>(*mod, library, //
        "getNameId", SideEffects::none, "id_name_map_get_name_id")
        ->generated = true;
      addExtern<DAS_BIND_FUN(id_name_map_get_name<T>)>(*mod, library, //
        "getName", SideEffects::none, "id_name_map_get_name")
        ->generated = true;
    }
    return makeHandleType(library, declN.c_str());
  }
};
} // namespace das
