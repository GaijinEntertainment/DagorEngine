// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <ecs/anim/anim.h>
#include <ecs/core/entityManager.h>
#include <osApiWrappers/dag_clipboard.h>

#include "game/player.h"
#include "camera/sceneCam.h"

template <typename Callable>
inline void set_irq_pos_ecs_query(Callable c);

using namespace console;
static bool anim_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("anim", "dump_anim_state", 1, 3)
  {
    ecs::EntityId targetEid = argc > 2 ? ecs::EntityId(console::to_int(argv[2])) : game::get_controlled_hero();
    auto *animChar = g_entity_mgr->getNullableRW<AnimV20::AnimcharBaseComponent>(targetEid, ECS_HASH("animchar"));
    const bool json_format = argc > 1 && console::to_bool(argv[1]);
    if (animChar)
      anim::dump_animchar_state(*animChar, json_format);
  }
  CONSOLE_CHECK_NAME("anim", "node_offs", 2, 10)
  {
    auto *animChar = g_entity_mgr->getNullableRW<AnimV20::AnimcharBaseComponent>(game::get_controlled_hero(), ECS_HASH("animchar"));
    if (animChar)
    {
      String nodeName(128, argv[1]);
      for (int i = 2; i < argc; ++i)
        nodeName.aprintf(128, " %s", argv[i]);
      if (auto node = animChar->getNodeTree().findNodeIndex(nodeName.str()))
      {
        TMatrix tm;
        animChar->getNodeTree().getNodeWtmScalar(node, tm);
        TMatrix itm = inverse(tm);
        Point3 offs = itm * get_cam_itm().getcol(3);
        console::print_d("Offs pos %g %g %g", offs.x, offs.y, offs.z);
        String str(128, "%g, %g, %g", offs.x, offs.y, offs.z);
        clipboard::set_clipboard_ansi_text(str.str());
      }
    }
  }
  CONSOLE_CHECK_NAME("anim", "set_irq_pos", 3, 3)
  {
    const char *name = argv[1];
    float relPos = console::to_real(argv[2]);
    set_irq_pos_ecs_query([&](AnimV20::AnimcharBaseComponent &animchar) {
      if (auto ag = animchar.getAnimGraph())
        ag->debugSetIrqPosInAllNodes(name, relPos);
    });
  }
  CONSOLE_CHECK_NAME("anim", "debug_anim_param", 1, 2)
  {
    const bool enable = argc > 1 ? console::to_bool(argv[1]) : !AnimV20::getDebugAnimParam();
    AnimV20::setDebugAnimParam(enable);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(anim_console_handler);
