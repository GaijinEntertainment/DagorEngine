// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/aotPathFinder.h>
#include <pathFinder/pathFinder.h>

DAS_BASE_BIND_ENUM_98(pathfinder::FindPathResult, FindPathResult, FPR_FAILED, FPR_PARTIAL, FPR_FULL);
DAS_BASE_BIND_ENUM_98(pathfinder::PolyFlag, PolyFlag, POLYFLAG_GROUND, POLYFLAG_OBSTACLE, POLYFLAG_JUMP, POLYFLAG_LADDER,
  POLYFLAG_BLOCKED);
DAS_BASE_BIND_ENUM_98(dtStraightPathFlags, StraightPathFlags, DT_STRAIGHTPATH_START, DT_STRAIGHTPATH_END,
  DT_STRAIGHTPATH_OFFMESH_CONNECTION);

namespace bind_dascript
{
static char pathFinder_das[] =
#include "pathFinder.das.inl"
  ;

struct CustomNavAnnotation : das::ManagedStructureAnnotation<pathfinder::CustomNav, false>
{
  CustomNavAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CustomNav", ml) { cppName = "pathfinder::CustomNav"; }
};

struct FindRequestAnnotation : das::ManagedStructureAnnotation<pathfinder::FindRequest, false>
{
  FindRequestAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("FindRequest", ml)
  {
    cppName = " pathfinder::FindRequest";

    addField<DAS_BIND_MANAGED_FIELD(start)>("start");
    addField<DAS_BIND_MANAGED_FIELD(end)>("end");
    addField<DAS_BIND_MANAGED_FIELD(includeFlags)>("includeFlags");
    addField<DAS_BIND_MANAGED_FIELD(excludeFlags)>("excludeFlags");
    addField<DAS_BIND_MANAGED_FIELD(extents)>("extents");
    addField<DAS_BIND_MANAGED_FIELD(maxJumpUpHeight)>("maxJumpUpHeight");
    addField<DAS_BIND_MANAGED_FIELD(startPoly)>("startPoly");
    addField<DAS_BIND_MANAGED_FIELD(endPoly)>("endPoly");
    addField<DAS_BIND_MANAGED_FIELD(numPolys)>("numPolys");
    addField<DAS_BIND_MANAGED_FIELD(areasCost)>("areasCost");
  }
};

struct CorridorInputAnnotation : das::ManagedStructureAnnotation<pathfinder::CorridorInput, false>
{
  CorridorInputAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CorridorInput", ml)
  {
    cppName = " pathfinder::CorridorInput";

    addField<DAS_BIND_MANAGED_FIELD(start)>("start");
    addField<DAS_BIND_MANAGED_FIELD(target)>("target");
    addField<DAS_BIND_MANAGED_FIELD(includeFlags)>("includeFlags");
    addField<DAS_BIND_MANAGED_FIELD(excludeFlags)>("excludeFlags");
    addField<DAS_BIND_MANAGED_FIELD(startPoly)>("startPoly");
    addField<DAS_BIND_MANAGED_FIELD(targetPoly)>("targetPoly");
    addField<DAS_BIND_MANAGED_FIELD(extents)>("extents");
    addField<DAS_BIND_MANAGED_FIELD(maxJumpUpHeight)>("maxJumpUpHeight");
    addField<DAS_BIND_MANAGED_FIELD(areasCost)>("areasCost");
  }
};

struct DtPathCorridorAnnotation : das::ManagedStructureAnnotation<dtPathCorridor, false>
{
  DtPathCorridorAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("dtPathCorridor", ml)
  {
    cppName = " ::dtPathCorridor";

    addProperty<DAS_BIND_MANAGED_PROP(getFirstPoly)>("firstPoly", "getFirstPoly");
    addProperty<DAS_BIND_MANAGED_PROP(getLastPoly)>("lastPoly", "getLastPoly");
    addProperty<DAS_BIND_MANAGED_PROP(getPathCount)>("pathCount", "getPathCount");
  }
};

struct NavMeshTriangleAnnotation : das::ManagedStructureAnnotation<pathfinder::NavMeshTriangle, false>
{
  NavMeshTriangleAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("NavMeshTriangle", ml)
  {
    cppName = " pathfinder::NavMeshTriangle";

    addField<DAS_BIND_MANAGED_FIELD(p0)>("p0");
    addField<DAS_BIND_MANAGED_FIELD(p1)>("p1");
    addField<DAS_BIND_MANAGED_FIELD(p2)>("p2");
    addField<DAS_BIND_MANAGED_FIELD(polyRef)>("polyRef");
  }
};

struct FindCornersResultAnnotation : das::ManagedStructureAnnotation<pathfinder::FindCornersResult, false>
{
  FindCornersResultAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("FindCornersResult", ml)
  {
    cppName = " pathfinder::FindCornersResult";

    addField<DAS_BIND_MANAGED_FIELD(cornerFlags)>("cornerFlags");
    addField<DAS_BIND_MANAGED_FIELD(cornerPolys)>("cornerPolys");
  }
};

class PathfinderModule final : public das::Module
{
public:
  PathfinderModule() : das::Module("pathfinder")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs")); // for set_corridor_agent_position
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("DagorMath"));
    G_STATIC_ASSERT(sizeof(dtPolyRef) == sizeof(uint64_t));
    auto pType = das::make_smart<das::TypeDecl>(das::Type::tUInt64);
    pType->alias = "dtPolyRef";
    addAlias(pType);

