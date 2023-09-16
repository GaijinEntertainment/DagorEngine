#include <dasModules/aotRiDestr.h>


struct DestructableObjectAnnotation : das::ManagedStructureAnnotation<gamephys::DestructableObject, false>
{
  DestructableObjectAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DestructableObject", ml)
  {
    cppName = " ::gamephys::DestructableObject";
  }
};

namespace bind_dascript
{
class RiDestrModule final : public das::Module
{
public:
  RiDestrModule() : das::Module("RiDestr")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("Dacoll"));
    addAnnotation(das::make_smart<DestructableObjectAnnotation>(lib));
    das::addExtern<DAS_BIND_FUN(rendinstdestr::apply_damage_to_riextra)>(*this, lib, "apply_damage_to_riextra",
      das::SideEffects::modifyExternal, "rendinstdestr::apply_damage_to_riextra");
    das::addExtern<DAS_BIND_FUN(rendinstdestr::remove_ri_without_collision_in_radius)>(*this, lib,
      "remove_ri_without_collision_in_radius", das::SideEffects::modifyExternal,
      "rendinstdestr::remove_ri_without_collision_in_radius");
    das::addExtern<DAS_BIND_FUN(destroyRendinstSimple)>(*this, lib, "destroyRendinst", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::destroyRendinstSimple");
    das::addExtern<DAS_BIND_FUN(destroyRendinst)>(*this, lib, "destroyRendinst", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::destroyRendinst");
    das::addExtern<DAS_BIND_FUN(apply_impulse_to_riextra)>(*this, lib, "apply_impulse_to_riextra", das::SideEffects::modifyExternal,
      "bind_dascript::apply_impulse_to_riextra");

    das::addExtern<DAS_BIND_FUN(get_destructable_objects)>(*this, lib, "get_destructable_objects", das::SideEffects::modifyExternal,
      "bind_dascript::get_destructable_objects");
    das::addExtern<DAS_BIND_FUN(destructable_object_get_phys_sys_instance)>(*this, lib, "get_phys_sys_instance",
      das::SideEffects::accessExternal, "bind_dascript::destructable_object_get_phys_sys_instance");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotRiDestr.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(RiDestrModule, bind_dascript);
