#include <dasModules/aotRendInst.h>
#include <ecs/phys/collRes.h>


struct RendInstDescAnnotation final : das::ManagedStructureAnnotation<rendinst::RendInstDesc, false>
{
  RendInstDescAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("RendInstDesc", ml)
  {
    cppName = " ::rendinst::RendInstDesc";
    addField<DAS_BIND_MANAGED_FIELD(cellIdx)>("cellIdx");
    addField<DAS_BIND_MANAGED_FIELD(idx)>("idx");
    addField<DAS_BIND_MANAGED_FIELD(pool)>("pool");
    addField<DAS_BIND_MANAGED_FIELD(offs)>("offs");
    addField<DAS_BIND_MANAGED_FIELD(layer)>("layer");
    addProperty<DAS_BIND_MANAGED_PROP(isValid)>("isValid");
    addProperty<DAS_BIND_MANAGED_PROP(isRiExtra)>("isRiExtra");
    addProperty<DAS_BIND_MANAGED_PROP(getRiExtraHandle)>("riExtraHandle", "getRiExtraHandle");
  }
  bool isLocal() const override { return true; } // force isLocal, because ctor is non trivial
  bool canBePlacedInContainer() const override { return true; }
};

struct RiExtraComponentAnnotation : das::ManagedStructureAnnotation<RiExtraComponent, false>
{
  RiExtraComponentAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("RiExtraComponent", ml)
  {
    cppName = " ::RiExtraComponent";
    addField<DAS_BIND_MANAGED_FIELD(handle)>("handle");
  }
};

struct CollisionInfoAnnotation final : das::ManagedStructureAnnotation<rendinst::CollisionInfo, false>
{
  CollisionInfoAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CollisionInfo", ml)
  {
    cppName = " ::rendinst::CollisionInfo";
    addField<DAS_BIND_MANAGED_FIELD(handle)>("handle");
    addField<DAS_BIND_MANAGED_FIELD(collRes)>("collRes");
    addField<DAS_BIND_MANAGED_FIELD(riPoolRef)>("riPoolRef");
    addField<DAS_BIND_MANAGED_FIELD(tm)>("tm");
    addField<DAS_BIND_MANAGED_FIELD(localBBox)>("localBBox");
    addField<DAS_BIND_MANAGED_FIELD(isImmortal)>("isImmortal");
    addField<DAS_BIND_MANAGED_FIELD(stopsBullets)>("stopsBullets");
    addField<DAS_BIND_MANAGED_FIELD(isDestr)>("isDestr");
    addField<DAS_BIND_MANAGED_FIELD(bushBehaviour)>("bushBehaviour");
    addField<DAS_BIND_MANAGED_FIELD(treeBehaviour)>("treeBehaviour");
    addField<DAS_BIND_MANAGED_FIELD(destrImpulse)>("destrImpulse");
    addField<DAS_BIND_MANAGED_FIELD(hp)>("hp");
    addField<DAS_BIND_MANAGED_FIELD(initialHp)>("initialHp");
    addField<DAS_BIND_MANAGED_FIELD(isParent)>("isParent");
    addField<DAS_BIND_MANAGED_FIELD(destructibleByParent)>("destructibleByParent");
    addField<DAS_BIND_MANAGED_FIELD(destroyNeighbourDepth)>("destroyNeighbourDepth");
    addField<DAS_BIND_MANAGED_FIELD(destrFxId)>("destrFxId");
    addField<DAS_BIND_MANAGED_FIELD(desc)>("desc");
    addField<DAS_BIND_MANAGED_FIELD(userData)>("userData");
  }
};

namespace bind_dascript
{

void draw_debug_collisions(int flags, const Point3 &view_pos, bool reverse_depth, float max_coll_dist_sq, float max_label_dist_sq)
{
  // TODO: move to das
  mat44f globtm;
  d3d::getglobtm(globtm);
  rendinst::drawDebugCollisions(static_cast<rendinst::DrawCollisionsFlag>(flags), globtm, view_pos, reverse_depth, max_coll_dist_sq,
    max_label_dist_sq);
}

class RendInstModule final : public das::Module
{
public:
  RendInstModule() : das::Module("RendInst")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("CollRes"));
    addBuiltinDependency(lib, require("DagorMath"));
    addAnnotation(das::make_smart<RendInstDescAnnotation>(lib));
    addEnumeration(das::make_smart<EnumerationDrawCollisionsFlags>());
    addAnnotation(das::make_smart<CollisionInfoAnnotation>(lib));
    das::addUsing<rendinst::CollisionInfo>(*this, lib, " ::rendinst::CollisionInfo");

