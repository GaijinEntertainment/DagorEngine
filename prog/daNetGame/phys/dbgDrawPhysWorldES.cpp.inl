// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/render/updateStageRender.h>
#include <phys/dag_physDebug.h>
#include <util/dag_convar.h>
#include <util/dag_console.h>
#include <gamePhys/collision/collisionLib.h>

#if DAGOR_DBGLEVEL > 0
static CONSOLE_BOOL_VAL("physWorld", debug_draw, false);
static CONSOLE_BOOL_VAL("physWorld", draw_ztest, false);
static CONSOLE_FLOAT_VAL("physWorld", draw_range, 200.f);

static physdbg::RenderFlags rflg = physdbg::RenderFlag::BODIES | physdbg::RenderFlag::CONSTRAINTS |
                                   physdbg::RenderFlag::CONSTRAINT_LIMITS | physdbg::RenderFlag::CONSTRAINT_REFSYS |
                                   /*physdbg::RenderFlag::BODY_BBOX|physdbg::RenderFlag::BODY_CENTER|*/
                                   physdbg::RenderFlag::CONTACT_POINTS;

#endif

ECS_NO_ORDER
ECS_TAG(render, dev)
static __forceinline void physdbg_render_world_es(const UpdateStageInfoRenderDebug &info)
{
#if DAGOR_DBGLEVEL > 0
  if (!debug_draw)
    return;
  if (draw_ztest)
    rflg |= physdbg::RenderFlag::USE_ZTEST;
  Point3 camPos = info.viewItm.getcol(3);
  Frustum frustum(info.globtm);
  physdbg::renderWorld(dacoll::get_phys_world(), rflg, &camPos, draw_range, &frustum);
#endif
  G_UNUSED(info);
}

static bool physdbg_console_handler(const char *argv[], int argc)
{
  int found = 0;
#if DAGOR_DBGLEVEL > 0
  CONSOLE_CHECK_NAME("physWorld", "renderFlags", 1, 2)
  {
    if (argc == 2)
    {
      rflg = make_bitmask((physdbg::RenderFlag)atoi(argv[1]));
      console::print_d("Phys world render flags set to %d", rflg.asInteger());
    }
    else
    {
      console::print_d("Current phys world render flags: %d", rflg.asInteger());
    }
  }
#endif
  G_UNUSED(argv);
  G_UNUSED(argc);
  return found;
}

REGISTER_CONSOLE_HANDLER(physdbg_console_handler);
