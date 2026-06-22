// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stddef.h>
#include <generic/dag_tab.h>
#include <daECS/core/event.h>

namespace ecs
{
class EntityManager;
}

ECS_BROADCAST_EVENT_TYPE(EventQueryReplayPlayback, bool * /*out: has playback file*/)

bool is_replay_recording();
void server_create_replay_record();
bool try_create_replay_playback(ecs::EntityManager &mgr);
void gather_replay_meta_info();
void clear_replay_meta_info();
void finalize_replay_record();
Tab<uint8_t> gen_replay_meta_data();
