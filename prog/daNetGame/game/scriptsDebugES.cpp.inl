// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <util/dag_string.h>
#include <debug/dag_textMarks.h>

namespace bind_dascript
{
extern int get_das_compile_errors_count();
}

ECS_TAG(dasDebug)
ECS_NO_ORDER
static void display_das_errors_es(const ecs::UpdateStageInfoAct &info)
{
  // This will display errors only when ALL scripts are reloaded. (i.e. on the game startup)
  // Hot-reload will always set it to 0.
  int count = bind_dascript::get_das_compile_errors_count();
  if (count > 0)
  {
    String msg = String(0, "das [E]: %d", count);
    add_debug_text_mark(35, 120, msg, msg.length(), 0, E3DCOLOR(255, ((int)info.curTime * 100) % 256, 100));
  }
}