//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_latencyTypes.h"

class DataBlock;

namespace nvlowlatency
{
void start_frame(uint32_t frame_id);
bool is_available();
// minimum_interval_ms should be 1000 / <max_frames_per_sec>
// minimum_interval_ms = 0 means no limit
void set_latency_mode(lowlatency::LatencyMode mode,
  float minimum_interval_ms); // set fps limit using
                              // nvidia low latency sdk
void sleep();
bool feed_latency_input(uint32_t frame_id, unsigned int msg);
void set_marker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type);

// max_count = 0 -> no limit
lowlatency::LatencyData get_statistics_since(uint32_t frame_id, uint32_t max_count = 0);
} // namespace nvlowlatency
