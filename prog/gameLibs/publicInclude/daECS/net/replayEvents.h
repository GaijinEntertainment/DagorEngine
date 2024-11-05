//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/net/netEvent.h>
#include <rapidjson/document.h>

#define NET_REPLAY_ECS_EVENTS NET_REPLAY_ECS_EVENT(EventOnWriteReplayMetaInfo, rapidjson::Document)

#define NET_REPLAY_ECS_EVENT ECS_BROADCAST_EVENT_TYPE
NET_REPLAY_ECS_EVENTS
#undef NET_REPLAY_ECS_EVENT
