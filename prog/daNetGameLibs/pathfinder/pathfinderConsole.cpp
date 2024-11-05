// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <pathFinder/pathFinder.h>
#include <camera/sceneCam.h>
#include <util/dag_console.h>

static bool pathfinder_console_handler(const char *argv[], int argc)
{
  using namespace console;
  int found = 0;
  CONSOLE_CHECK_NAME("path", "raycast", 1, 1)
  {
    static bool initial = true;
    static Point3 startPos;
    if (initial)
      startPos = get_cam_itm().getcol(3);
    else
    {
      Point3 resPos;
      pathfinder::traceray_navmesh(startPos, get_cam_itm().getcol(3), Point3(0.5f, FLT_MAX, 0.5f), resPos);
      Tab<Point3> path(framemem_ptr());
      path.push_back() = startPos;
      path.push_back() = resPos;
      pathfinder::setPathForDebug(path);
    }
    initial = !initial;
  }
  CONSOLE_CHECK_NAME("path", "find", 1, 4)
  {
    static bool initial = true;
    static Point3 startPos;
    if (initial)
      startPos = get_cam_itm().getcol(3);
    else
    {
      float dist = argc > 1 ? to_real(argv[1]) : 0.5f;
      float step = argc > 2 ? to_real(argv[2]) : 1.f;
      float slop = argc > 3 ? to_real(argv[3]) : 0.25f;
      Tab<Point3> path(framemem_ptr());
      pathfinder::findPath(startPos, get_cam_itm().getcol(3), path, dist, step, slop);
      pathfinder::setPathForDebug(path);
    }
    initial = !initial;
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(pathfinder_console_handler);

#include <daECS/core/componentType.h>
ECS_DEF_PULL_VAR(dng_pathfinder_con);
