// ATTENTION!
// this file is coupling things to much! Split it!
// shouldDecreaseSize, allowedSizeIncrease = 25

#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <util/dag_console.h>
#include "main/ecsUtils.h"
#include <camTrack/camTrack.h>
#include "camera/camTrack.h"
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <math/dag_geomTree.h>
#include "game/player.h"
#include "main/editMode.h"
#include "camera/sceneCam.h"
#include "input/inputControls.h"
#include <gamePhys/collision/collisionLib.h>
#include "render/renderer.h"
#include <math/random/dag_random.h>

// before adding something here consider to register console command from game/ui script
// as most of actions should and already are bound to game/ui scripts.
// Do not do binding twice, Do not repeat yoursef.
// TODO:
//  * move input to script or to inputConsole.cpp
//  * move path to script or to pathConsole.cpp

using namespace console;
static bool game_console_handler(const char *argv[], int argc) // move it somewhere else
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("camera", "record_track", 2, 2) { camtrack::record(argv[1]); }
  CONSOLE_CHECK_NAME("camera", "stop_record", 1, 1) { camtrack::stop_record(); }
  CONSOLE_CHECK_NAME("camera", "play_record", 2, 2) { camtrack::play(argv[1]); }
  CONSOLE_CHECK_NAME("game", "edit_mode", 1, 1) { editmode::toggle(); }
  return found;
}

REGISTER_CONSOLE_HANDLER(game_console_handler);
