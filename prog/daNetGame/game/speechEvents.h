// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/event.h>
#include <daECS/core/componentTypes.h>


#define DEF_SPEECH_EVENTS                                                             \
  DECL_GAME_EVENT(CmdRequestHumanSpeech, SimpleString /*phrase*/)                     \
  DECL_GAME_EVENT(CmdRequestHumanSpeechAbout, SimpleString /*phrase*/, ecs::EntityId) \
  DECL_GAME_EVENT(CmdRequestHumanVoiceEffect, SimpleString /*phrase*/)


#define DECL_GAME_EVENT ECS_UNICAST_EVENT_TYPE
DEF_SPEECH_EVENTS
#undef DECL_GAME_EVENT