    G_STATIC_ASSERT(sizeof(rendinst::riex_handle_t) == sizeof(uint64_t));
    auto pType = das::make_smart<das::TypeDecl>(das::Type::tUInt64);
    pType->alias = "riex_handle_t";
    addAlias(pType);

    das::addConstant(*this, "RIEX_HANDLE_NULL", rendinst::RIEX_HANDLE_NULL);
    das::addConstant(*this, "RI_COLLISION_RES_SUFFIX", RI_COLLISION_RES_SUFFIX);

    das::addExtern<DAS_BIND_FUN(rendinst::getRiGenExtraResCount)>(*this, lib, "rendinst_getRiGenExtraResCount",
      das::SideEffects::accessExternal, "rendinst::getRiGenExtraResCount");
    das::addExtern<DAS_BIND_FUN(find_ri_extra_eid)>(*this, lib, "find_ri_extra_eid", das::SideEffects::accessExternal,
      "find_ri_extra_eid");
    das::addExtern<DAS_BIND_FUN(replace_ri_extra_res)>(*this, lib, "replace_ri_extra_res", das::SideEffects::modifyExternal,
      "replace_ri_extra_res");
    das::addExtern<DAS_BIND_FUN(traceTransparencyRayRIGenNormalized)>(*this, lib, "traceTransparencyRayRIGenNormalized",
      das::SideEffects::accessExternal, "bind_dascript::traceTransparencyRayRIGenNormalized");
    das::addExtern<DAS_BIND_FUN(traceTransparencyRayRIGenNormalizedEx)>(*this, lib, "traceTransparencyRayRIGenNormalized",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::traceTransparencyRayRIGenNormalizedEx");
    das::addExtern<DAS_BIND_FUN(traceTransparencyRayRIGenNormalized_with_mat_id)>(*this, lib, "traceTransparencyRayRIGenNormalized",
      das::SideEffects::accessExternal, "bind_dascript::traceTransparencyRayRIGenNormalized_with_mat_id");
    das::addExtern<DAS_BIND_FUN(rendinst::riex_get_lbb), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "riex_get_lbb",
      das::SideEffects::accessExternal, "rendinst::riex_get_lbb");
    das::addExtern<DAS_BIND_FUN(get_rigen_extra_res_idx)>(*this, lib, "get_rigen_extra_res_idx", das::SideEffects::accessExternal,
      "bind_dascript::get_rigen_extra_res_idx");
    das::addExtern<DAS_BIND_FUN(get_rigen_extra_matrix)>(*this, lib, "get_rigen_extra_matrix", das::SideEffects::accessExternal,
      "bind_dascript::get_rigen_extra_matrix");
    das::addExtern<DAS_BIND_FUN(rendinst::handle_to_ri_type)>(*this, lib, "handle_to_ri_type", das::SideEffects::accessExternal,
      "rendinst::handle_to_ri_type");
    das::addExtern<DAS_BIND_FUN(rendinst::handle_to_ri_inst)>(*this, lib, "handle_to_ri_inst", das::SideEffects::accessExternal,
      "rendinst::handle_to_ri_inst");


    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenExtraResIdx)>(*this, lib, "getRIGenExtraResIdx", das::SideEffects::accessExternal,
      "rendinst::getRIGenExtraResIdx");
    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenExtraCollRes)>(*this, lib, "get_ri_gen_extra_collres",
      das::SideEffects::accessExternal, "rendinst::getRIGenExtraCollRes");
    das::addExtern<DAS_BIND_FUN(rendinst::getRiGenCollisionResource)>(*this, lib, "getRiGenCollisionResource",
      das::SideEffects::accessExternal, "rendinst::getRiGenCollisionResource");
    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenExtra44)>(*this, lib, "getRIGenExtra44",
      das::SideEffects::modifyArgumentAndAccessExternal, "rendinst::getRIGenExtra44");

    das::addExtern<DAS_BIND_FUN(get_collres_slice_mean_and_dispersion)>(*this, lib, "get_collres_slice_mean_and_dispersion",
      das::SideEffects::none, "get_collres_slice_mean_and_dispersion");

    das::addExtern<DAS_BIND_FUN(gather_ri_gen_extra_collidable)>(*this, lib, "gather_ri_gen_extra_collidable",
      das::SideEffects::accessExternal, "bind_dascript::gather_ri_gen_extra_collidable");
    das::addExtern<DAS_BIND_FUN(get_ri_gen_extra_instances)>(*this, lib, "get_ri_gen_extra_instances",
      das::SideEffects::accessExternal, "bind_dascript::get_ri_gen_extra_instances");
    das::addExtern<DAS_BIND_FUN(get_ri_gen_extra_instances_by_box)>(*this, lib, "getRiGenExtraInstances",
      das::SideEffects::accessExternal, "bind_dascript::get_ri_gen_extra_instances_by_box");
    das::addExtern<DAS_BIND_FUN(bind_dascript::getRiGenDestrInfo)>(*this, lib, "getRiGenDestrInfo", das::SideEffects::modifyArgument,
      "bind_dascript::getRiGenDestrInfo");
    das::addExtern<DAS_BIND_FUN(rendinst::setRiGenExtraHp)>(*this, lib, "riex_set_hp", das::SideEffects::modifyExternal,
      "rendinst::setRiGenExtraHp");
    das::addExtern<DAS_BIND_FUN(rendinst::getHP)>(*this, lib, "riex_get_hp", das::SideEffects::accessExternal, "rendinst::getHP");
    das::addExtern<DAS_BIND_FUN(rendinst::setRIGenExtraImmortal)>(*this, lib, "set_riex_immortal", das::SideEffects::modifyExternal,
      "rendinst::setRIGenExtraImmortal");
    das::addExtern<DAS_BIND_FUN(rendinst::getInitialHP)>(*this, lib, "riex_getInitialHP", das::SideEffects::accessExternal,
      "rendinst::getInitialHP");
    das::addExtern<DAS_BIND_FUN(rendinst::isPaintFxOnHit)>(*this, lib, "riex_isPaintFxOnHit", das::SideEffects::accessExternal,
      "rendinst::isPaintFxOnHit");
    das::addExtern<DAS_BIND_FUN(rendinst::isRiGenExtraValid)>(*this, lib, "riex_isRiGenExtraValid", das::SideEffects::accessExternal,
      "rendinst::isRiGenExtraValid");

    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenExtraName)>(*this, lib, "riex_getRIGenExtraName", das::SideEffects::accessExternal,
      "rendinst::getRIGenExtraName");
    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenResName)>(*this, lib, "getRIGenResName", das::SideEffects::accessExternal,
      "rendinst::getRIGenResName");
    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenDestrName)>(*this, lib, "getRIGenDestrName", das::SideEffects::accessExternal,
      "rendinst::getRIGenDestrName");

    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenBBox), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "getRIGenBBox",
      das::SideEffects::accessExternal, "rendinst::getRIGenBBox");
    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenMatrix), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "getRIGenMatrix",
      das::SideEffects::accessExternal, "rendinst::getRIGenMatrix");
    das::addExtern<DAS_BIND_FUN(rendinst::isRIGenExtraImmortal)>(*this, lib, "riex_isImmortal", das::SideEffects::accessExternal,
      "rendinst::isRIGenExtraImmortal");
    das::addExtern<DAS_BIND_FUN(rendinst::isRIGenExtraWalls)>(*this, lib, "riex_isWalls", das::SideEffects::accessExternal,
      "rendinst::isRIGenExtraWalls");
    das::addExtern<DAS_BIND_FUN(get_ri_gen_extra_bsphere)>(*this, lib, "getRIGenExtraBSphere", das::SideEffects::accessExternal,
      "bind_dascript::get_ri_gen_extra_bsphere");
    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenExtraBSphereByTM)>(*this, lib, "getRIGenExtraBSphereByTM",
      das::SideEffects::accessExternal, "rendinst::getRIGenExtraBSphereByTM");
    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenExtraBsphRad)>(*this, lib, "riex_getBsphRad", das::SideEffects::accessExternal,
      "rendinst::getRIGenExtraBsphRad");
    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenExtraBsphPos)>(*this, lib, "getRIGenExtraBsphPos", das::SideEffects::accessExternal,
      "rendinst::getRIGenExtraBsphPos");
    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenExtraParentForDestroyedRiIdx)>(*this, lib, "riex_getParentForDestroyedRiIdx",
      das::SideEffects::accessExternal, "rendinst::getRIGenExtraParentForDestroyedRiIdx");
    das::addExtern<DAS_BIND_FUN(rendinst::isRIGenExtraDestroyedPhysResExist)>(*this, lib, "riex_isDestroyedPhysResExist",
      das::SideEffects::accessExternal, "rendinst::isRIGenExtraDestroyedPhysResExist");
    das::addExtern<DAS_BIND_FUN(rendinst::getRIGenExtraDestroyedRiIdx)>(*this, lib, "riex_getDestroyedRiIdx",
      das::SideEffects::accessExternal, "rendinst::getRIGenExtraDestroyedRiIdx");
    das::addExtern<DAS_BIND_FUN(draw_debug_collisions)>(*this, lib, "rendinst_drawDebugCollisions", das::SideEffects::modifyExternal,
      "draw_debug_collisions");
    das::addExtern<DAS_BIND_FUN(damage_ri_in_sphere)>(*this, lib, "damage_ri_in_sphere", das::SideEffects::modifyExternal,
      "bind_dascript::damage_ri_in_sphere");
    das::addExtern<DAS_BIND_FUN(doRIGenDamage)>(*this, lib, "rendinst_doRIGenDamage", das::SideEffects::modifyExternal,
      "bind_dascript::doRIGenDamage");
    das::addExtern<DAS_BIND_FUN(doRendinstDamage)>(*this, lib, "rendinst_doRendinstDamage", das::SideEffects::modifyExternal,
      "bind_dascript::doRendinstDamage");
    das::addExtern<DAS_BIND_FUN(rendinst::isRIGenDestr)>(*this, lib, "rendinst_isRIGenDestr", das::SideEffects::accessExternal,
      "rendinst::isRIGenDestr");
    das::addExtern<DAS_BIND_FUN(rendinst::isRIGenPosInst)>(*this, lib, "rendinst_isRIGenPosInst", das::SideEffects::accessExternal,
      "rendinst::isRIGenPosInst");
    das::addExtern<DAS_BIND_FUN(rendinst::isRIExtraGenPosInst)>(*this, lib, "rendinst_isRIExtraGenPosInst",
      das::SideEffects::accessExternal, "rendinst::isRIExtraGenPosInst");
    das::addExtern<DAS_BIND_FUN(rendinst::updateRiExtraReqLod)>(*this, lib, "rendinst_updateRiExtraReqLod",
      das::SideEffects::modifyExternal, "rendinst::updateRiExtraReqLod");
    das::addExtern<DAS_BIND_FUN(rendinst::isDestroyedRIExtraFromNextRes)>(*this, lib, "rendinst_isDestroyedRIExtraFromNextRes",
      das::SideEffects::accessExternal, "rendinst::isDestroyedRIExtraFromNextRes");
    das::addExtern<DAS_BIND_FUN(rendinst::getRIExtraNextResIdx)>(*this, lib, "rendinst_getRIExtraNextResIdx",
      das::SideEffects::accessExternal, "rendinst::getRIExtraNextResIdx");
    das::addExtern<DAS_BIND_FUN(rendinst::setRiGenResMatId)>(*this, lib, "rendinst_setRiGenResMatId", das::SideEffects::modifyExternal,
      "rendinst::setRiGenResMatId");
    das::addExtern<DAS_BIND_FUN(rendinst::setRiGenResHp)>(*this, lib, "rendinst_setRiGenResHp", das::SideEffects::modifyExternal,
      "rendinst::setRiGenResHp");
    das::addExtern<DAS_BIND_FUN(rendinst::setRiGenResDestructionImpulse)>(*this, lib, "rendinst_setRiGenResDestructionImpulse",
      das::SideEffects::modifyExternal, "rendinst::setRiGenResDestructionImpulse");
    das::addExtern<DAS_BIND_FUN(rendinst_addRIGenExtraResIdx)>(*this, lib, "rendinst_addRIGenExtraResIdx",
      das::SideEffects::modifyExternal, "bind_dascript::rendinst_addRIGenExtraResIdx");
    das::addExtern<DAS_BIND_FUN(rendinst::addRiGenExtraDebris)>(*this, lib, "rendinst_addRiGenExtraDebris",
      das::SideEffects::modifyExternal, "rendinst::addRiGenExtraDebris");
    das::addExtern<DAS_BIND_FUN(rendinst::reloadRIExtraResources)>(*this, lib, "rendinst_reloadRIExtraResources",
      das::SideEffects::modifyExternal, "rendinst::reloadRIExtraResources");
    das::addExtern<DAS_BIND_FUN(rendinst_cloneRIGenExtraResIdx)>(*this, lib, "rendinst_cloneRIGenExtraResIdx",
      das::SideEffects::modifyExternal, "bind_dascript::rendinst_cloneRIGenExtraResIdx");
    das::addExtern<DAS_BIND_FUN(rendinst::delRIGenExtra)>(*this, lib, "rendinst_delRIGenExtra", das::SideEffects::modifyExternal,
      "rendinst::delRIGenExtra");
    das::addExtern<DAS_BIND_FUN(rendinst_foreachRIGenInBox)>(*this, lib, "rendinst_foreachRIGenInBox",
      das::SideEffects::accessExternal, "bind_dascript::rendinst_foreachRIGenInBox");
    das::addExtern<DAS_BIND_FUN(rendinst::moveToOriginalScene)>(*this, lib, "rendinst_moveToOriginalScene",
      das::SideEffects::modifyExternal, "rendinst::moveToOriginalScene");
    das::addExtern<DAS_BIND_FUN(rendinst::applyTiledScenesUpdateForRIGenExtra)>(*this, lib, "applyTiledScenesUpdateForRIGenExtra",
      das::SideEffects::modifyExternal, "rendinst::applyTiledScenesUpdateForRIGenExtra");
    das::addExtern<DAS_BIND_FUN(rendinst_foreachTreeInBox)>(*this, lib, "rendinst_foreachTreeInBox", das::SideEffects::accessExternal,
      "bind_dascript::rendinst_foreachTreeInBox");
    das::addExtern<DAS_BIND_FUN(get_ri_color_infos)>(*this, lib, "get_ri_color_infos", das::SideEffects::accessExternal,
      "bind_dascript::get_ri_color_infos");
    das::addExtern<DAS_BIND_FUN(iterate_riextra_map)>(*this, lib, "iterate_riextra_map", das::SideEffects::accessExternal,
      "bind_dascript::iterate_riextra_map");
    das::addExtern<DAS_BIND_FUN(rendinst::gpuobjects::erase_inside_sphere)>(*this, lib, "erase_gpu_objects",
      das::SideEffects::modifyExternal, "rendinst::gpuobjects::erase_inside_sphere");

    das::addCtorAndUsing<rendinst::RendInstDesc>(*this, lib, "RendInstDesc", "::rendinst::RendInstDesc");
    das::addCtorAndUsing<rendinst::RendInstDesc, rendinst::riex_handle_t>(*this, lib, "RendInstDesc", "::rendinst::RendInstDesc");
    das::addCtorAndUsing<rendinst::RendInstDesc, int, int, int, uint32_t, int>(*this, lib, "RendInstDesc", "::rendinst::RendInstDesc");

    addAnnotation(das::make_smart<RiExtraComponentAnnotation>(lib));
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotRendInst.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(RendInstModule, bind_dascript);
