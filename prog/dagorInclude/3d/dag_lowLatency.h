//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

/*
 * This header contains the interface for the platform independent low latency system
 * It has backends for both REFLEX, Anti Lag 2 and XeLL.
 */

#include <3d/dag_latencyTypes.h>
#include <util/dag_globDef.h>

class DataBlock;

namespace lowlatency
{

bool is_available();
void start_frame(uint32_t frame_id);
void set_latency_mode(LatencyMode mode);
// set fps limit using Nvidia Reflex SDK
// minimum_interval_ms should be 1000 / <max_frames_per_sec>
// minimum_interval_ms = 0 means no limit
void set_latency_mode(LatencyMode mode, float minimum_interval_ms);
LatencyModeFlag get_supported_latency_modes();
void sleep(uint32_t frame_id);
void set_marker(uint32_t frame_id, LatencyMarkerType marker_type);
void mark_flash_indicator();
LatencyMode get_latency_mode();
LatencyMode get_from_blk();
// max_count = 0 -> no limit
LatencyData get_statistics_since(uint32_t frame_id, uint32_t max_count = 0);
LatencyData get_new_statistics(uint32_t max_count = 0); // frames, that have not been queried
LatencyData get_last_statistics();
void render_debug_low_latency();
bool is_vsync_allowed();

} // namespace lowlatency
