#include "gamepad_classdrv.h"
#include <humanInput/dag_hiCreate.h>

using namespace HumanInput;

// for correct navigation in gui and showing buttons hints setup following console variables
// show_console_buttons = true
// always_reload_scenes = true
// implement it for a big picture mode if controller is plugged
// steam does not loads controller.vdf if the platform (linux, steams) are not indicated as supported in the steamworks
// but for testing purpose you may editing user file located in
// home/<user>/.local/share/Steam/userData/<userId>/config/controller_configs

IGenJoystickClassDrv *HumanInput::createSteamJoystickClassDriver(const char *absolute_path_to_controller_config)
{
  SteamGamepadClassDriver *cd = new (inimem) SteamGamepadClassDriver;

  if (!cd->init(absolute_path_to_controller_config))
  {
    delete cd;
    cd = NULL;
  }

  return cd;
}
