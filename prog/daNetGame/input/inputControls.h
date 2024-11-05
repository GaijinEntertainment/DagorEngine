// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <squirrel.h>
#include <json/json.h>

class DataBlock;

namespace dainput
{
class HidChainModule;
};

struct InputRegRecord
{
  InputRegRecord *next = nullptr;
  dainput::HidChainModule *(*func)();
};

namespace uiinput
{
class MouseKbdHandler;
};

namespace darg
{
struct IGuiScene;
};

class SqModules;

namespace controls
{
void global_init();
void global_destroy();
void init_drivers();
void init_control(const char *game, const char *user_game_mode_input_cfg_fn = nullptr);
void destroy();

void process_input(double dt);
void bind_script_api(SqModules *moduleMgr);

struct SensScale
{
  float humanAiming = 0.65;
  float humanTpsCam = 1.0f;
  float humanFpsCam = 0.65;
  float vehicleCam = 1.0;
  float planeCam = 1.0;

  void writeTo(DataBlock &blk);
  void readFrom(const DataBlock &blk);
};
extern SensScale sens_scale;
} // namespace controls

void pull_input_das();
