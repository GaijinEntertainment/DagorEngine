//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <soundSystem/handle.h>

namespace sndsys
{
StreamHandle init_stream(const char *url, const Point2 &min_max_distance, const char *bus = nullptr);

void open(StreamHandle stream_handle);
void close(StreamHandle stream_handle);
void set_pos(StreamHandle stream_handle, const Point3 &pos);
void release(StreamHandle &stream_handle);
inline void abandon(StreamHandle &stream_handle) { release(stream_handle); }
bool set_paused(StreamHandle stream_handle, bool pause);
bool set_volume(StreamHandle stream_handle, float volume);
void set_fader(StreamHandle stream_handle, const Point2 &fade_near_far_time);
bool is_valid_handle(StreamHandle stream_handle);

enum class StreamState
{
  ERROR,
  CLOSED,
  OPENED,
  CONNECTING,
  BUFFERING,
  STOPPED,
  PAUSED,
  PLAYING,
  NUM_STATES
};

StreamState get_state(StreamHandle stream_handle);
const char *get_state_name(StreamHandle stream_handle);
} // namespace sndsys
