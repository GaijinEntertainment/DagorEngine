// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_lowLatency.h>

void lowlatency::start_frame(uint32_t) {}

bool lowlatency::is_available() { return false; }

lowlatency::LatencyMode lowlatency::get_latency_mode() { return lowlatency::LATENCY_MODE_OFF; }

lowlatency::LatencyMode lowlatency::get_from_blk() { return lowlatency::LATENCY_MODE_OFF; }

lowlatency::LatencyModeFlag lowlatency::get_supported_latency_modes() { return 0; }

void lowlatency::set_latency_mode(LatencyMode) {}

void lowlatency::set_latency_mode(LatencyMode, float) {}

void lowlatency::sleep(uint32_t) {}

void lowlatency::mark_flash_indicator() {}

void lowlatency::set_marker(uint32_t, LatencyMarkerType) {}

lowlatency::LatencyData lowlatency::get_statistics_since(uint32_t, uint32_t) { return {}; }

lowlatency::LatencyData lowlatency::get_last_statistics() { return {}; }

lowlatency::LatencyData lowlatency::get_new_statistics(uint32_t) { return {}; }

void lowlatency::render_debug_low_latency() {}

bool lowlatency::is_vsync_allowed() { return true; }
