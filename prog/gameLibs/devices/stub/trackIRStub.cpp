// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "devices/trackIR.h"

namespace trackir
{
static TrackData current_track_data;

void init(int /*smoothing*/) {}
void shutdown() {}

bool is_installed() { return false; }
bool is_enabled() { return false; }
bool is_active() { return false; }

void update() {}

void start() {}
void stop() {}

TrackData &get_data() { return current_track_data; }

void toggle_debug_draw() {}
void draw_debug(HudPrimitives *) {}

} // namespace trackir

#if _TARGET_C1 | _TARGET_C2










#endif
