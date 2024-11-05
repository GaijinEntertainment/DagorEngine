// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include "main/appProfile.h"
#include "net/replay.h"


namespace bind_dascript
{
static inline const char *replay_get_play_file() { return app_profile::get().replay.playFile.c_str(); }
static inline void replay_clear_current_play_file() { return app_profile::getRW().replay.playFile.clear(); }
static inline float replay_get_play_start_time() { return app_profile::get().replay.startTime; }
static inline bool is_replay_playing() { return !app_profile::get().replay.playFile.empty(); }
bool load_replay_meta_info(
  const char *path, const das::TBlock<void, const das::TTemporary<rapidjson::Document>> &blk, das::Context *ctx, das::LineInfoArg *at);
void replay_play(const char *path, float start_time = 0.f);

} // namespace bind_dascript
