#include <dasModules/aotDagorResources.h>


namespace bind_dascript
{

class DagorResources final : public das::Module
{
public:
  DagorResources() : das::Module("DagorResources")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_game_resource_name)>(*this, lib, "get_game_resource_name",
      das::SideEffects::accessExternal, "bind_dascript::get_game_resource_name");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorResources.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorResources, bind_dascript);