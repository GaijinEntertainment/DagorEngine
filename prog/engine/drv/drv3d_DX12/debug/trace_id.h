// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace drv3d_dx12::debug
{
enum class TraceID : uint32_t
{
};

inline bool operator<(const TraceID l, const TraceID r) { return static_cast<uint32_t>(l) < static_cast<uint32_t>(r); }

inline bool operator>(const TraceID l, const TraceID r) { return static_cast<uint32_t>(l) > static_cast<uint32_t>(r); }
/// May returned when a trace could not be written. Reading such a trace will always yield NotLaunched.
inline constexpr TraceID invalid_trace_value = static_cast<TraceID>(0xFFffFFff);
/// Default value of a trace, indicates the trace before the first actually written trace.
inline constexpr TraceID null_trace_value = static_cast<TraceID>(0);
} // namespace drv3d_dx12::debug