    addEnumeration(das::make_smart<EnumerationFindPathResult>());
    addEnumeration(das::make_smart<EnumerationPolyFlag>());
    das::addEnumFlagOps<pathfinder::PolyFlag>(*this, lib, "pathfinder::PolyFlag");
    addEnumeration(das::make_smart<EnumerationStraightPathFlags>());
    addAnnotation(das::make_smart<CustomNavAnnotation>(lib));
    addAnnotation(das::make_smart<FindRequestAnnotation>(lib));
    addAnnotation(das::make_smart<CorridorInputAnnotation>(lib));
    addAnnotation(das::make_smart<DtPathCorridorAnnotation>(lib));
    addAnnotation(das::make_smart<NavMeshTriangleAnnotation>(lib));
    addAnnotation(das::make_smart<FindCornersResultAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(find_path_req)>(*this, lib, "find_path", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::find_path_req");
    das::addExtern<DAS_BIND_FUN(find_path_req_custom)>(*this, lib, "find_path", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::find_path_req_custom");

    das::addExtern<DAS_BIND_FUN(find_path)>(*this, lib, "find_path", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::find_path");

    das::addExtern<DAS_BIND_FUN(check_path_req)>(*this, lib, "check_path", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::check_path_req");

    das::addExtern<bool (*)(const Point3 &, const Point3 &, const Point3 &, float, float, const pathfinder::CustomNav *, int, int),
      &pathfinder::check_path>(*this, lib, "check_path", das::SideEffects::accessExternal, "::pathfinder::check_path");

    das::addExtern<float (*)(pathfinder::FindRequest &, float, float), &pathfinder::calc_approx_path_length>(*this, lib,
      "calc_approx_path_length", das::SideEffects::modifyArgumentAndAccessExternal, "::pathfinder::calc_approx_path_length");

    das::addExtern<float (*)(const Point3 &, const Point3 &, const Point3 &, float, float, int, int),
      &pathfinder::calc_approx_path_length>(*this, lib, "calc_approx_path_length", das::SideEffects::accessExternal,
      "::pathfinder::calc_approx_path_length");

    das::addExtern<bool (*)(const Point3 &, const Point3 &, const Point3 &, das::float3 &), &traceray_navmesh>(*this, lib,
      "traceray_navmesh", das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::traceray_navmesh");

    das::addExtern<bool (*)(const Point3 &, const Point3 &, const Point3 &, das::float3 &, pathfinder::CustomNav *),
      &traceray_navmesh>(*this, lib, "traceray_navmesh", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::traceray_navmesh");

    das::addExtern<bool (*)(const Point3 &, const Point3 &, const Point3 &, das::float3 &, dtPolyRef &, pathfinder::CustomNav *),
      &traceray_navmesh>(*this, lib, "traceray_navmesh", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::traceray_navmesh");

    das::addExtern<bool (*)(const Point3 &, const Point3 &, const Point3 &, das::float3 &, dtPolyRef &), &traceray_navmesh>(*this, lib,
      "traceray_navmesh", das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::traceray_navmesh");

    das::addExtern<bool (*)(das::float3 &, float), &project_to_nearest_navmesh_point>(*this, lib, "project_to_nearest_navmesh_point",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::project_to_nearest_navmesh_point");

    das::addExtern<bool (*)(das::float3 &, float, pathfinder::CustomNav *), &project_to_nearest_navmesh_point>(*this, lib,
      "project_to_nearest_navmesh_point", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::project_to_nearest_navmesh_point");

    das::addExtern<DAS_BIND_FUN(project_to_nearest_navmesh_point_3d)>(*this, lib, "project_to_nearest_navmesh_point",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::project_to_nearest_navmesh_point_3d");

    das::addExtern<DAS_BIND_FUN(project_to_nearest_navmesh_point_ref)>(*this, lib, "project_to_nearest_navmesh_point",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::project_to_nearest_navmesh_point_ref");

    using method_drawDebug = DAS_CALL_MEMBER(pathfinder::CustomNav::drawDebug);
    das::addExtern<DAS_CALL_METHOD(method_drawDebug)>(*this, lib, "drawDebug", das::SideEffects::modifyExternal,
      DAS_CALL_MEMBER_CPP(pathfinder::CustomNav::drawDebug));

    das::addExtern<DAS_BIND_FUN(pathfinder::corridor_moveOverOffmeshConnection)>(*this, lib, "corridor_moveOverOffmeshConnection",
      das::SideEffects::modifyArgumentAndAccessExternal, "pathfinder::corridor_moveOverOffmeshConnection");

    das::addExtern<DAS_BIND_FUN(project_to_nearest_navmesh_point_no_obstacles_3d)>(*this, lib,
      "project_to_nearest_navmesh_point_no_obstacles", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::project_to_nearest_navmesh_point_no_obstacles_3d");

    das::addExtern<DAS_BIND_FUN(pathfinder::find_random_point_around_circle)>(*this, lib, "find_random_point_around_circle",
      das::SideEffects::modifyArgument, "pathfinder::find_random_point_around_circle");

    das::addExtern<DAS_BIND_FUN(find_random_points_around_circle)>(*this, lib, "find_random_points_around_circle",
      das::SideEffects::accessExternal, "bind_dascript::find_random_points_around_circle");

    das::addExtern<DAS_BIND_FUN(pathfinder::find_random_point_inside_circle)>(*this, lib, "find_random_point_inside_circle",
      das::SideEffects::modifyArgument, "pathfinder::find_random_point_inside_circle");

    das::addExtern<float (*)(const Point3 &pos, float horz_extents, float search_rad, const pathfinder::CustomNav *custom_nav),
      pathfinder::get_distance_to_wall>(*this, lib, "get_distance_to_wall", das::SideEffects::accessExternal,
      "::pathfinder::get_distance_to_wall")
      ->arg_init(3 /*custom_nav*/, das::make_smart<das::ExprConstPtr>());

    das::addExtern<float (*)(const Point3 &pos, float horz_extents, float search_rad, Point3 &hit_norm,
                     const pathfinder::CustomNav *custom_nav),
      pathfinder::get_distance_to_wall>(*this, lib, "get_distance_to_wall", das::SideEffects::modifyArgument,
      "::pathfinder::get_distance_to_wall")
      ->arg_init(4 /*custom_nav*/, das::make_smart<das::ExprConstPtr>());

    das::addExtern<DAS_BIND_FUN(pathfinder::isLoaded)>(*this, lib, "pathfinder_is_loaded", das::SideEffects::accessExternal,
      "::pathfinder::isLoaded");

    das::addExtern<DAS_BIND_FUN(pathfinder::tilecache_is_loaded)>(*this, lib, "tilecache_is_loaded", das::SideEffects::accessExternal,
      "::pathfinder::tilecache_is_loaded");

    das::addExtern<DAS_BIND_FUN(pathfinder::tilecache_sync)>(*this, lib, "tilecache_sync", das::SideEffects::accessExternal,
      "::pathfinder::tilecache_sync");

    das::addExtern<DAS_BIND_FUN(pathfinder::tilecache_disable_dynamic_jump_links)>(*this, lib, "tilecache_disable_dynamic_jump_links",
      das::SideEffects::modifyExternal, "::pathfinder::tilecache_disable_dynamic_jump_links");

    das::addExtern<DAS_BIND_FUN(pathfinder::tilecache_disable_dynamic_ladder_links)>(*this, lib,
      "tilecache_disable_dynamic_ladder_links", das::SideEffects::modifyExternal,
      "::pathfinder::tilecache_disable_dynamic_ladder_links");

    das::addExtern<DAS_BIND_FUN(pathfinder::tilecache_is_blocking)>(*this, lib, "tilecache_is_blocking",
      das::SideEffects::accessExternal, "::pathfinder::tilecache_is_blocking");
    das::addExtern<DAS_BIND_FUN(pathfinder::tilecache_is_working)>(*this, lib, "tilecache_is_working",
      das::SideEffects::accessExternal, "::pathfinder::tilecache_is_working");

    das::addExtern<DAS_BIND_FUN(tilecache_obstacle_add)>(*this, lib, "tilecache_obstacle_add", das::SideEffects::modifyExternal,
      "bind_dascript::tilecache_obstacle_add");
    das::addExtern<DAS_BIND_FUN(tilecache_obstacle_add_with_type)>(*this, lib, "tilecache_obstacle_add",
      das::SideEffects::modifyExternal, "bind_dascript::tilecache_obstacle_add_with_type");

    das::addExtern<DAS_BIND_FUN(pathfinder::mark_polygons_lower)>(*this, lib, "mark_polygons_lower", das::SideEffects::modifyExternal,
      "pathfinder::mark_polygons_lower");

    das::addExtern<DAS_BIND_FUN(pathfinder::mark_polygons_upper)>(*this, lib, "mark_polygons_upper", das::SideEffects::modifyExternal,
      "pathfinder::mark_polygons_upper");

    das::addExtern<DAS_BIND_FUN(change_navpolys_flags_in_box)>(*this, lib, "change_navpolys_flags_in_box",
      das::SideEffects::modifyExternal, "bind_dascript::change_navpolys_flags_in_box");

    das::addExtern<DAS_BIND_FUN(change_navpolys_flags_in_box_no_area_cb)>(*this, lib, "change_navpolys_flags_in_box",
      das::SideEffects::modifyExternal, "bind_dascript::change_navpolys_flags_in_box_no_area_cb");

    das::addExtern<DAS_BIND_FUN(change_navpolys_flags_in_poly)>(*this, lib, "change_navpolys_flags_in_poly",
      das::SideEffects::modifyExternal, "bind_dascript::change_navpolys_flags_in_poly");

    das::addExtern<DAS_BIND_FUN(change_navpolys_flags_in_poly_no_area_cb)>(*this, lib, "change_navpolys_flags_in_poly",
      das::SideEffects::modifyExternal, "bind_dascript::change_navpolys_flags_in_poly_no_area_cb");

    das::addExtern<DAS_BIND_FUN(pathfinder::change_navpolys_flags_all)>(*this, lib, "change_navpolys_flags_all",
      das::SideEffects::modifyExternal, "pathfinder::change_navpolys_flags_all");

    das::addExtern<DAS_BIND_FUN(pathfinder::is_loaded_ex)>(*this, lib, "pathfinder_is_loaded_ex", das::SideEffects::accessExternal,
      "::pathfinder::is_loaded_ex");

    das::addExtern<DAS_BIND_FUN(find_path_ex)>(*this, lib, "find_path_ex", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::find_path_ex");

    das::addExtern<DAS_BIND_FUN(find_path_ex_req)>(*this, lib, "find_path_ex", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::find_path_ex_req");

    das::addExtern<DAS_BIND_FUN(check_path_ex_req)>(*this, lib, "check_path_ex", das::SideEffects::modifyArgumentAndAccessExternal,
      "bind_dascript::check_path_ex_req");

    das::addExtern<bool (*)(int, const Point3 &, const Point3 &, const Point3 &, float, float, const pathfinder::CustomNav *, int,
                     int),
      &pathfinder::check_path_ex>(*this, lib, "check_path_ex", das::SideEffects::accessExternal, "::pathfinder::check_path_ex");

    das::addExtern<DAS_BIND_FUN(pathfinder::get_triangle_by_pos_ex)>(*this, lib, "get_triangle_by_pos_ex",
      das::SideEffects::modifyArgumentAndAccessExternal, "pathfinder::get_triangle_by_pos_ex");

    das::addExtern<DAS_BIND_FUN(pathfinder::set_poly_flags_ex)>(*this, lib, "set_poly_flags_ex", das::SideEffects::modifyExternal,
      "pathfinder::set_poly_flags_ex");

    das::addExtern<DAS_BIND_FUN(pathfinder::get_poly_flags_ex)>(*this, lib, "get_poly_flags_ex", das::SideEffects::modifyArgument,
      "pathfinder::get_poly_flags_ex");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_init)>(*this, lib, "rebuildNavMesh_init", das::SideEffects::modifyExternal,
      "pathfinder::rebuildNavMesh_init");

    das::addExtern<void (*)(const char *, float), &pathfinder::rebuildNavMesh_setup>(*this, lib, "rebuildNavMesh_setup",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_setup");

    das::addExtern<void (*)(const char *, const Point2 &), &pathfinder::rebuildNavMesh_setup>(*this, lib, "rebuildNavMesh_setup",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_setup");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_addBBox)>(*this, lib, "rebuildNavMesh_addBBox",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_addBBox");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_update)>(*this, lib, "rebuildNavMesh_update",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_update");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_getProgress)>(*this, lib, "rebuildNavMesh_getProgress",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_getProgress");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_getTotalTiles)>(*this, lib, "rebuildNavMesh_getTotalTiles",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_getTotalTiles");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_saveToFile)>(*this, lib, "rebuildNavMesh_saveToFile",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_saveToFile");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_close)>(*this, lib, "rebuildNavMesh_close",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_close");

    das::addExtern<DAS_BIND_FUN(tilecache_obstacle_move)>(*this, lib, "tilecache_obstacle_move", das::SideEffects::modifyExternal,
      "bind_dascript::tilecache_obstacle_move");
    das::addExtern<DAS_BIND_FUN(tilecache_obstacle_move_with_type)>(*this, lib, "tilecache_obstacle_move",
      das::SideEffects::modifyExternal, "bind_dascript::tilecache_obstacle_move_with_type");

    das::addExtern<DAS_BIND_FUN(tilecache_obstacle_remove)>(*this, lib, "tilecache_obstacle_remove", das::SideEffects::modifyExternal,
      "bind_dascript::tilecache_obstacle_remove");

    das::addExtern<DAS_BIND_FUN(tilecache_ri_obstacle_add)>(*this, lib, "tilecache_ri_obstacle_add", das::SideEffects::modifyExternal,
      "bind_dascript::tilecache_ri_obstacle_add");

    das::addExtern<DAS_BIND_FUN(tilecache_ri_obstacle_remove)>(*this, lib, "tilecache_ri_obstacle_remove",
      das::SideEffects::modifyExternal, "bind_dascript::tilecache_ri_obstacle_remove");

    das::addExtern<DAS_BIND_FUN(query_navmesh_projections)>(*this, lib, "query_navmesh_projections", das::SideEffects::accessExternal,
      "bind_dascript::query_navmesh_projections");

    das::addExtern<DAS_BIND_FUN(query_navmesh_projections_with_polys)>(*this, lib, "query_navmesh_projections",
      das::SideEffects::accessExternal, "bind_dascript::query_navmesh_projections_with_polys");

    das::addExtern<DAS_BIND_FUN(pathfinder::init_path_corridor)>(*this, lib, "init_path_corridor",
      das::SideEffects::modifyArgumentAndExternal, "pathfinder::init_path_corridor");

    das::addExtern<DAS_BIND_FUN(pathfinder::set_path_corridor)>(*this, lib, "set_path_corridor",
      das::SideEffects::modifyArgumentAndExternal, "pathfinder::set_path_corridor");

    das::addExtern<DAS_BIND_FUN(pathfinder::set_curved_path_corridor)>(*this, lib, "set_curved_path_corridor",
      das::SideEffects::modifyArgumentAndExternal, "pathfinder::set_curved_path_corridor");

    das::addExtern<DAS_BIND_FUN(das_find_corridor_corners)>(*this, lib, "find_corridor_corners",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::das_find_corridor_corners");

    das::addExtern<DAS_BIND_FUN(das_find_corridor_corners_result)>(*this, lib, "find_corridor_corners",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::das_find_corridor_corners_result");

    das::addExtern<DAS_BIND_FUN(das_get_corridor_pos)>(*this, lib, "corridor_getPos", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::das_get_corridor_pos");

    das::addExtern<DAS_BIND_FUN(das_get_corridor_target)>(*this, lib, "corridor_getTarget",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::das_get_corridor_target");

    das::addExtern<DAS_BIND_FUN(corridor_get_path)>(*this, lib, "corridor_getPath", das::SideEffects::accessExternal,
      "bind_dascript::corridor_get_path");

    das::addExtern<DAS_BIND_FUN(bind_dascript::using_find_request)>(*this, lib, "using", das::SideEffects::invoke,
      "bind_dascript::using_find_request");

    das::addExtern<DAS_BIND_FUN(bind_dascript::using_corridor_input)>(*this, lib, "using", das::SideEffects::invoke,
      "bind_dascript::using_corridor_input");

    das::addExtern<DAS_BIND_FUN(bind_dascript::set_corridor_agent_position)>(*this, lib, "set_corridor_agent_position",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::set_corridor_agent_position");

    das::addExtern<DAS_BIND_FUN(bind_dascript::set_corridor_target)>(*this, lib, "set_corridor_target",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::set_corridor_target");

    das::addExtern<bool (*)(dtPathCorridor &, const pathfinder::FindRequest &, bool, const pathfinder::CustomNav *),
      &pathfinder::optimize_corridor_path>(*this, lib, "optimize_corridor_path", das::SideEffects::modifyArgumentAndExternal,
      "::pathfinder::optimize_corridor_path");

    das::addExtern<bool (*)(dtPathCorridor &, const Point3 &, bool, const pathfinder::CustomNav *),
      &pathfinder::optimize_corridor_path>(*this, lib, "optimize_corridor_path", das::SideEffects::modifyArgumentAndExternal,
      "::pathfinder::optimize_corridor_path");

    das::addExtern<DAS_BIND_FUN(pathfinder::set_poly_flags)>(*this, lib, "set_poly_flags", das::SideEffects::modifyExternal,
      "pathfinder::set_poly_flags");

    das::addExtern<DAS_BIND_FUN(pathfinder::get_poly_flags)>(*this, lib, "get_poly_flags", das::SideEffects::modifyArgument,
      "pathfinder::get_poly_flags");

    das::addExtern<DAS_BIND_FUN(pathfinder::get_poly_area)>(*this, lib, "get_poly_area", das::SideEffects::modifyArgument,
      "pathfinder::get_poly_area");

    das::addExtern<DAS_BIND_FUN(pathfinder::set_poly_area)>(*this, lib, "set_poly_area", das::SideEffects::modifyExternal,
      "pathfinder::set_poly_area");

    das::addExtern<DAS_BIND_FUN(das_move_over_offmesh_link_in_corridor)>(*this, lib, "move_over_offmesh_link_in_corridor",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::das_move_over_offmesh_link_in_corridor");

    das::addExtern<DAS_BIND_FUN(pathfinder::is_corridor_valid)>(*this, lib, "is_corridor_valid",
      das::SideEffects::modifyArgumentAndExternal, "pathfinder::is_corridor_valid");

    das::addExtern<DAS_BIND_FUN(pathfinder::draw_corridor_path)>(*this, lib, "draw_corridor_path", das::SideEffects::accessExternal,
      "pathfinder::draw_corridor_path");

    das::addExtern<DAS_BIND_FUN(pathfinder::move_along_surface)>(*this, lib, "move_along_surface",
      das::SideEffects::modifyArgumentAndExternal, "pathfinder::move_along_surface");

    das::addExtern<DAS_BIND_FUN(pathfinder::get_triangle_by_pos)>(*this, lib, "get_triangle_by_pos",
      das::SideEffects::modifyArgumentAndAccessExternal, "pathfinder::get_triangle_by_pos");

    das::addExtern<DAS_BIND_FUN(pathfinder::find_nearest_triangle_by_pos)>(*this, lib, "find_nearest_triangle_by_pos",
      das::SideEffects::modifyArgumentAndAccessExternal, "pathfinder::find_nearest_triangle_by_pos");

    das::addExtern<DAS_BIND_FUN(pathfinder::navmesh_is_valid_poly_ref)>(*this, lib, "navmesh_is_valid_poly_ref",
      das::SideEffects::accessExternal, "pathfinder::navmesh_is_valid_poly_ref");

    das::addExtern<DAS_BIND_FUN(pathfinder::is_valid_poly_ref)>(*this, lib, "is_valid_poly_ref", das::SideEffects::accessExternal,
      "pathfinder::is_valid_poly_ref");

    das::addExtern<bool (*)(const Point3 &pos, const uint64_t poly_id, pathfinder::NavMeshTriangle &result),
      &pathfinder::get_triangle_by_poly>(*this, lib, "get_triangle_by_poly", das::SideEffects::modifyArgumentAndExternal,
      "::pathfinder::get_triangle_by_poly");

    das::addExtern<bool (*)(const uint64_t poly_id, pathfinder::NavMeshTriangle &result), &pathfinder::get_triangle_by_poly>(*this,
      lib, "get_triangle_by_poly", das::SideEffects::modifyArgumentAndExternal, "::pathfinder::get_triangle_by_poly");

    das::addExtern<DAS_BIND_FUN(pathfinder::squash_jumplinks)>(*this, lib, "squash_jumplinks", das::SideEffects::modifyExternal,
      "pathfinder::squash_jumplinks");

    das::addExtern<DAS_BIND_FUN(bind_dascript::find_polys_in_circle)>(*this, lib, "find_polys_in_circle",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::find_polys_in_circle");

    das::addExtern<DAS_BIND_FUN(bind_dascript::corridor_input_add_area_cost)>(*this, lib, "add_area_cost",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::corridor_input_add_area_cost");

    das::addExtern<DAS_BIND_FUN(bind_dascript::find_request_add_area_cost)>(*this, lib, "add_area_cost",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::find_request_add_area_cost");

    das::addExtern<DAS_BIND_FUN(bind_dascript::for_each_linked_poly)>(*this, lib, "for_each_linked_poly",
      das::SideEffects::accessExternal, "bind_dascript::for_each_linked_poly");

    das::addExtern<DAS_BIND_FUN(bind_dascript::for_each_nmesh_poly)>(*this, lib, "for_each_nmesh_poly",
      das::SideEffects::accessExternal, "bind_dascript::for_each_nmesh_poly");

    das::addUsing<dtPathCorridor>(*this, lib, " ::dtPathCorridor");

    das::addConstant(*this, "POLYAREA_UNWALKABLE", (int)pathfinder::POLYAREA_UNWALKABLE);
    das::addConstant(*this, "POLYAREA_GROUND", (int)pathfinder::POLYAREA_GROUND);
    das::addConstant(*this, "POLYAREA_OBSTACLE", (int)pathfinder::POLYAREA_OBSTACLE);
    das::addConstant(*this, "POLYAREA_BRIDGE", (int)pathfinder::POLYAREA_BRIDGE);
    das::addConstant(*this, "POLYAREA_BLOCKED", (int)pathfinder::POLYAREA_BLOCKED);
    das::addConstant(*this, "POLYAREA_JUMP", (int)pathfinder::POLYAREA_JUMP);
    das::addConstant(*this, "POLYAREA_WALKABLE", 63); // @see DT_TILECACHE_WALKABLE_AREA

    das::addConstant(*this, "NM_MAIN", (int)pathfinder::NM_MAIN);
    das::addConstant(*this, "NM_EXT_1", (int)pathfinder::NM_EXT_1);
    das::addConstant(*this, "NMS_COUNT", (int)pathfinder::NMS_COUNT);

    compileBuiltinModule("pathFinder.das", (unsigned char *)pathFinder_das, sizeof(pathFinder_das));
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotPathFinder.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(PathfinderModule, bind_dascript);
