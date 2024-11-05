// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqModules/sqModules.h>

#include <sqrat.h>

#include <pathFinder/pathFinder.h>
#include <math/dag_Point3.h>
#include <memory/dag_framemem.h>

namespace bindquirrel
{

inline Point3 sq_project_to_nearest_navmesh_point(const Point3 &wish_pos, float horz_extents)
{
  Point3 pos(wish_pos);
  pathfinder::project_to_nearest_navmesh_point(pos, horz_extents);
  return pos;
}

inline Point3 sq_project_to_nearest_navmesh_point_no_obstacles(const Point3 &wish_pos, const Point3 &horz_extents)
{
  Point3 pos(wish_pos);
  pathfinder::project_to_nearest_navmesh_point_no_obstacles(pos, horz_extents);
  return pos;
}

inline Point3 sq_traceray_navmesh(const Point3 &start_pos, const Point3 &end_pos, float dist_to_path)
{
  Point3 pos(end_pos);
  pathfinder::traceray_navmesh(start_pos, end_pos, Point3(dist_to_path, FLT_MAX, dist_to_path), pos);
  return pos;
}

inline SQInteger sq_find_path(HSQUIRRELVM vm)
{
  HSQOBJECT hObjFrom, hObjTo;
  sq_getstackobj(vm, 2, &hObjFrom);
  sq_getstackobj(vm, 3, &hObjTo);
  if (!Sqrat::ClassType<Point3>::IsObjectOfClass(&hObjFrom) || !Sqrat::ClassType<Point3>::IsObjectOfClass(&hObjTo))
    return sq_throwerror(vm, "Source from and to must be Point3 instances");

  Sqrat::Var<const Point3 *> varFrom(vm, 2);
  Sqrat::Var<const Point3 *> varTo(vm, 3);

  float distToPath;
  float stepSize;
  float slop;
  sq_getfloat(vm, 4, &distToPath);
  sq_getfloat(vm, 5, &stepSize);
  sq_getfloat(vm, 6, &slop);

  Tab<Point3> path(framemem_ptr());
  if (pathfinder::findPath(*varFrom.value, *varTo.value, path, distToPath, stepSize, slop) > pathfinder::FPR_FAILED)
  {
    Sqrat::Array res(vm, path.size());
    for (int i = 0; i < path.size(); ++i)
      res.SetValue(SQInteger(i), path[i]);
    Sqrat::Var<Sqrat::Array>::push(vm, res);
    return 1;
  }

  sq_pushnull(vm);
  return 1;
}

bool sq_check_path(const Point3 &start_pos, const Point3 &end_pos, float dist_to_path, float horz_threshold, float max_vert_dist,
  int flags = POLYFLAGS_WALK | pathfinder::POLYFLAG_JUMP)
{
  const Point3 extents = Point3(dist_to_path, FLT_MAX, dist_to_path);
  return pathfinder::check_path(start_pos, end_pos, extents, horz_threshold, max_vert_dist, nullptr, flags);
}

void sqrat_bind_path_finder(SqModules *module_mgr)
{
  G_ASSERT(module_mgr);
  HSQUIRRELVM vm = module_mgr->getVM();

  Sqrat::Table nsTbl(vm);
  ///@module pathfinder
  nsTbl //
    .Func("project_to_nearest_navmesh_point", sq_project_to_nearest_navmesh_point)
    .Func("project_to_nearest_navmesh_point_no_obstacles", sq_project_to_nearest_navmesh_point_no_obstacles)
    .Func("check_path", sq_check_path)
    .SquirrelFunc("find_path", sq_find_path, -5, ".xxfff")
    .Func("traceray_navmesh", sq_traceray_navmesh)
    .SetValue("POLYFLAG_GROUND", pathfinder::POLYFLAG_GROUND)
    .SetValue("POLYFLAG_OBSTACLE", pathfinder::POLYFLAG_OBSTACLE)
    .SetValue("POLYFLAG_LADDER", pathfinder::POLYFLAG_LADDER)
    .SetValue("POLYFLAG_JUMP", pathfinder::POLYFLAG_JUMP)
    .SetValue("POLYFLAG_BLOCKED", pathfinder::POLYFLAG_BLOCKED)
    /**/;

  module_mgr->addNativeModule("pathfinder", nsTbl);
}

} // namespace bindquirrel
