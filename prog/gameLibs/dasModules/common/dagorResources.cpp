// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotDagorResources.h>


namespace bind_dascript
{

struct GameResourceFactoryAnnotation : das::ManagedStructureAnnotation<GameResourceFactory, false>
{
  GameResourceFactoryAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("GameResourceFactory", ml)
  {
    cppName = " ::GameResourceFactory";
  }
};

class DagorResources final : public das::Module
{
public:
  DagorResources() : das::Module("DagorResources")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));

    addAnnotation(das::make_smart<GameResourceFactoryAnnotation>(lib));

    das::addConstant(*this, "DynModelGameResClassId", DynModelGameResClassId);
    das::addConstant(*this, "RendInstGameResClassId", RendInstGameResClassId);
    das::addConstant(*this, "ImpostorDataGameResClassId", ImpostorDataGameResClassId);
    das::addConstant(*this, "RndGrassGameResClassId", RndGrassGameResClassId);
    das::addConstant(*this, "GeomNodeTreeGameResClassId", GeomNodeTreeGameResClassId);
    das::addConstant(*this, "CharacterGameResClassId", CharacterGameResClassId);
    das::addConstant(*this, "FastPhysDataGameResClassId", FastPhysDataGameResClassId);
    das::addConstant(*this, "Anim2DataGameResClassId", Anim2DataGameResClassId);
    das::addConstant(*this, "PhysSysGameResClassId", PhysSysGameResClassId);
    das::addConstant(*this, "PhysObjGameResClassId", PhysObjGameResClassId);
    das::addConstant(*this, "CollisionGameResClassId", CollisionGameResClassId);
    das::addConstant(*this, "RagdollGameResClassId", RagdollGameResClassId);
    das::addConstant(*this, "EffectGameResClassId", EffectGameResClassId);
    das::addConstant(*this, "IslLightGameResClassId", IslLightGameResClassId);
    das::addConstant(*this, "CloudGameResClassId", CloudGameResClassId);
    das::addConstant(*this, "MaterialGameResClassId", MaterialGameResClassId);
    das::addConstant(*this, "LShaderGameResClassId", LShaderGameResClassId);
    das::addConstant(*this, "AnimBnlGameResClassId", AnimBnlGameResClassId);
    das::addConstant(*this, "AnimGraphGameResClassId", AnimGraphGameResClassId);

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_game_resource_name)>(*this, lib, "get_game_resource_name",
      das::SideEffects::accessExternal, "bind_dascript::get_game_resource_name");
    das::addExtern<DAS_BIND_FUN(find_gameres_factory)>(*this, lib, "find_gameres_factory", das::SideEffects::accessExternal,
      "find_gameres_factory");
    das::addExtern<DAS_BIND_FUN(bind_dascript::iterate_gameres_factory_resources)>(*this, lib, "iterate_gameres_factory_resources",
      das::SideEffects::accessExternal, "bind_dascript::iterate_gameres_factory_resources");

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