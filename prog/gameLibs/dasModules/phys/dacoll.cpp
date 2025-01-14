// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotDacoll.h>
#include <dasModules/dasDataBlock.h>
#include <dasModules/dasModulesCommon.h>


DAS_BASE_BIND_ENUM_98(dacoll::PhysLayer, PhysLayer, EPL_DEFAULT, EPL_STATIC, EPL_KINEMATIC, EPL_DEBRIS, EPL_SENSOR, EPL_CHARACTER,
  EPL_ALL);
DAS_BASE_BIND_ENUM_98(dacoll::CollType, CollType, ETF_LMESH, ETF_FRT, ETF_RI, ETF_RESTORABLES, ETF_OBJECTS_GROUP, ETF_STRUCTURES,
  ETF_HEIGHTMAP, ETF_STATIC, ETF_RI_TREES, ETF_RI_PHYS, ETF_DEFAULT, ETF_ALL);
struct TraceMeshFacesAnnotation : das::ManagedStructureAnnotation<TraceMeshFaces, false>
{
  TraceMeshFacesAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("TraceMeshFaces", ml)
  {
    cppName = " ::TraceMeshFaces";
  }
};

struct CollisionObjectAnnotation : das::ManagedStructureAnnotation<CollisionObject, false>
{
  CollisionObjectAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CollisionObject", ml)
  {
    cppName = " ::CollisionObject";
  }
};

struct ShapeQueryOutputAnnotation : das::ManagedStructureAnnotation<dacoll::ShapeQueryOutput, false>
{
  ShapeQueryOutputAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ShapeQueryOutput", ml)
  {
    cppName = " ::dacoll::ShapeQueryOutput";
    addField<DAS_BIND_MANAGED_FIELD(res)>("res");
    addField<DAS_BIND_MANAGED_FIELD(norm)>("norm");
    addField<DAS_BIND_MANAGED_FIELD(vel)>("vel");
    addField<DAS_BIND_MANAGED_FIELD(t)>("t");
  }
  virtual bool isLocal() const { return true; } // force isLocal, because ctor is non trivial
};


struct CollisionContactDataMinAnnotation : das::ManagedStructureAnnotation<gamephys::CollisionContactDataMin, false>
{
  CollisionContactDataMinAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CollisionContactDataMin", ml)
  {
    cppName = " ::gamephys::CollisionContactDataMin";
    addField<DAS_BIND_MANAGED_FIELD(wpos)>("wpos");
    addField<DAS_BIND_MANAGED_FIELD(matId)>("matId");
  }
};

struct CollisionContactDataAnnotation final : das::ManagedStructureAnnotation<gamephys::CollisionContactData, false>
{
  das::TypeDeclPtr parentType;

  CollisionContactDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CollisionContactData", ml)
  {
    cppName = " ::gamephys::CollisionContactData";
    addField<DAS_BIND_MANAGED_FIELD(wpos)>("wpos");
    addField<DAS_BIND_MANAGED_FIELD(wposB)>("wposB");
    addField<DAS_BIND_MANAGED_FIELD(wnormB)>("wnormB");
    addField<DAS_BIND_MANAGED_FIELD(posA)>("posA");
    addField<DAS_BIND_MANAGED_FIELD(posB)>("posB");
    addField<DAS_BIND_MANAGED_FIELD(depth)>("depth");
    addField<DAS_BIND_MANAGED_FIELD(riPool)>("riPool");
    addField<DAS_BIND_MANAGED_FIELD(matId)>("matId");

    static_assert(eastl::is_base_of_v<gamephys::CollisionContactDataMin, gamephys::CollisionContactData>);
    parentType = das::makeType<gamephys::CollisionContactDataMin>(ml);
  }

  bool canBePlacedInContainer() const override { return true; }

  bool canBeSubstituted(TypeAnnotation *pass_type) const override { return parentType->annotation == pass_type; }
};

