// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <rapidjson/fwd.h>
#include <daECS/net/replay.h>

namespace sceneload
{
struct UserGameModeContext;
}

void net_replay_rewind();
const rapidjson::Document &get_currently_playing_replay_meta_info();
bool load_replay_meta_info(const char *path, const eastl::function<void(const rapidjson::Document &)> &meta_cb);
void replay_play(const char *path, float start_time, sceneload::UserGameModeContext &&ugmCtx, const char *scene = "");
