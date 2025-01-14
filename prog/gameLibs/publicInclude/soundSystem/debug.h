//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/functional.h>

class TMatrix4;

namespace sndsys
{
void debug_trace_info(const char *format, ...);
void debug_trace_warn(const char *format, ...);
void debug_trace_err(const char *format, ...);
void debug_trace_err_once(const char *format, ...);
void debug_trace_log(const char *format, ...);

void debug_draw();
void set_draw_audibility(bool enable);
void debug_enum_events();
void debug_enum_events(const char *bank_name, eastl::function<void(const char *)> &&fun);
}; // namespace sndsys