struct CollisionLinkDataAnnotation : das::ManagedStructureAnnotation<dacoll::CollisionLinkData, false>
{
  CollisionLinkDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CollisionLinkData", ml)
  {
    cppName = " ::dacoll::CollisionLinkData";
    addField<DAS_BIND_MANAGED_FIELD(axis)>("axis");
    addField<DAS_BIND_MANAGED_FIELD(offset)>("offset");
    addField<DAS_BIND_MANAGED_FIELD(capsuleRadiusScale)>("capsuleRadiusScale");
    addField<DAS_BIND_MANAGED_FIELD(oriParamMult)>("oriParamMult");
    addField<DAS_BIND_MANAGED_FIELD(nameId)>("nameId");
    addField<DAS_BIND_MANAGED_FIELD(haveCollision)>("haveCollision");
  }
};

namespace bind_dascript
{
static char dacoll_das[] =
#include "dacoll.das.inl"
  ;

class DacollModule final : public das::Module
{
public:
  DacollModule() : das::Module("Dacoll")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("CollRes"));
    addBuiltinDependency(lib, require("RendInst"), true);
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("DagorDataBlock"));
    addEnumeration(das::make_smart<EnumerationPhysLayer>());
    das::addEnumFlagOps<dacoll::PhysLayer>(*this, lib, "dacoll::PhysLayer");
    addEnumeration(das::make_smart<EnumerationCollType>());
    das::addEnumFlagOps<dacoll::CollType>(*this, lib, "dacoll::CollType");
    addAnnotation(das::make_smart<TraceMeshFacesAnnotation>(lib));
    addAnnotation(das::make_smart<CollisionObjectAnnotation>(lib));
    addAnnotation(das::make_smart<ShapeQueryOutputAnnotation>(lib));
    addAnnotation(das::make_smart<CollisionContactDataMinAnnotation>(lib));
    addAnnotation(das::make_smart<CollisionContactDataAnnotation>(lib));
    addAnnotation(das::make_smart<CollisionLinkDataAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(dacoll_traceray_normalized)>(*this, lib, "traceray_normalized", das::SideEffects::modifyArgument,
      "bind_dascript::dacoll_traceray_normalized");
    das::addExtern<DAS_BIND_FUN(dacoll_traceray_normalized_coll_type)>(*this, lib, "traceray_normalized_coll_type",
      das::SideEffects::modifyArgument, "bind_dascript::dacoll_traceray_normalized_coll_type");
    das::addExtern<DAS_BIND_FUN(dacoll_traceray_normalized_trace_handle)>(*this, lib, "traceray_normalized",
      das::SideEffects::modifyArgument, "bind_dascript::dacoll_traceray_normalized_trace_handle");
    das::addExtern<DAS_BIND_FUN(dacoll_traceray_normalized_trace_handle2)>(*this, lib, "traceray_normalized",
      das::SideEffects::modifyArgument, "bind_dascript::dacoll_traceray_normalized_trace_handle2");
    das::addExtern<DAS_BIND_FUN(dacoll_tracedown_normalized)>(*this, lib, "tracedown_normalized", das::SideEffects::modifyArgument,
      "bind_dascript::dacoll_tracedown_normalized");
    das::addExtern<DAS_BIND_FUN(dacoll_tracedown_normalized_with_mat_id)>(*this, lib, "tracedown_normalized_with_mat_id",
      das::SideEffects(uint32_t(das::SideEffects::modifyArgument) | uint32_t(das::SideEffects::accessExternal)),
      "bind_dascript::dacoll_tracedown_normalized_with_mat_id");
    das::addExtern<DAS_BIND_FUN(dacoll_tracedown_normalized_with_mat_id_ex)>(*this, lib, "tracedown_normalized_with_mat_id",
      das::SideEffects(uint32_t(das::SideEffects::modifyArgument) | uint32_t(das::SideEffects::accessExternal)),
      "bind_dascript::dacoll_tracedown_normalized_with_mat_id_ex");
    das::addExtern<DAS_BIND_FUN(dacoll_tracedown_normalized_with_norm)>(*this, lib, "tracedown_normalized",
      das::SideEffects::modifyArgument, "bind_dascript::dacoll_tracedown_normalized_with_norm");
    das::addExtern<DAS_BIND_FUN(dacoll_tracedown_normalized_with_pmid)>(*this, lib, "tracedown_normalized",
      das::SideEffects::modifyArgument, "bind_dascript::dacoll_tracedown_normalized_with_pmid");
    das::addExtern<DAS_BIND_FUN(dacoll_tracedown_normalized_with_norm_and_pmid)>(*this, lib, "tracedown_normalized",
      das::SideEffects::modifyArgument, "bind_dascript::dacoll_tracedown_normalized_with_norm_and_pmid");
    das::addExtern<DAS_BIND_FUN(dacoll_tracedown_normalized_trace_handle_with_pmid)>(*this, lib, "tracedown_normalized",
      das::SideEffects::modifyArgument, "bind_dascript::dacoll_tracedown_normalized_trace_handle_with_pmid");
    das::addExtern<DAS_BIND_FUN(dacoll::traceht_water)>(*this, lib, "traceht_water", das::SideEffects::modifyArgument,
      "dacoll::traceht_water");
    das::addExtern<DAS_BIND_FUN(dacoll_validate_trace_cache)>(*this, lib, "validate_trace_cache", das::SideEffects::modifyArgument,
      "bind_dascript::dacoll_validate_trace_cache");
    das::addExtern<DAS_BIND_FUN(dacoll::trace_game_objects)>(*this, lib, "trace_game_objects", das::SideEffects::modifyArgument,
      "dacoll::trace_game_objects");
    das::addExtern<DAS_BIND_FUN(dacoll_rayhit_normalized)>(*this, lib, "rayhit_normalized", das::SideEffects::accessExternal,
      "bind_dascript::dacoll_rayhit_normalized");
    das::addExtern<DAS_BIND_FUN(dacoll_rayhit_normalized_trace_handle)>(*this, lib, "rayhit_normalized",
      das::SideEffects::accessExternal, "bind_dascript::dacoll_rayhit_normalized_trace_handle");
    das::addExtern<DAS_BIND_FUN(dacoll_traceray_normalized_frt)>(*this, lib, "traceray_normalized_frt",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::dacoll_traceray_normalized_frt");
    das::addExtern<DAS_BIND_FUN(dacoll_traceray_normalized_lmesh)>(*this, lib, "traceray_normalized_lmesh",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::dacoll_traceray_normalized_lmesh");
    das::addExtern<DAS_BIND_FUN(dacoll_traceray_normalized_ri)>(*this, lib, "traceray_normalized_ri",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::dacoll_traceray_normalized_ri");
    das::addExtern<DAS_BIND_FUN(dacoll::rayhit_normalized_lmesh)>(*this, lib, "rayhit_normalized_lmesh",
      das::SideEffects::accessExternal, "dacoll::rayhit_normalized_lmesh");

    das::addExtern<DAS_BIND_FUN(dacoll::is_valid_heightmap_pos)>(*this, lib, "is_valid_heightmap_pos", das::SideEffects::none,
      "dacoll::is_valid_heightmap_pos");
    das::addExtern<DAS_BIND_FUN(dacoll::is_valid_water_height)>(*this, lib, "is_valid_water_height", das::SideEffects::none,
      "dacoll::is_valid_water_height");

    das::addExtern<DAS_BIND_FUN(dacoll_traceht_water_at_time)>(*this, lib, "traceht_water_at_time", das::SideEffects::modifyArgument,
      "bind_dascript::dacoll_traceht_water_at_time");
    das::addExtern<DAS_BIND_FUN(dacoll::traceray_water_at_time)>(*this, lib, "traceray_water_at_time",
      das::SideEffects::modifyArgumentAndAccessExternal, "dacoll::traceray_water_at_time");


    das::addExtern<DAS_BIND_FUN(dacoll::get_min_max_hmap_in_circle)>(*this, lib, "get_min_max_hmap_in_circle",
      das::SideEffects::modifyArgument, "dacoll::get_min_max_hmap_in_circle");

    das::addExtern<DAS_BIND_FUN(dacoll::traceht_lmesh)>(*this, lib, "traceht_lmesh", das::SideEffects::accessExternal,
      "dacoll::traceht_lmesh");
    das::addExtern<DAS_BIND_FUN(dacoll::traceht_hmap)>(*this, lib, "traceht_hmap", das::SideEffects::accessExternal,
      "dacoll::traceht_hmap");

    das::addExtern<DAS_BIND_FUN(_builtin_sphere_cast)>(*this, lib, "_builtin_sphere_cast", das::SideEffects::modifyArgument,
      "bind_dascript::_builtin_sphere_cast");

    das::addExtern<DAS_BIND_FUN(dacoll_sphere_cast_with_collision_object_ex)>(*this, lib, "sphere_cast_ex",
      das::SideEffects::modifyArgument, "bind_dascript::dacoll_sphere_cast_with_collision_object_ex");
    das::addExtern<DAS_BIND_FUN(dacoll_sphere_cast_land)>(*this, lib, "sphere_cast_land", das::SideEffects::modifyArgument,
      "bind_dascript::dacoll_sphere_cast_land");
    das::addExtern<DAS_BIND_FUN(dacoll_sphere_cast_ex)>(*this, lib, "sphere_cast_ex", das::SideEffects::modifyArgument,
      "bind_dascript::dacoll_sphere_cast_ex");
    das::addExtern<DAS_BIND_FUN(dacoll::trace_sphere_cast_ex)>(*this, lib, "trace_sphere_cast_ex", das::SideEffects::modifyArgument,
      "dacoll::trace_sphere_cast_ex");

    das::addExtern<DAS_BIND_FUN(get_min_max_hmap_list_in_circle)>(*this, lib, "get_min_max_hmap_list_in_circle",
      das::SideEffects::invoke, "bind_dascript::get_min_max_hmap_list_in_circle");

    das::addExtern<DAS_BIND_FUN(dacoll_traceray_normalized_ex)>(*this, lib, "traceray_normalized",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::dacoll_traceray_normalized_ex");

    das::addExtern<DAS_BIND_FUN(dacoll_sphere_query_ri)>(*this, lib, "sphere_query_ri", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::dacoll_sphere_query_ri");
    das::addExtern<DAS_BIND_FUN(dacoll_sphere_query_ri_filtered)>(*this, lib, "sphere_query_ri",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::dacoll_sphere_query_ri_filtered");

    das::addExtern<DAS_BIND_FUN(dacoll_add_dynamic_collision_from_coll_resource)>(*this, lib,
      "add_dynamic_collision_from_coll_resource", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::dacoll_add_dynamic_collision_from_coll_resource");

    das::addExtern<DAS_BIND_FUN(dacoll::add_dynamic_collision), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "add_dynamic_collision", das::SideEffects::accessExternal, "dacoll::add_dynamic_collision");

    das::addExtern<DAS_BIND_FUN(bind_dascript::dacoll_add_dynamic_sphere_collision)>(*this, lib, "add_dynamic_sphere_collision",
      das::SideEffects::modifyArgument, "bind_dascript::dacoll_add_dynamic_sphere_collision");
    das::addExtern<DAS_BIND_FUN(bind_dascript::dacoll_add_dynamic_capsule_collision)>(*this, lib, "add_dynamic_capsule_collision",
      das::SideEffects::modifyArgument, "bind_dascript::dacoll_add_dynamic_capsule_collision");
    das::addExtern<DAS_BIND_FUN(bind_dascript::dacoll_add_dynamic_cylinder_collision)>(*this, lib, "add_dynamic_cylinder_collision",
      das::SideEffects::modifyArgument, "bind_dascript::dacoll_add_dynamic_cylinder_collision");
    das::addExtern<DAS_BIND_FUN(dacoll::add_collision_hmap_custom)>(*this, lib, "add_collision_hmap_custom",
      das::SideEffects::modifyExternal, "dacoll::add_collision_hmap_custom");


    das::addExtern<DAS_BIND_FUN(dacoll_destroy_dynamic_collision)>(*this, lib, "destroy_dynamic_collision",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::dacoll_destroy_dynamic_collision");

    das::addExtern<DAS_BIND_FUN(dacoll_test_collision_world)>(*this, lib, "test_collision_world", das::SideEffects::accessExternal,
      "bind_dascript::dacoll_test_collision_world");

    das::addExtern<DAS_BIND_FUN(dacoll_test_collision_world_ex)>(*this, lib, "test_collision_world_ex",
      das::SideEffects::accessExternal, "bind_dascript::dacoll_test_collision_world_ex");

    das::addExtern<DAS_BIND_FUN(dacoll::update_ri_cache_in_volume_to_phys_world)>(*this, lib,
      "update_ri_cache_in_volume_to_phys_world", das::SideEffects::modifyExternal, "dacoll::update_ri_cache_in_volume_to_phys_world");

    das::addExtern<DAS_BIND_FUN(dacoll_test_collision_ri_ex)>(*this, lib, "test_collision_ri_ex", das::SideEffects::modifyExternal,
      "bind_dascript::dacoll_test_collision_ri_ex");

    das::addExtern<DAS_BIND_FUN(dacoll_test_collision_ri)>(*this, lib, "test_collision_ri", das::SideEffects::modifyExternal,
      "bind_dascript::dacoll_test_collision_ri");

    das::addExtern<DAS_BIND_FUN(dacoll_test_collision_frt)>(*this, lib, "test_collision_frt", das::SideEffects::modifyExternal,
      "bind_dascript::dacoll_test_collision_frt");

    das::addExtern<DAS_BIND_FUN(dacoll_test_sphere_collision_world)>(*this, lib, "test_sphere_collision_world",
      das::SideEffects::accessExternal, "bind_dascript::dacoll_test_sphere_collision_world");

    das::addExtern<DAS_BIND_FUN(dacoll_test_box_collision_world)>(*this, lib, "test_box_collision_world",
      das::SideEffects::accessExternal, "bind_dascript::dacoll_test_box_collision_world");

    das::addExtern<DAS_BIND_FUN(dacoll_fetch_sim_res)>(*this, lib, "dacoll_fetch_sim_res", das::SideEffects::accessExternal,
      "bind_dascript::dacoll_fetch_sim_res");
    das::addExtern<DAS_BIND_FUN(dacoll::set_collision_object_tm)>(*this, lib, "dacoll_set_collision_object_tm",
      das::SideEffects::modifyArgumentAndExternal, "dacoll::set_collision_object_tm");
    das::addExtern<DAS_BIND_FUN(dacoll::set_vert_capsule_shape_size)>(*this, lib, "dacoll_set_vert_capsule_shape_size",
      das::SideEffects::modifyArgumentAndExternal, "dacoll::set_vert_capsule_shape_size");
    das::addExtern<DAS_BIND_FUN(dacoll::set_collision_sphere_rad)>(*this, lib, "dacoll_set_collision_sphere_rad",
      das::SideEffects::modifyArgumentAndExternal, "dacoll::set_collision_sphere_rad");
    das::addExtern<DAS_BIND_FUN(dacoll_draw_collision_object)>(*this, lib, "dacoll_draw_collision_object",
      das::SideEffects::modifyExternal, "bind_dascript::dacoll_draw_collision_object");

    das::addExtern<DAS_BIND_FUN(dacoll::enable_disable_ri_instance)>(*this, lib, "enable_disable_ri_instance",
      das::SideEffects::modifyExternal, "dacoll::enable_disable_ri_instance");

    das::addExtern<DAS_BIND_FUN(dacoll::flush_ri_instances)>(*this, lib, "flush_ri_instances", das::SideEffects::modifyExternal,
      "dacoll::flush_ri_instances");

    das::addExtern<DAS_BIND_FUN(bind_dascript::use_box_collision)>(*this, lib, "dacoll_use_box_collision",
      das::SideEffects::modifyExternal, "bind_dascript::use_box_collision");
    das::addExtern<DAS_BIND_FUN(bind_dascript::use_sphere_collision)>(*this, lib, "dacoll_use_sphere_collision",
      das::SideEffects::modifyExternal, "bind_dascript::use_sphere_collision");
    das::addExtern<DAS_BIND_FUN(bind_dascript::use_capsule_collision)>(*this, lib, "dacoll_use_capsule_collision",
      das::SideEffects::modifyExternal, "bind_dascript::use_capsule_collision");

    das::addExtern<DAS_BIND_FUN(dacoll_shape_query_frt)>(*this, lib, "dacoll_shape_query_frt",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::dacoll_shape_query_frt");
    das::addExtern<DAS_BIND_FUN(dacoll_shape_query_lmesh)>(*this, lib, "dacoll_shape_query_lmesh",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::dacoll_shape_query_lmesh");
    das::addExtern<DAS_BIND_FUN(dacoll_shape_query_ri)>(*this, lib, "dacoll_shape_query_ri",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::dacoll_shape_query_ri");
    das::addExtern<DAS_BIND_FUN(dacoll_shape_query_world)>(*this, lib, "dacoll_shape_query_world",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::dacoll_shape_query_world");

    das::addExtern<DAS_BIND_FUN(dacoll::set_hmap_step)>(*this, lib, "dacoll_set_hmap_step", das::SideEffects::modifyExternal,
      "dacoll::set_hmap_step");
    das::addExtern<DAS_BIND_FUN(dacoll::get_hmap_step)>(*this, lib, "dacoll_get_hmap_step", das::SideEffects::accessExternal,
      "dacoll::get_hmap_step");

    das::addExtern<DAS_BIND_FUN(dacoll::get_lmesh_mat_id_at_point)>(*this, lib, "dacoll_get_lmesh_mat_id_at_point",
      das::SideEffects::accessExternal, "dacoll::get_lmesh_mat_id_at_point");

    using method_invalidate = DAS_CALL_MEMBER(rendinst::RendInstDesc::invalidate);
    das::addExtern<DAS_CALL_METHOD(method_invalidate)>(*this, lib, "invalidate", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(rendinst::RendInstDesc::invalidate));

    using method_markDirty = DAS_CALL_MEMBER(TraceMeshFaces::markDirty);
    das::addExtern<DAS_CALL_METHOD(method_markDirty)>(*this, lib, "markDirty", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(TraceMeshFaces::markDirty));

    das::addCtor<dacoll::ShapeQueryOutput>(*this, lib, "ShapeQueryOutput", "::dacoll::ShapeQueryOutput");
    das::addUsing<TraceMeshFaces>(*this, lib, "::TraceMeshFaces");

    das::addConstant<int>(*this, "DEFAULT_DYN_COLLISION_MASK", dacoll::DEFAULT_DYN_COLLISION_MASK);
    das::addConstant<int>(*this, "ETF_LMESH", dacoll::ETF_LMESH);
    das::addConstant<int>(*this, "ETF_FRT", dacoll::ETF_FRT);
    das::addConstant<int>(*this, "ETF_RI", dacoll::ETF_RI);
    das::addConstant<int>(*this, "ETF_RESTORABLES", dacoll::ETF_RESTORABLES);
    das::addConstant<int>(*this, "ETF_OBJECTS_GROUP", dacoll::ETF_OBJECTS_GROUP);
    das::addConstant<int>(*this, "ETF_STRUCTURES", dacoll::ETF_STRUCTURES);
    das::addConstant<int>(*this, "ETF_HEIGHTMAP", dacoll::ETF_HEIGHTMAP);
    das::addConstant<int>(*this, "ETF_STATIC", dacoll::ETF_STATIC);
    das::addConstant<int>(*this, "ETF_RI_TREES", dacoll::ETF_RI_TREES);
    das::addConstant<int>(*this, "ETF_RI_PHYS", dacoll::ETF_RI_PHYS);
    das::addConstant<int>(*this, "ETF_DEFAULT", dacoll::ETF_DEFAULT);
    das::addConstant<int>(*this, "ETF_ALL", dacoll::ETF_ALL);

    compileBuiltinModule("dacoll.das", (unsigned char *)dacoll_das, sizeof(dacoll_das));
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDacoll.h>\n";
    tw << "#include <dasModules/dasDataBlock.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DacollModule, bind_dascript);
