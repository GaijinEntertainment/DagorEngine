// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasModulesCommon.h>
#include <math/random/dag_random.h>
namespace bind_dascript
{

class DagorRandom final : public das::Module
{
public:
  DagorRandom() : das::Module("DagorRandom")
  {
    das::ModuleLibrary lib(this);
    das::addConstant(*this, "DAGOR_RAND_MAX", DAGOR_RAND_MAX);
    das::addExtern<DAS_BIND_FUN(grnd)>(*this, lib, "grnd", das::SideEffects::modifyExternal, "grnd");
    das::addExtern<DAS_BIND_FUN(gsrnd)>(*this, lib, "gsrnd", das::SideEffects::modifyExternal, "gsrnd");
    das::addExtern<DAS_BIND_FUN(gfrnd)>(*this, lib, "gfrnd", das::SideEffects::modifyExternal, "gfrnd");
    das::addExtern<DAS_BIND_FUN(rnd_float)>(*this, lib, "rnd_float", das::SideEffects::modifyExternal, "rnd_float");
    das::addExtern<DAS_BIND_FUN(rnd_int)>(*this, lib, "rnd_int", das::SideEffects::modifyExternal, "rnd_int");
    das::addExtern<DAS_BIND_FUN(_rnd_float)>(*this, lib, "_rnd_float", das::SideEffects::modifyArgument, "_rnd_float");
    das::addExtern<DAS_BIND_FUN(_rnd_int)>(*this, lib, "_rnd_int", das::SideEffects::modifyArgument, "_rnd_int");
    das::addExtern<DAS_BIND_FUN(_rnd_range)>(*this, lib, "_rnd_range", das::SideEffects::modifyArgument, "_rnd_range");
    das::addExtern<DAS_BIND_FUN(gauss_rnd)>(*this, lib, "gauss_rnd", das::SideEffects::modifyExternal, "gauss_rnd");
    das::addExtern<DAS_BIND_FUN(set_rnd_seed)>(*this, lib, "set_rnd_seed", das::SideEffects::modifyExternal, "set_rnd_seed");
    das::addExtern<DAS_BIND_FUN(get_rnd_seed)>(*this, lib, "get_rnd_seed", das::SideEffects::accessExternal, "get_rnd_seed");

    das::addExtern<DAS_BIND_FUN(_gauss_rnd)>(*this, lib, "_gauss_rnd", das::SideEffects::modifyArgument, "_gauss_rnd");
    das::addExtern<DAS_BIND_FUN(_rnd)>(*this, lib, "_rnd", das::SideEffects::modifyArgument, "_rnd");
    das::addExtern<DAS_BIND_FUN(_frnd)>(*this, lib, "_frnd", das::SideEffects::modifyArgument, "_frnd");
    das::addExtern<DAS_BIND_FUN(_srnd)>(*this, lib, "_srnd", das::SideEffects::modifyArgument, "_srnd");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <math/random/dag_random.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DagorRandom, bind_dascript);