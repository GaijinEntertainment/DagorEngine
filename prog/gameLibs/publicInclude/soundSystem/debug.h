//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace sndsys
{
void debug_trace_info(const char *format, ...);
void debug_trace_warn(const char *format, ...);
void debug_trace_err(const char *format, ...);
void debug_trace_log(const char *format, ...);

void debug_draw();
void set_enable_debug_draw(bool enable);
bool get_enable_debug_draw();
void set_draw_audibility(bool enable);
void debug_enum_events();
}; // namespace sndsys
