// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_lowLatency.h>

uint32_t lowlatency::start_frame() { return 0; }

uint32_t lowlatency::get_current_frame() { return 0; }

void lowlatency::start_render() {}

uint32_t lowlatency::get_current_render_frame() { return 0; }

void lowlatency::init() {}

bool lowlatency::is_inited() { return false; }

void lowlatency::close() {}

lowlatency::LatencyMode lowlatency::get_latency_mode() { return lowlatency::LATENCY_MODE_OFF; }

lowlatency::LatencyMode lowlatency::get_from_blk() { return lowlatency::LATENCY_MODE_OFF; }

lowlatency::LatencyModeFlag lowlatency::get_supported_latency_modes() { return 0; }

void lowlatency::set_latency_mode(LatencyMode) {}

void lowlatency::set_latency_mode(LatencyMode, float) {}

bool lowlatency::sleep() { return false; }

void lowlatency::mark_flash_indicator() {}

bool lowlatency::feed_latency_input(unsigned int) { return false; }

void lowlatency::set_marker(uint32_t, LatencyMarkerType) {}

lowlatency::LatencyData lowlatency::get_statistics_since(uint32_t, uint32_t) { return {}; }

lowlatency::LatencyData lowlatency::get_last_statistics() { return {}; }

lowlatency::LatencyData lowlatency::get_new_statistics(uint32_t) { return {}; }

void lowlatency::ScopedLatencyMarker::close() {}

lowlatency::ScopedLatencyMarker::ScopedLatencyMarker(uint32_t, LatencyMarkerType, LatencyMarkerType) {}

lowlatency::ScopedLatencyMarker::~ScopedLatencyMarker() {}

void lowlatency::register_slop(uint32_t, int) {}

void lowlatency::render_debug_low_latency() {}

lowlatency::Timer::Timer() : stamp(0) {}

int lowlatency::Timer::timeUSec() const { return 0; }
