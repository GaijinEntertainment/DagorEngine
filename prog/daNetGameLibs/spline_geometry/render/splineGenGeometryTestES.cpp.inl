// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/updateStage.h>
#include <math/dag_TMatrix.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <camera/sceneCam.h>

CONSOLE_BOOL_VAL("spline_gen", test_active, true);

ECS_TAG(render, dev)
ECS_NO_ORDER
static void spline_gen_test_make_points_es(const ecs::UpdateStageInfoAct & /*stg*/,
  const TMatrix &transform,
  ecs::List<Point3> &spline_gen_geometry__points,
  bool &spline_gen_geometry__request_active ECS_REQUIRE(ecs::Tag splineGenTest))
{
  spline_gen_geometry__request_active = test_active;
  // if spline_gen_geometry__points is empty the entity was created this frame with test_active = false
  if (spline_gen_geometry__request_active || spline_gen_geometry__points.empty())
  {
    spline_gen_geometry__points.clear();
    Point3 start = transform.getcol(3);
    Point3 end = start + transform.getcol(2);
    spline_gen_geometry__points.push_back(start);
    spline_gen_geometry__points.push_back(end);
  }
}

static bool spline_gen_test_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("spline_gen", "spawn_test", 2, 2)
  {
    ecs::EntityId camEid = get_cur_cam_entity();
    TMatrix camTm = g_entity_mgr->getOr(camEid, ECS_HASH("transform"), TMatrix::IDENT);
    const char *templateName = argv[1];
    ecs::ComponentsInitializer attrs;
    attrs[ECS_HASH("transform")] = camTm;
    g_entity_mgr->createEntityAsync(templateName, eastl::move(attrs));
  }
  CONSOLE_CHECK_NAME("spline_gen", "spawn_test_grid", 2, 4)
  {
    ecs::EntityId camEid = get_cur_cam_entity();
    TMatrix camTm = g_entity_mgr->getOr(camEid, ECS_HASH("transform"), TMatrix::IDENT);
    const char *templateName = argv[1];
    int width = argc > 2 ? max(console::to_int(argv[2]), 1) : 10;
    int height = argc > 3 ? max(console::to_int(argv[3]), 1) : width;

    for (int i = width / 2 - width; i < width / 2; i++)
    {
      for (int j = height / 2 - height; j < height / 2; j++)
      {
        TMatrix tm;
        tm.setcol(0, Point3(1, 0, 0));
        tm.setcol(1, Point3(0, 0, 1));
        tm.setcol(2, Point3(0, 1, 0));
        tm.setcol(3, camTm.getcol(3) + Point3(0.5 * i, 0, 0.5 * j));
        ecs::ComponentsInitializer attrs;
        attrs[ECS_HASH("transform")] = tm;
        g_entity_mgr->createEntityAsync(templateName, eastl::move(attrs));
      }
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(spline_gen_test_console_handler);