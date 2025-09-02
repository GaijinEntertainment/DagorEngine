// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <sqModules/sqModules.h>

class DataBlock;

#define DEF_INPUT_EVENTS_EX                                                                            \
  DEF_INPUT_EVENT(OnInputControlsLoadConfig, const DataBlock * /* cfg_blk */)                          \
  DEF_INPUT_EVENT(OnInputControlsSaveConfig, DataBlock * /* cfg_blk */)                                \
  DEF_INPUT_EVENT(OnInputControlsBindSq, SqModules * /*moduleMgr*/, Sqrat::Table * /* controlTable */) \
  DEF_INPUT_EVENT(GetInputControlsNamedSensScale, const char * /*name*/, const float ** /* dest_scale_ptr */)

#define DEF_INPUT_EVENT ECS_BROADCAST_PROFILE_EVENT_TYPE
DEF_INPUT_EVENTS_EX
#undef DEF_INPUT_EVENT
