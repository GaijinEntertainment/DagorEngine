// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <soundSystem/strHash.h>
#include <util/dag_simpleString.h>

#define ANIMCHAR_SOUND_ECS_EVENTS                                                                         \
  ANIMCHAR_SOUND_ECS_EVENT(CmdSoundIrq, const char * /*irq_name*/, sndsys::str_hash_t /*irqType*/)        \
  ANIMCHAR_SOUND_ECS_EVENT(CmdSoundMeleeIrq, const char * /*irq_name*/)                                   \
  ANIMCHAR_SOUND_ECS_EVENT(CmdSoundVoicefxIrq, const char * /*irq_name*/, sndsys::str_hash_t /*irqType*/) \
  ANIMCHAR_SOUND_ECS_EVENT(CmdSoundDebugAnimParam, float /*param*/, SimpleString /*node_name*/)

ECS_UNICAST_PROFILE_EVENT_TYPE(CmdSoundStepIrq, int /*obj_idx*/)

#define ANIMCHAR_SOUND_ECS_EVENT ECS_UNICAST_EVENT_TYPE
ANIMCHAR_SOUND_ECS_EVENTS
#undef ANIMCHAR_SOUND_ECS_EVENT
