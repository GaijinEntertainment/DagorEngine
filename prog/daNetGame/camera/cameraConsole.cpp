// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <util/dag_string.h>
#include "math/dag_mathUtils.h"
#include <render/cameraPositions.h>
#include <3d/dag_render.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_clipboard.h>
#include <ecs/core/entityManager.h>
#include "camera/sceneCam.h"
#include "game/player.h"

using namespace console;

static bool camera_console_handler(const char *argv[], int argc) // move to other file!
{
  if (argc < 1)
    return false;
  int found = 0;

  CONSOLE_CHECK_NAME("camera", "save_pos_of_linear_camera_track", 1, 1)
  {
    const char *res = "";

    static Point3 pos1 = Point3(0, 0, 0);
    static Point3 dir1 = Point3(1, 0, 0);
    static float fov1 = 80.0;
    static bool firstSaved = false;

    ecs::EntityId camEid = get_cur_cam_entity();
    const TMatrix &camTm = get_cam_itm();
    if (!firstSaved)
    {
      fov1 = g_entity_mgr->getOr(camEid, ECS_HASH("fov"), 80.f);
      pos1 = camTm.getcol(3);
      dir1 = camTm.getcol(2);
      firstSaved = true;
      res = "linear_camera: saved 'from'";
    }
    else
    {
      float fov2 = g_entity_mgr->getOr(camEid, ECS_HASH("fov"), 80.f);
      Point3 pos2 = camTm.getcol(3);
      Point3 dir2 = camTm.getcol(2);

      debug("linear_camera::  "
            "track { duration:r=5.0; from_pos:p3=%f,%f,%f; from_dir:p3=%f,%f,%f; from_fov:r=%g; "
            "to_pos:p3=%f,%f,%f; to_dir:p3=%f,%f,%f; to_fov:r=%g; }",
        pos1.x, pos1.y, pos1.z, dir1.x, dir1.y, dir1.z, fov1, pos2.x, pos2.y, pos2.z, dir2.x, dir2.y, dir2.z, fov2);

      debug("\nlinear_camera_script::  "
            "  { duration=5.0, from_pos=Point3(%f,%f,%f), from_dir=Point3(%f,%f,%f), from_fov=%g, "
            "to_pos=Point3(%f,%f,%f), to_dir=Point3(%f,%f,%f), to_fov=%g },",
        pos1.x, pos1.y, pos1.z, dir1.x, dir1.y, dir1.z, fov1, pos2.x, pos2.y, pos2.z, dir2.x, dir2.y, dir2.z, fov2);

      firstSaved = false;
      res = "linear_camera: saved 'to', track written to debug";
    }
    console::print(res);
  }
  CONSOLE_CHECK_NAME("camera", "pos", 1, 4)
  {
    TMatrix itm = get_cam_itm();
    static Point3 lastPos = itm.getcol(3);
    Point3 pos = itm.getcol(3);
    if (argc == 1)
    {
      console::print_d("Camera pos %g %g %g, moved %g", pos.x, pos.y, pos.z, length(lastPos - pos));
      String str(128, "%g, %g, %g", pos.x, pos.y, pos.z);
      clipboard::set_clipboard_ansi_text(str.str());
    }
    lastPos = pos;
    if (argc == 2)
      itm.setcol(3, pos.x, atof(argv[1]), pos.z);
    if (argc == 3)
      itm.setcol(3, atof(argv[1]), pos.y, atof(argv[2]));
    if (argc == 4)
      itm.setcol(3, atof(argv[1]), atof(argv[2]), atof(argv[3]));

    if (argc > 1)
      set_cam_itm(itm);
  }
  CONSOLE_CHECK_NAME("camera", "free", 1, 2)
  {
    if (argc == 1)
      toggle_free_camera();
    else if (to_bool(argv[1]))
      enable_free_camera();
    else
      disable_free_camera();
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(camera_console_handler);
