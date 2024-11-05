// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotPhysMat.h>
#include <daScript/ast/ast_handle.h>

struct MaterialDataAnnotation : das::ManagedStructureAnnotation<PhysMat::MaterialData, false>
{
  MaterialDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("MaterialData", ml)
  {
    cppName = " ::PhysMat::MaterialData";
    addField<DAS_BIND_MANAGED_FIELD(id)>("id");
  }
};

struct PhysMatDamageModelPropsAnnotation : das::ManagedStructureAnnotation<PhysMatDamageModelProps, false>
{
  PhysMatDamageModelPropsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysMatDamageModelProps", ml)
  {
    cppName = " ::PhysMatDamageModelProps";
    addField<DAS_BIND_MANAGED_FIELD(armorThickness)>("armorThickness");
    addField<DAS_BIND_MANAGED_FIELD(ricochetAngleMult)>("ricochetAngleMult");
    addField<DAS_BIND_MANAGED_FIELD(bulletBrokenThreshold)>("bulletBrokenThreshold");
  }
};

struct DeformMatPropsAnnotation : das::ManagedStructureAnnotation<physmat::DeformMatProps, false>
{
  DeformMatPropsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DeformMatProps", ml)
  {
    cppName = " ::physmat::DeformMatProps";
    addField<DAS_BIND_MANAGED_FIELD(coverNoiseMult)>("coverNoiseMult");
    addField<DAS_BIND_MANAGED_FIELD(coverNoiseAdd)>("coverNoiseAdd");
    addField<DAS_BIND_MANAGED_FIELD(period)>("period");
    addField<DAS_BIND_MANAGED_FIELD(mult)>("mult");
  }
};

namespace bind_dascript
{
class PhysMatModule final : public das::Module
{
public:
  PhysMatModule() : das::Module("PhysMat")
  {
    das::ModuleLibrary lib(this);
    addAnnotation(das::make_smart<MaterialDataAnnotation>(lib));
    addAnnotation(das::make_smart<PhysMatDamageModelPropsAnnotation>(lib));
    addAnnotation(das::make_smart<DeformMatPropsAnnotation>(lib));

    G_STATIC_ASSERT(sizeof(PhysMat::MatID) == sizeof(int32_t));
    auto pType = das::make_smart<das::TypeDecl>(das::Type::tInt);
    pType->alias = "MatID";
    addAlias(pType);

    das::addExtern<DAS_BIND_FUN(PhysMat::physMatCount)>(*this, lib, "physMatCount", das::SideEffects::accessExternal,
      "PhysMat::physMatCount");
    das::addExtern<DAS_BIND_FUN(PhysMat::getMaterialId)>(*this, lib, "get_material_id", das::SideEffects::accessExternal,
      "PhysMat::getMaterialId");
    das::addExtern<DAS_BIND_FUN(get_material_name)>(*this, lib, "get_material_name", das::SideEffects::none,
      "bind_dascript::get_material_name");
    das::addExtern<const PhysMat::MaterialData &(*)(int), PhysMat::getMaterial, das::SimNode_ExtFuncCallRef>(*this, lib,
      "get_material", das::SideEffects::accessExternal, "PhysMat::getMaterial");

    das::addExtern<DAS_BIND_FUN(bind_dascript::das_get_props<PhysMatDamageModelProps>)>(*this, lib, "phys_mat_damage_model_get_props",
      das::SideEffects::accessExternal, "bind_dascript::das_get_props<PhysMatDamageModelProps>");

    das::addExtern<DAS_BIND_FUN(bind_dascript::das_get_props<physmat::DeformMatProps>)>(*this, lib, "phys_mat_deform_mat_get_props",
      das::SideEffects::accessExternal, "bind_dascript::das_get_props<physmat::DeformMatProps>");

    das::addConstant<int32_t>(*this, "PHYSMAT_DEFAULT", PHYSMAT_DEFAULT);
    das::addConstant<int32_t>(*this, "PHYSMAT_INVALID", PHYSMAT_INVALID);

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotPhysMat.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(PhysMatModule, bind_dascript);
