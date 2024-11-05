// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/aotGameMath.h>
#include <dasModules/aotEcs.h>


namespace bind_dascript
{

class GameMathModule final : public das::Module
{
public:
  GameMathModule() : das::Module("GameMath")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(pack_unit_vec_uint)>(*this, lib, "pack_unit_vec_uint", das::SideEffects::none,
      "bind_dascript::pack_unit_vec_uint");
    das::addExtern<DAS_BIND_FUN(unpack_unit_vec_uint)>(*this, lib, "unpack_unit_vec_uint", das::SideEffects::none,
      "bind_dascript::unpack_unit_vec_uint");
    das::addExtern<DAS_BIND_FUN(gamemath::construct_convex_from_frustum)>(*this, lib, "construct_convex_from_frustum",
      das::SideEffects::modifyArgument, "gamemath::construct_convex_from_frustum");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotGameMath.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(GameMathModule, bind_dascript);
