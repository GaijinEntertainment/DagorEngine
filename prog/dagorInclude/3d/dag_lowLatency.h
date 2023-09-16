//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/*
 * This header contains the interface for the platform independent low latency system
 * The Nvidia Low Latency SDK integration is done in separate files, prefixed by 'nv'
 * (dag_nvLowLatency.h)
 */

#include <3d/dag_latencyTypes.h>
#include <util/dag_globDef.h>

class DataBlock;

namespace lowlatency
{

void init();
void close();
uint32_t start_frame();
uint32_t get_current_frame();
void start_render();
uint32_t get_current_render_frame();
void set_latency_mode(LatencyMode mode);
// set fps limit using Nvidia Reflex SDK
// minimum_interval_ms should be 1000 / <max_frames_per_sec>
// minimum_interval_ms = 0 means no limit
void set_latency_mode(LatencyMode mode, float minimum_interval_ms);
LatencyModeFlag get_supported_latency_modes();
// return true if a significant amount of sleep was performed. Used to determine if extra input sampling is useful
bool sleep();
void set_marker(uint32_t frame_id, LatencyMarkerType marker_type);
void mark_flash_indicator();
bool feed_latency_input(unsigned int msg);
LatencyMode get_latency_mode();
LatencyMode get_from_blk();
// max_count = 0 -> no limit
LatencyData get_statistics_since(uint32_t frame_id, uint32_t max_count = 0);
LatencyData get_new_statistics(uint32_t max_count = 0); // frames, that have not been queried
LatencyData get_last_statistics();
void render_debug_low_latency();

void register_slop(uint32_t frame, int duration_usec); // since last frame

class ScopedLatencyMarker
{
  uint32_t frameId;
  LatencyMarkerType endMarker;
  bool closed = false;

public:
  ScopedLatencyMarker(uint32_t frame_id, LatencyMarkerType start_marker, LatencyMarkerType end_marker);
  ~ScopedLatencyMarker();
  ScopedLatencyMarker(const ScopedLatencyMarker &) = delete;
  ScopedLatencyMarker &operator=(const ScopedLatencyMarker &) = delete;

  void close();
};

class Timer
{
  int64_t stamp;

public:
  Timer();
  Timer(const Timer &) = delete;
  Timer &operator=(const Timer &) = delete;
  Timer(Timer &&) = default;
  Timer &operator=(Timer &&) = default;

  int timeUSec() const;
};

struct ScopedSlop
{
  uint32_t frame;
  Timer t;
  explicit ScopedSlop(uint32_t frame_id) : frame(frame_id) {}
  ~ScopedSlop() { register_slop(frame, t.timeUSec()); }
};
} // namespace lowlatency

#define SCOPED_LATENCY_MARKER(frame_id, start_marker, end_marker)                                                  \
  lowlatency::ScopedLatencyMarker latency_marker_##__LINE__(frame_id, lowlatency::LatencyMarkerType::start_marker, \
    lowlatency::LatencyMarkerType::end_marker)
#define REGISTER_SCOPED_SLOP(frame_id, name)               \
  lowlatency::ScopedSlop scoped_slop_##__LINE__(frame_id); \
  TIME_D3D_PROFILE(latency_slop__##name);
