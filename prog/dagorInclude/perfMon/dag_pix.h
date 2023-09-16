//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if _TARGET_XBOX && DAGOR_DBGLEVEL != 0

#ifndef USE_PIX
#define USE_PIX 1
#endif

#include <windows.h>
#include <pix.h>
#define PIX_BEGIN_CPU_EVENT(name) PIXBeginEvent(0, name)
#define PIX_END_CPU_EVENT()       PIXEndEvent();

#else

#define PIX_BEGIN_CPU_EVENT(name)
#define PIX_END_CPU_EVENT()

#endif


namespace perfmon
{
inline void add_named_counter(const char * /*name*/, float /*val*/) {}
inline void begin_named_event(const char * /*name*/) {}
inline void end_named_event() {}
inline void set_named_marker(const char * /*name*/) {}
inline void set_thread_name(const char * /*name*/) {}
} // namespace perfmon

namespace perfmon
{
class AutoNamedEvent
{
public:
  AutoNamedEvent(const char *name) { perfmon::begin_named_event(name); }
  ~AutoNamedEvent() { perfmon::end_named_event(); }
};
} // namespace perfmon

#if _TARGET_XBOXONE
#include <d3d11_x.h>
#else
typedef enum D3D11X_PIX_CAPTURE_FLAGS
{
  D3D11X_PIX_CAPTURE_ALL_ENGINES = 0x2, // Captures data from all engines.

  D3D11X_PIX_CAPTURE_ASYNC_COMPUTE_AS_CPU = 0x4, // Captures the effects of async compute work as memory modifications done by the CPU,
                                                 // instead of capturing the async compute commands themselves. This can be useful if
                                                 // you want to capture only graphics commands, without any async compute commands.
                                                 // However, since the effects of the async compute commands are replicated as memory
                                                 // updates by the CPU, performance analysis in PIX might not accurately reflect the
                                                 // title.

  D3D11X_PIX_CAPTURE_DMA_AS_CPU = 0x8, // Captures the effects of DMA work as memory modifications done by the CPU, instead of
                                       // capturing the DMA commands themselves. This can be useful if you want to capture only
                                       // graphics commands, without any DMA commands. However, since the effects of the DMA commands
                                       // are replicated as memory updates by the CPU, performance analysis in PIX might not accurately
                                       // reflect the title.

  D3D11X_PIX_CAPTURE_ALL_MEMORY = 0x10, // Captures all graphics memory allocated by the title, instead of on the memory used by the
                                        // captured frame. This will produce a capture file which is quite large. PIX analysis of the
                                        // resulting capture may be significantly slower with this flag.

  D3D11X_PIX_CAPTURE_API = 0x10000, // Performs an API capture that collects all API arguments, in addition to GPU command buffers.
                                    // This includes the resource names from ID3D11DeviceChild::SetName and shader symbols.

  D3D11X_PIX_CAPTURE_GOLD_IMAGE = 0x200000 // This member is private; do not use.
} D3D11X_PIX_CAPTURE_FLAGS;
#endif


// Example:
// PIX_GPU_BEGIN_CAPTURE(D3D11X_PIX_CAPTURE_API, L"D:\\PIXCaptureFileName.pix3");
// ...
// PIX_GPU_END_CAPTURE()

#define PIX_GPU_CAT1(a, b) a##b
#define PIX_GPU_CAT2(a, b) STATE_GUARD_CAT1(a, b)

#include <3d/dag_drv3dCmd.h>

#define PIX_GPU_BEGIN_CAPTURE(flags, output_filename)                                                  \
  d3d::driver_command(DRV3D_COMMAND_PIX_GPU_BEGIN_CAPTURE, reinterpret_cast<void *>(uintptr_t(flags)), \
    const_cast<wchar_t *>(output_filename), NULL);

#define PIX_GPU_END_CAPTURE() d3d::driver_command(DRV3D_COMMAND_PIX_GPU_END_CAPTURE, NULL, NULL, NULL);

#define PIX_GPU_CAPTURE_NEXT_FRAME(flags, output_filename)                                                   \
  d3d::driver_command(DRV3D_COMMAND_PIX_GPU_CAPTURE_NEXT_FRAMES, reinterpret_cast<void *>(uintptr_t(flags)), \
    const_cast<wchar_t *>(output_filename), reinterpret_cast<void *>(uintptr_t(1)));

#define PIX_GPU_CAPTURE_NEXT_FRAMES(flags, output_filename, frame_count)                                     \
  d3d::driver_command(DRV3D_COMMAND_PIX_GPU_CAPTURE_NEXT_FRAMES, reinterpret_cast<void *>(uintptr_t(flags)), \
    const_cast<wchar_t *>(output_filename), reinterpret_cast<void *>(uintptr_t(frame_count)));

#define PIX_GPU_CAPTURE_NCALLS_IMPL(N, CounterName, flags, output_filename) \
  static uint(CounterName) = (N);                                           \
  if ((CounterName) == (N))                                                 \
    PIX_GPU_BEGIN_CAPTURE(flags, output_filename)                           \
  if (!(CounterName--))                                                     \
  PIX_GPU_END_CAPTURE()

#define PIX_GPU_CAPTURE_NCALLS(N, flags, output_filename) \
  PIX_GPU_CAPTURE_NCALLS_IMPL(N, PIX_GPU_CAT2(calls_counter, __LINE__), flags, output_filename);

#define PIX_GPU_CAPTURE_SCOPE(flags, output_filename, enabled) \
  PixGpuCaptureScope PIX_GPU_CAT2(captureScope, __LINE__)(flags, output_filename, enabled);

struct CaptureAfterLongFrameParams
{
  uintptr_t thresholdUS = 30000;
  uintptr_t frames = 1;
  uintptr_t flags = D3D11X_PIX_CAPTURE_API;
  int captureCountLimit = -1;
};

struct PixGpuCaptureScope
{
  bool enabled;
  PixGpuCaptureScope(uintptr_t flags, const wchar_t *output_filename, bool enabled = true) : enabled(enabled)
  {
    if (enabled)
      PIX_GPU_BEGIN_CAPTURE(flags, output_filename);
  }
  ~PixGpuCaptureScope()
  {
    if (enabled)
      PIX_GPU_END_CAPTURE();
  }
};
