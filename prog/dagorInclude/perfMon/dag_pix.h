//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstdint>


void PIX_BEGIN_CPU_EVENT(const char *name);
void PIX_END_CPU_EVENT();

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
#include <d3d12_x.h>
#elif _TARGET_SCARLETT
#include <d3d12_xs.h>
#else
typedef enum D3D12XBOX_PIX_CAPTURE_FLAGS
{
  D3D12XBOX_PIX_CAPTURE_ALL_ENGINES = 0x2, // Capture data from all engines.

  D3D12XBOX_PIX_CAPTURE_ASYNC_COMPUTE_AS_CPU = 0x4, // Instead of capturing the async compute commands themselves, capture the effects
                                                    // of async compute work as memory modifications made by the CPU. This can be
                                                    // useful if you want to capture only graphics commands and avoid capturing async
                                                    // compute commands. However, because the CPU replicates the effects of the async
                                                    // compute commands as memory updates, performance analysis in PIX might not
                                                    // accurately reflect the title’s performance.

  D3D12XBOX_PIX_CAPTURE_DMA_AS_CPU = 0x8, // Instead of capturing the DMA commands themselves, capture the effects of DMA work as
                                          // memory modifications made by the CPU. This can be useful if you want to capture only
                                          // graphics commands and avoid capturing DMA commands. However, because the CPU replicates
                                          // the effects of the Direct Memory Access (DMA) commands as memory updates, performance
                                          // analysis in PIX might not accurately reflect the title’s performance.

  D3D12XBOX_PIX_CAPTURE_ALL_MEMORY = 0x10, // Instead of capturing the memory used by the captured frame, capture all graphics memory
                                           // allocated by the title. This produces a large capture file. Specifying this flag might
                                           // slow down the PIX analysis of the capture file.

  D3D12XBOX_PIX_CAPTURE_API = 0x10000, // Capture all API arguments and GPU command buffers. This includes the resource names from
                                       // SetName and shader symbols.

  D3D12XBOX_PIX_CAPTURE_GOLD_IMAGE = 0x200000, // Reserved for internal use.
} D3D12XBOX_PIX_CAPTURE_FLAGS;
#endif


// Example:
// PIX_GPU_BEGIN_CAPTURE(D3D12XBOX_PIX_CAPTURE_API, L"D:\\PIXCaptureFileName.pix3");
// ...
// PIX_GPU_END_CAPTURE()

#include <drv/3d/dag_commands.h>
#include <util/dag_preprocessor.h>
#include <workCycle/dag_gameSettings.h>

#define PIX_GPU_BEGIN_CAPTURE(flags, output_filename)                                                    \
  do                                                                                                     \
  {                                                                                                      \
    dgctrl_need_screen_shot = true;                                                                      \
    d3d::driver_command(Drv3dCommand::PIX_GPU_BEGIN_CAPTURE, reinterpret_cast<void *>(uintptr_t(flags)), \
      const_cast<wchar_t *>(output_filename));                                                           \
  } while (0)

#define PIX_GPU_END_CAPTURE() d3d::driver_command(Drv3dCommand::PIX_GPU_END_CAPTURE);

#define PIX_GPU_CAPTURE_NEXT_FRAME(flags, output_filename)                                                     \
  do                                                                                                           \
  {                                                                                                            \
    dgctrl_need_screen_shot = true;                                                                            \
    d3d::driver_command(Drv3dCommand::PIX_GPU_CAPTURE_NEXT_FRAMES, reinterpret_cast<void *>(uintptr_t(flags)), \
      const_cast<wchar_t *>(output_filename), reinterpret_cast<void *>(uintptr_t(1)));                         \
  } while (0)

#define PIX_GPU_CAPTURE_NEXT_FRAMES(flags, output_filename, frame_count)                                       \
  do                                                                                                           \
  {                                                                                                            \
    dgctrl_need_screen_shot = true;                                                                            \
    d3d::driver_command(Drv3dCommand::PIX_GPU_CAPTURE_NEXT_FRAMES, reinterpret_cast<void *>(uintptr_t(flags)), \
      const_cast<wchar_t *>(output_filename), reinterpret_cast<void *>(uintptr_t(frame_count)));               \
  } while (0)

#define PIX_GPU_CAPTURE_NCALLS_IMPL(N, CounterName, flags, output_filename) \
  do                                                                        \
  {                                                                         \
    static uint(CounterName) = (N);                                         \
    if ((CounterName) == (N))                                               \
      PIX_GPU_BEGIN_CAPTURE(flags, output_filename);                        \
    if (!(CounterName--))                                                   \
      PIX_GPU_END_CAPTURE();                                                \
  } while (0)

#define PIX_GPU_CAPTURE_NCALLS(N, flags, output_filename) \
  PIX_GPU_CAPTURE_NCALLS_IMPL(N, DAG_CONCAT(calls_counter, __LINE__), flags, output_filename)

#define PIX_GPU_CAPTURE_SCOPE(flags, output_filename, enabled) \
  PixGpuCaptureScope DAG_CONCAT(captureScope, __LINE__)(flags, output_filename, enabled)

struct CaptureAfterLongFrameParams
{
  uintptr_t thresholdUS = 30000;
  uintptr_t frames = 1;
  uintptr_t flags = D3D12XBOX_PIX_CAPTURE_API;
  int captureCountLimit = -1;
};

struct PixGpuCaptureScope
{
  bool enabled;
  PixGpuCaptureScope(uintptr_t flags, const wchar_t *output_filename, bool enabled = true) : enabled(enabled)
  {
    if (enabled)
    {
      PIX_GPU_BEGIN_CAPTURE(flags, output_filename);
    }
  }
  ~PixGpuCaptureScope()
  {
    if (enabled)
      PIX_GPU_END_CAPTURE();
  }
};

#undef THIS // Workaround for Xbox not compiling when this is included in worldRenderer.cpp
