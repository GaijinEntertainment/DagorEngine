//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/event.h>
#include <daInput/input_api.h>

#define DAINPUT_ECS_EVENTS                                                                                   \
  DAINPUT_ECS_EVENT(EventHidGlobalInputSink, unsigned /*t0_msec*/, int /*dev*/, bool /*pressed*/,            \
    unsigned /*button_id=(dev<<8)|btn_idx*/)                                                                 \
  DAINPUT_ECS_EVENT(EventDaInputInit, bool /*init*/)                                                         \
  DAINPUT_ECS_EVENT(EventDaInputActionTriggered, dainput::action_handle_t /*action*/, short /*duration_ms*/) \
  DAINPUT_ECS_EVENT(EventDaInputActionTerminated, dainput::action_handle_t /*action*/, short /*duration_ms*/)

#define DAINPUT_ECS_EVENT ECS_BROADCAST_EVENT_TYPE
DAINPUT_ECS_EVENTS
#undef DAINPUT_ECS_EVENT

#define SETUP_DAINPUT_ACTION(VAR, PREFIX, TYPEGRP) a##VAR = dainput::get_action_handle(PREFIX #VAR, TYPEGRP)
#define SETUP_ACTION_DIGITAL(VAR, PREFIX)          SETUP_DAINPUT_ACTION(VAR, PREFIX, dainput::TYPEGRP_DIGITAL)
#define SETUP_ACTION_AXIS(VAR, PREFIX)             SETUP_DAINPUT_ACTION(VAR, PREFIX, dainput::TYPEGRP_AXIS)
#define SETUP_ACTION_STICK(VAR, PREFIX)            SETUP_DAINPUT_ACTION(VAR, PREFIX, dainput::TYPEGRP_STICK)
#define CLEAR_DAINPUT_ACTION(VAR)                  a##VAR = dainput::BAD_ACTION_HANDLE
