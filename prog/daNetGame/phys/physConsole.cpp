// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <gamePhys/collision/collisionLib.h>
#include <ecs/core/entityManager.h>

using namespace console;
static bool phys_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("phys", "force_debug_draw", 1, 1) { dacoll::force_debug_draw(!dacoll::is_debug_draw_forced()); }
  return found;
}

REGISTER_CONSOLE_HANDLER(phys_console_handler);
