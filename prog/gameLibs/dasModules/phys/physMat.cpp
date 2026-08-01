// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotPhysMat.h>
#include <daScript/ast/ast_handle.h>

struct MaterialDataAnnotation : das::ManagedStructureAnnotation<PhysMat::MaterialData, false>
{
  MaterialDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("MaterialData", ml)
  {
    cppName = " ::PhysMat::MaterialData";
    addField<DAS_BIND_MANAGED_FIELD(id)>("id");
    addField<DAS_BIND_MANAGED_FIELD(imp_absorb_k)>("imp_absorb_k");
    addField<DAS_BIND_MANAGED_FIELD(imp_weak_k)>("imp_weak_k");
    addField<DAS_BIND_MANAGED_FIELD(r_bounce_k)>("r_bounce_k");
    addField<DAS_BIND_MANAGED_FIELD(shake_factor)>("shake_factor");
    addField<DAS_BIND_MANAGED_FIELD(mk_dmg)>("mk_dmg");
    addField<DAS_BIND_MANAGED_FIELD(dont_trace)>("dont_trace");
    addField<DAS_BIND_MANAGED_FIELD(clippable)>("clippable");
    addField<DAS_BIND_MANAGED_FIELD(autoReset)>("autoReset");
    addField<DAS_BIND_MANAGED_FIELD(disable_control)>("disable_control");
    addField<DAS_BIND_MANAGED_FIELD(invisible_clipping)>("invisible_clipping");
    addField<DAS_BIND_MANAGED_FIELD(phobj_only)>("phobj_only");
    addField<DAS_BIND_MANAGED_FIELD(damage_k)>("damage_k");
    addField<DAS_BIND_MANAGED_FIELD(deformableWidth)>("deformableWidth");
    addField<DAS_BIND_MANAGED_FIELD(resistanceK)>("resistanceK");
    addField<DAS_BIND_MANAGED_FIELD(completelyTransparent)>("completelyTransparent");
    addField<DAS_BIND_MANAGED_FIELD(lightTransparent)>("lightTransparent");
    addField<DAS_BIND_MANAGED_FIELD(noTransparentThickness)>("noTransparentThickness");
    addField<DAS_BIND_MANAGED_FIELD(fly_through_clip)>("fly_through_clip");
    addField<DAS_BIND_MANAGED_FIELD(stick_k)>("stick_k");
    addField<DAS_BIND_MANAGED_FIELD(lifeTime)>("lifeTime");
    addField<DAS_BIND_MANAGED_FIELD(physBodyMaterial)>("physBodyMaterial");
    addField<DAS_BIND_MANAGED_FIELD(physStaticFriction)>("physStaticFriction");
    addField<DAS_BIND_MANAGED_FIELD(physRestitution)>("physRestitution");
    addField<DAS_BIND_MANAGED_FIELD(camera_collision)>("camera_collision");
    addField<DAS_BIND_MANAGED_FIELD(physics_collision)>("physics_collision");
    addField<DAS_BIND_MANAGED_FIELD(bullets_collision)>("bullets_collision");
    addField<DAS_BIND_MANAGED_FIELD(characters_collision)>("characters_collision");
    addField<DAS_BIND_MANAGED_FIELD(characters_collision2)>("characters_collision2");
    addField<DAS_BIND_MANAGED_FIELD(characters_collision3)>("characters_collision3");
    addField<DAS_BIND_MANAGED_FIELD(directocclusion)>("directocclusion");
    addField<DAS_BIND_MANAGED_FIELD(reverbocclusion)>("reverbocclusion");
    addFieldEx("soundMaterial", "soundMaterial", offsetof(PhysMat::MaterialData, soundMaterial), das::makeType<char *>(ml));
    addField<DAS_BIND_MANAGED_FIELD(isSolid)>("isSolid");
    addField<DAS_BIND_MANAGED_FIELD(tankTracksTexId)>("tankTracksTexId");
    addField<DAS_BIND_MANAGED_FIELD(vehicleHeightmapDeformation)>("vehicleHeightmapDeformation");
    addField<DAS_BIND_MANAGED_FIELD(humanHeightmapDeformation)>("humanHeightmapDeformation");
    addField<DAS_BIND_MANAGED_FIELD(trailDetailStrength)>("trailDetailStrength");
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
    addAnnotation(new MaterialDataAnnotation(lib));
    addAnnotation(new PhysMatDamageModelPropsAnnotation(lib));
    addAnnotation(new DeformMatPropsAnnotation(lib));

    G_STATIC_ASSERT(sizeof(PhysMat::MatID) == sizeof(int32_t));
    auto pType = new das::TypeDecl(das::Type::tInt);
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
