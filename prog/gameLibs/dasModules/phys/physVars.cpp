#include <dasModules/aotPhysVars.h>

struct PhysVarsAnnotation : das::ManagedStructureAnnotation<PhysVars, false>
{
  PhysVarsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysVars", ml) { cppName = " ::PhysVars"; }
};

namespace bind_dascript
{
class PhysVarsModule final : public das::Module
{
public:
  PhysVarsModule() : das::Module("PhysVars")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addAnnotation(das::make_smart<PhysVarsAnnotation>(lib));
    das::addExtern<DAS_BIND_FUN(physvars_get_var_id)>(*this, lib, "getVarId", das::SideEffects::accessExternal,
      "bind_dascript::physvars_get_var_id");
    das::addExtern<DAS_BIND_FUN(physvars_set_var)>(*this, lib, "setVar", das::SideEffects::modifyArgument,
      "bind_dascript::physvars_set_var");
    das::addExtern<DAS_BIND_FUN(physvars_set_var_val)>(*this, lib, "setVar", das::SideEffects::modifyArgument,
      "bind_dascript::physvars_set_var_val");
    das::addExtern<DAS_BIND_FUN(physvars_get_var)>(*this, lib, "getVar", das::SideEffects::none, "bind_dascript::physvars_get_var");
    das::addExtern<DAS_BIND_FUN(physvars_register_var)>(*this, lib, "registerVar", das::SideEffects::modifyArgument,
      "bind_dascript::physvars_register_var");
    das::addExtern<DAS_BIND_FUN(physvars_register_pull_var)>(*this, lib, "registerPullVar", das::SideEffects::modifyArgument,
      "bind_dascript::physvars_register_pull_var");
    das::addExtern<DAS_BIND_FUN(physvars_is_var_pullable)>(*this, lib, "isVarPullable", das::SideEffects::accessExternal,
      "bind_dascript::physvars_is_var_pullable");
    das::addExtern<DAS_BIND_FUN(physvars_get_vars_count)>(*this, lib, "getVarsCount", das::SideEffects::accessExternal,
      "bind_dascript::physvars_get_vars_count");
    das::addExtern<DAS_BIND_FUN(physvars_get_var_name)>(*this, lib, "getVarName", das::SideEffects::accessExternal,
      "bind_dascript::physvars_get_var_name");
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotPhysVars.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(PhysVarsModule, bind_dascript);
