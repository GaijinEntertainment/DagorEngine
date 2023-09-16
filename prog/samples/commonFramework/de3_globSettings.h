#pragma once

#include <startup/dag_globalSettings.h>
#include <startup/dag_loadSettings.h>
#include <ioSys/dag_dataBlock.h>

#define DE3_LOAD_GLOBAL_SETTINGS(S_NM, GP_NM) \
  dgs_load_settings_blk(true, S_NM);          \
  dgs_load_game_params_blk(GP_NM);
