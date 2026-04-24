//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stddef.h>
#include <generic/dag_span.h>

/** \defgroup BenchmarkPerFrameStat Per-frame benchmark statistics
 * A simple helper to collect per-frame metrics for benchmarks.
 * @{
 */

namespace benchmark_perframe_stat
{

/// GPU timing helpers using a ring buffer of GPU timestamp queries.
/// The ring buffer compensates for GPU_TIMESTAMP_LATENCY frames of delay
/// between issuing a query and reading back the result.
/// Call init() before the benchmark loop and close() when done.
namespace gpu_time
{
/// \brief Initialises the GPU frequency and allocates the timestamp ring buffer.
void init();

/// \brief Returns true if the ring buffer has been allocated.
bool initialized();

/// \brief Destroys the ring buffer. Must be called before shutdown (checked by assert).
void close();
} // namespace gpu_time

/**
 * \brief Allocates the buffer that holds per-frame metrics for a single benchmark iteration.
 * \details Expects that workcycleperf::record_mode is not Disabled before calling.
 * May be called more than once, but doing so is suboptimal and will log a warning.
 * \param frames_limit Maximum number of frames the buffer can hold.
 */
void init(unsigned frames_limit);

/// \brief Returns true if the buffer has been allocated.
bool initialized();

/// \brief Clears the buffer between benchmark iterations.
void reset();

/**
 * \brief Appends the last frame's data to the buffer.
 * \details Should be called exactly once per frame while a benchmark is running.
 * Logs an error if called more than once for the same frame.
 */
void add_last_frame();

/**
 * \brief Dumps the collected buffer to a file using the currently set dump function.
 * \details By default writes a CSV file with workcycle time, CPU-only cycle time.
 * The implementation can be overridden via set_dump_to_file().
 * Logs an error if record_mode is Disabled or the file cannot be opened.
 * \param fname Path to the output file.
 */
void dump_to_file(const char *fname);

/**
 * \brief Overrides the default dump_to_file implementation.
 * \details Pass nullptr to restore the built-in CSV writer.
 * \param dump_function Pointer to the new dump function.
 */
void set_dump_to_file(void (*dump_function)(const char *fname));

/// Snapshot of a single frame's performance counters.
struct PerFrameData
{
  int workcycleTimeUsec;    ///< Total workcycle time (CPU + wait-for-GPU), microseconds.
  int cpuOnlyCycleTimeUsec; ///< CPU-only portion of the cycle, microseconds.
  int gpuMemoryUsageKb;     ///< Device VRAM in use at the end of the frame, kilobytes.
  int gpuTimeUsec;          ///< GPU frame time from timestamp queries, microseconds (0 if gpu_time not initialized).
};

/// \brief Returns a read-only view of all frames recorded since the last reset().
dag::ConstSpan<PerFrameData> get_per_frame_data();

} // namespace benchmark_perframe_stat

//@}
