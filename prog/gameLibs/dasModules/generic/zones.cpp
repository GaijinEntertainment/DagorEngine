#include <dasModules/aotZones.h>

namespace bind_dascript
{
static char zones_das[] =
#include "zones.das.inl"
  ;

class ZonesModule final : public das::Module
{
public:
  ZonesModule() : das::Module("zones")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    das::addExtern<DAS_BIND_FUN(game::is_point_in_capzone)>(*this, lib, "is_point_in_capzone", das::SideEffects::accessExternal,
      "game::is_point_in_capzone");
    das::addExtern<DAS_BIND_FUN(get_active_capzones_on_pos)>(*this, lib, "get_active_capzones_on_pos",
      das::SideEffects::accessExternal, "bind_dascript::get_active_capzones_on_pos");
    das::addExtern<DAS_BIND_FUN(is_point_in_polygon_zone)>(*this, lib, "is_point_in_polygon_zone", das::SideEffects::none,
      "bind_dascript::is_point_in_polygon_zone");
    compileBuiltinModule("zones.das", (unsigned char *)zones_das, sizeof(zones_das));
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotZones.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(ZonesModule, bind_dascript);
