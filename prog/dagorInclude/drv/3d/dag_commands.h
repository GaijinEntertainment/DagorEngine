//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;

struct FrameEvents
{
  virtual ~FrameEvents() = default;

  virtual void beginFrameEvent() = 0;
  virtual void endFrameEvent() = 0;
};

struct DeviceResetEventHandler
{
  virtual ~DeviceResetEventHandler() = default;

  virtual void preRecovery() = 0;
  virtual void recovery() = 0;
};

enum class Drv3dCommand
{
  GETVISIBILITYBEGIN,
  GETVISIBILITYEND,
  GETVISIBILITYCOUNT,
  RELEASE_QUERY,

  //! acquire/release D3D lock (to perform rendering in non-primary thread)
  ACQUIRE_OWNERSHIP,
  RELEASE_OWNERSHIP,

  //! acquire/release a mutex to perform resource loading in non-primary thread and postpone GPU resets, the lock is nonexclusive.
  ACQUIRE_LOADING,
  RELEASE_LOADING,

  GET_GPU_FRAME_TIME,

  GET_VSYNC_REFRESH_RATE,

  // async pipeline compilation range
  // both commands require GPU lock to avoid affecting unrelated draws/dispatches
  //
  // starts range, returns 1 if supported by driver
  //  par1 - if not null, disables async for graphics & compute (regardless of par2)
  //  par2 - if not null, disables async for compute
  // i.e. default arguments enable async for graphics & compute
  ASYNC_PIPELINE_COMPILE_RANGE_BEGIN,
  // ends range, returns 1 if supported by driver
  //  last async pipeline range is continued after ending,
  //  or if there is no active range after end, async pipeline compilation is disabled
  ASYNC_PIPELINE_COMPILE_RANGE_END,

  // recives current amount of pipelines in compilation queue
  GET_PIPELINE_COMPILATION_QUEUE_LENGTH,
  // forms async pipeline compilation feeedback in range of draw commands executed between begin&end
  // par1 - user pointer to uint32_t, pointer must not be released (i.e. permanent pointer!)
  //  begin marks range begin
  //        sets bit 2 if other calls to this command setted bit 1 in same frame
  //        sets bit 1 if some of draws was rejected due to pipeline was not ready
  //        sets bit 0 if supported
  //        returns 0 if not supported
  //  end marks range end
  //  ranges must not intersect (for simplicity)
  ASYNC_PIPELINE_COMPILATION_FEEDBACK_BEGIN,
  ASYNC_PIPELINE_COMPILATION_FEEDBACK_END,

  TIMESTAMPFREQ,
  TIMESTAMPISSUE,
  TIMESTAMPGET,

  //! Gets CPU and GPU time nearly at the same moment
  // par1 - cpu timestamp
  // par2 - gpu timestamp
  // par3 (optional) - cpu frequency type (see below, DRV3D_CPU_FREQ_TYPE_*)
  // Warning! with time timestamp _will_ deviate. We need to recalibrate clock from time to time!
  TIMECLOCKCALIBRATION,


  FLUSH_STATES, // flush all states, except buffer. will resolve msaa, generate mips, etc - if supported!
  D3D_FLUSH,    // ID3D11DeviceContext::Flush or IDirect3DQuery9::GetData(D3DGETDATA_FLUSH).

  //! flush pipeline states to compile pipeline
  // par1: uintptr_t that stores ShaderStage
  //       if par1 == STAGE_PS, compiles graphics pipeline
  //       if par1 == STAGE_CS, compiles compute pipeline
  // par2: uintptr_t that stores topology if par1 == ShaderStage::STAGE_PS
  COMPILE_PIPELINE,

  GET_SECONDARY_BACKBUFFER,

  GET_VENDOR,

  OVERRIDE_MAX_ANISOTROPY_LEVEL,

  // Creates an engine texture object from a raw resource
  MAKE_TEXTURE,
  GET_TEXTURE_HANDLE,

  // Start / Finish exclusive access for working with graphical resources and rendering context.
  // It is useful in cases when part of some rendering is performed beyond the borders of the driver
  BEGIN_EXTERNAL_ACCESS,
  END_EXTERNAL_ACCESS,

  SET_VS_DEBUG_INFO,
  SET_PS_DEBUG_INFO,

  // MRT clear sequence is an optimization to clear mrt targets with different colors in
  // an optimized fashion for target APIs like Vulkan.

  // Begins a mrt clear sequence.
  // This can turn a sequence of bind color target -> clear color target
  // into one operation of the backend (and even can merge the clear into
  // the render target load on the first draw to it).
  // This delays the actuall clear either until the next flush or to the next
  // draw command (which is a implicite flush).
  // Param1 is the index of the next mrt clear color, param1 is used as is
  // so it is _NOT_ a pointer to an int, the pointer value is _REINTERPRETED_ as
  // intptr_t and used as index.
  // You can send this command multiple times with different indices each time
  // to set the index before commiting with END_MRT_CLEAR_SEQUENCE.
  // Each clearview after this incrments the index of the next render target clear
  // color.
  // Param2 is an optinal parameter to an uint32_t that is set to 1 if the backend
  // implements mrt clear sequence support.
  BEGIN_MRT_CLEAR_SEQUENCE,
  // Ends a mrt clear sequence. The next command that triggers a clear will then
  // use the color values (and depth stencil values for color target 0) to do
  // the clear the currently set render targets.
  // Param1 is used as a hint if a immediate clear with this call is required or not,
  // if Param1 is NULL then a clear with this call is executed, if Param1 is not NULL
  // then the clear is delayed as long as possible.
  END_MRT_CLEAR_SEQUENCE,
  // Queries the selected render dervices driver cache format uuid.
  // First parameter has to be a pointer to a 128 byte big memory block to store the uuid.
  // All other parameters are unused.
  // Returns 1 if the uuid was written to the first parameter.
  GET_SHADER_CACHE_UUID,

  AFTERMATH_MARKER, // Nvidia Aftermath marker. Pointer must be valid until the crash. Does nothing when Aftermath is
                    // disabled.

  START_CAPTURE_FRAME,

  IS_HDR_AVAILABLE,
  IS_HDR_ENABLED,
  INT10_HDR_BUFFER,
  HDR_OUTPUT_MODE,
  GET_LUMINANCE,

  MEM_STAT, // Single line tex and buffers statistics.

  // Lets the D3D backend know about a ScriptedShadersBinDumpOwner that has just been loaded.
  // This command allows the backend to prepare all shaders of this bin dump for faster loading,
  // including usage of caches of previous runs of the application.
  //
  // par1: const ScriptedShadersBinDumpOwner *: Pointer to scripted shader dump, or null.
  // par2: const char: Name of the file of the scripted shader dump, or null.
  REGISTER_SHADER_DUMP,

  // par1 is external shader id (shader number in the dump)
  // par2 is shader stage (ShaderCodeType)
  // [out] par3 is pointer to internal shader id (if driver doesn't support this command, then par3 remains unchanged)
  GET_SHADER,

  PIX_GPU_BEGIN_CAPTURE,
  PIX_GPU_END_CAPTURE,
  PIX_GPU_CAPTURE_NEXT_FRAMES,
  PIX_GPU_CAPTURE_AFTER_LONG_FRAMES,

  // Ask driver by what factor it was limited when rendering last frame
  GET_FRAMERATE_LIMITING_FACTOR,

  // Loads additional pipeline cache from path @par1
  LOAD_PIPELINE_CACHE,
  // Saves current pipeline cache on FS.
  SAVE_PIPELINE_CACHE,

  // Get pointer to the DLSS object
  // par1: DLSS**
  GET_DLSS,

  // Get pointer to the Reflex object
  // par1: Reflex**
  GET_REFLEX,

  // Returns XeSS state as a XessState struct casted to int. No parameters required
  GET_XESS_STATE,

  // Returns if a XeSS quality is available at a target resolution
  // par1: IPoint2* that contains target resolution width and height
  // par2: XeSS quality as int*
  IS_XESS_QUALITY_AVAILABLE_AT_RESOLUTION,

  // Return current XESS rendering resolution in par1 and par2
  // par1: int *width
  // par2: int *height
  GET_XESS_RESOLUTION,

  // Returns current XESS version as a string.
  // par1: char *version
  // par2: size_t versionSize
  GET_XESS_VERSION,

  // Executes DLSS
  // par1: DlssParams *
  // par2: int *view_index
  EXECUTE_DLSS,

  // Executes DLSS-G
  // par1: DlssGParams *
  // par2: int *view_index
  EXECUTE_DLSS_G,

  // Executes DLSS
  // par1: XessParams *
  EXECUTE_XESS,

  // Set velocity scaling for XeSS,
  // par1: float *x
  // par2: float *y
  SET_XESS_VELOCITY_SCALE,

  // Execute FSR
  // par1: amd::FSR::UpscalingArgs *
  EXECUTE_FSR,

  // Execute FSR2
  // par1: Fsr2Params *
  EXECUTE_FSR2,

  // Returns current FSR2 state as Fsr2State casted to int. No parameters required
  GET_FSR2_STATE,

  // Return current FSR2 rendering resolution in par1 and par2
  // par1: int *width
  // par2: int *height
  GET_FSR2_RESOLUTION,

  // Execute PSSR
  // par1: d3d::PSSR::FrameParameters *
  EXECUTE_PSSR,

  // Returns state of MetalFX upscale availability (int)
  GET_METALFX_UPSCALE_STATE,

  // Execute MetalFX upscale
  // par1: MtlFxUpscaleParams *
  EXECUTE_METALFX_UPSCALE,

  // Queries the current timing statistics of the driver.
  // par1: A pointer to a Drv3dTimings variable to receive the timing data.
  // par2: Optional intptr_t (not a pointer!) as an index into the list of past timings
  //       0 is current, 1 is one frame in the past and so on.
  // Return: Number of timings available.
  GET_TIMINGS,

  // Reports the current GPU memory used by raytrace acceleration structures.
  // par1: (uint64_t *): Raytrace acceleration structures memory usage in bytes.
  GET_RAYTRACE_ACCELERATION_STRUCTURES_MEMORY_USAGE,

  // Simple command, prints the string into the debug log.
  // The message is printed as the driver executes the command stream it might has recorded.
  // This command is designed primarily for postmortem log analysis, to have a orientation point
  // for other messages, to easier track down locations of interest.
  // Takes up to 3 parameters:
  // First is the string, second is the string length as intptr_t (<=0 means use strlen),
  // and third is severity level as intptr_t (0-debug, 1-warning, 2-error).
  DEBUG_MESSAGE,

  // Returns unique, identifiable names of active monitors (eg. "\\.\DISPLAY1"), which are attached to the active adapter
  // par1: Tab<String> *monitorNames
  GET_MONITORS,

  // Returns some user-friendly info about a monitor (friendly name, unique index)
  // par1: const char *uniqueMonitorName
  // par1: String *friendlyMonitorName (optional)
  // par2: int *uniqueMonitorIndex (optional)
  GET_MONITOR_INFO,

  // Returns available resolutions for a monitor
  // par1: Tab<String> *monitorNames
  // par2: Tab<String> *availableResolutions
  GET_RESOLUTIONS_FROM_MONITOR,

  /// Returns video memory budget in KiByte values
  /// par1: pointer to uint32_t: Total memory budget on the device, possibly less than physical memory of the device
  /// par2: pointer to uint32_t: Free memory budget on the device, this is *par1 minus used memory
  /// par3: pointer to uint32_t: Used memory, this is *par1 minus *par2
  /// Return: uint32_t: Physical memory of the device
  GET_VIDEO_MEMORY_BUDGET,

  // Returns a handle for the platform specific graphics command queue
  // par1: void **handleHolder
  GET_RENDERING_COMMAND_QUEUE,

  // Returns the corresponding vulkan objects on par1
  GET_INSTANCE,
  GET_PHYSICAL_DEVICE,
  GET_QUEUE_FAMILY_INDEX,
  GET_QUEUE_INDEX,

  // Should be called when work cycle update loop decides
  // that no drawing will be performed due to app being hidden/inactive/minimized
  PROCESS_APP_INACTIVE_UPDATE,

  // Starts special BB rotate transform pass
  PRE_ROTATE_PASS,

  // Indicates generic render pass start for validation
  BEGIN_GENERIC_RENDER_PASS_CHECKS,

  // Indicates generic render pass end for validation
  END_GENERIC_RENDER_PASS_CHECKS,

  // par1 is a pointer to an os_event_t, which is going to be
  // signaled once the rendering of the current frame is completed
  // on the GPU.
  REGISTER_FRAME_COMPLETION_EVENT,

  // par1 is a pointer to a FrameEvents object.
  // par2 is understood as a boolean, specifying if events should coming from frontend or backend
  REGISTER_ONE_TIME_FRAME_EXECUTION_EVENT_CALLBACKS,

  // The driver will report all resource state transitions, of the given resource, to the debug log.
  // par1 pointer to resource to be tracked.
  // Returns 1 if the driver supports this command.
  REPORT_RESOURCE_STATE_TRANSITIONS,

  // par1 is a pointer to a DeviceResetEventHandler object.
  REGISTER_DEVICE_RESET_EVENT_HANDLER,
  UNREGISTER_DEVICE_RESET_EVENT_HANDLER,

  // Tells the driver to transition the textures passed into a state that it is consumeable by VR
  // runtimes. This should be RB_RW_RENDER_TARGET or RB_RW_DEPTH_STENCIL_TARGET.
  // par1 is an array of BaseTexture* pointers.
  // par2 is the number of textures in the array.
  PREPARE_TEXTURES_FOR_VR_CONSUMPTION,

  // Informs driver about current app name & version/build number
  // par1 is app name string,
  // par2 is app ver uint32_t
  SET_APP_INFO,

  // Uses the provided data to simulate a GPU crash, where we capture a dump and send it to logs server.
  // Logs send by this will have a extra meta data field "was-manually-send" set to true.
  // par1: const char *dump_type: currently only "nv-gpudmp" is supported, any other will be ignored.
  // par2: const void *dump_data: pointer to dump data to use.
  // par3: uintptr_t dump_data_size: size of the data of 'dump_data'.
  SEND_GPU_CRASH_DUMP,

  // Allows telling driver to perform needed perf-frame update (when used from wait cycle on main thread)
  // return 1 when pending ops performed, 0 otherwise
  PROCESS_PENDING_RESOURCE_UPDATED,

  GET_PS5_PSSR_STATUS,

  SET_FREQ_LEVEL,

  // For debug purpose.
  // If this returns true then the render thread will be wait after each draw and dispatch calls.
  // This helps investigating which command causes error on GPU.
  ENABLE_IMMEDIATE_FLUSH,
  DISABLE_IMMEDIATE_FLUSH,

  GET_WORKER_CPU_CORE,

  // Sets to the PS5 driver which eye is rendered in VR and get back the distortion resolve LUT's ID
  SET_PS5_FSR_VIEW,

  // par1: bool enable/disable HDR
  SET_HDR,

  // This raises a debug break event / signal in the driver when it is executed.
  // Drivers with backend workers this will be executed in that thread.
  // This command allows to debug break into the backend to help with issues during execution of d3d commands.
  DEBUG_BREAK,
  // Adds a string that is searched in the resolved call stack string and if found a debug break event is raised.
  ADD_DEBUG_BREAK_STRING_SEARCH,
  // Removes a string previously added with ADD_DEBUG_BREAK_STRING_SEARCH
  REMOVE_DEBUG_BREAK_STRING_SEARCH,

  IS_DEFRAG_REQUESTED,
  // returns true if defragmentation was done
  PROCESS_PENDING_DEFRAG_REQUESTS,

  // par1: CompilePipelineSet*
  COMPILE_PIPELINE_SET,

  // par1: Sbuffer*
  // par2: uint64_t*
  GET_BUFFER_GPU_ADDRESS,

  ENABLE_WORKER_LOW_LATENCY_MODE,

  // par1 enable (if par1 not null)/disable logerr reporting when doing sync readback of textures/buffers
  LOGERR_ON_SYNC,

  // Registers a driver network manager that wiil be used to send data from client
  // After the command execution the driver will take ownership of the object
  // par1: pointer on a DriverNetManager object
  SET_DRIVER_NETWORD_MANAGER,

  // Gets if the console is running at 120Hz
  // returns true if running on a console and the display is running at 120Hz
  GET_CONSOLE_HFR_STATUS,

  // Gets if the console is capable to run at 120Hz
  // returns true if running on a console and the display supports 120Hz
  GET_CONSOLE_HFR_SUPPORTED,

  // Collects a dump of all gpu resources currently in memory to a vector of ResourceDumpInfo structs
  // (It works similarly to GET_TEXTURE_STATISTICS, however instead of it dumping a bunch of text
  // it dumps it into the same vector of structs across all drivers)
  // par1: pointer to a Tab<ResourceDumpInfo> object
  GET_RESOURCE_STATISTICS,

  // helper for low level APIs (vulkan) to improve barrier tracking on batched workloads
  // delay disables barrier generation till continue is called
  // when continue is called, recorded work is analyzed to place barriers at point of time when delay was called
  // this way batched workload is free from barriers,
  // while tracking is still properly performed and proper barriers generated
  //
  // for simplicity
  //  - nesting delay-continue pairs is not allowed
  //  - delay-continue pair must be completed before flushing/submitting workload to GPU
  DELAY_SYNC,
  CONTINUE_SYNC,

  //
  // Changes GPU queue/pipeline where order dependant action commands will be submitted
  // i.e. dispatches, draws, and mostly any commands that execute work on GPU and require GPU lock
  // Represents queue+command buffer combo of low level graphics APIs
  // Must be called under GPU lock
  // Effect of this command do not "revert" back on some automatic basis, yet
  // device reset/queue flush restores queue to default (graphics)
  // GpuPipeline parameters of d3d:: API calls are ignored when this command is in effect
  //
  // par1: GpuPipeline - target queue/pipeline where commands will be submitted after this command
  //
  // returns 1 if driver switched queue, otherwise returns 0
  CHANGE_QUEUE,

  // Returns whether PIX Capturer is loaded
  IS_PIX_CAPTURE_LOADED,

  USER = 1000,
};

enum
{
  DRV3D_CPU_FREQ_TYPE_QPC,     // QueryPerformanceCounter / QueryPerformanceFrequency
  DRV3D_CPU_FREQ_TYPE_REF,     // ref_time_ticks / ref_ticks_frequency. Often same as DRV3D_CPU_FREQ_TYPE_QPC (but not always, see
                               // implementation!)
  DRV3D_CPU_FREQ_NSEC,         // timestamp is nanoseconds, clock_gettime / 1000000000ull
  DRV3D_CPU_FREQ_TYPE_PROFILE, // profile_ref_ticks / profile_ticks_frequency
  DRV3D_CPU_FREQ_TYPE_UNKNOWN,
};

// All values are in ticks
struct Drv3dTimings
{
  long long frontendUpdateScreenInterval;
  long long frontendToBackendUpdateScreenLatency;
  long long frontendBackendWaitDuration;
  long long backendFrontendWaitDuration;
  long long gpuWaitDuration;
  long long presentDuration;
  long long backbufferAcquireDuration;
  long long frontendWaitForSwapchainDuration;
};

/// All values are optional, but graphics and mesh pipelines can not be created without output and render state sets.
struct CompilePipelineSet
{
  /// V1 and V2 value. When not specified, the driver assumes its driver specific format (if supported) and otherwise engine.
  const char *defaultFormat;
  /// V1 and V2 block. A block with a set of feature sets referenced by pipelines in the pipeline sets to indicate required features.
  const DataBlock *featureSet;
  /// V1 and V2 block. A block with a set of blocks describing input layouts.
  const DataBlock *inputLayoutSet;
  /// V1 and V2 block. A block with a set of blocks describing render states. The driver will ignore render states that are
  /// incompatible with the system.
  const DataBlock *renderStateSet;
  /// V1 and V2 block. A block with a set of blocks describing output format states. The driver will ignore output format states that
  /// are incompatible wit the system.
  const DataBlock *outputFormatSet;
  /// V1 block. A block with a set of blocks describing graphics pipelines. The driver will ignore pipelines using unsupported input
  /// layouts, render states or output formats.
  const DataBlock *graphicsPipelineSet;
  /// V1 block. A block with a set of blocks describing mesh pipelines. The driver will ignore pipelines using unsupported render
  /// states or output formats.
  const DataBlock *meshPipelineSet;
  /// V1 block. A block with a set of blocks describing compute pipelines.
  const DataBlock *computePipelineSet;
  /// V2 block, this block stores shader class signatures, this ensures we try not to load incompatible shader class uses.
  const DataBlock *signature;
  /// V2 block, new compute use set, uses shader class names and codes.
  const DataBlock *computeSet;
  /// V2 block, new graphics use set, uses shader class names and codes.
  const DataBlock *graphicsSet;
  /// V2 block, new graphics use set, uses shader class names and codes. This set has pipelines where the paired pixel shader is
  /// replaced by the null pixel shader.
  const DataBlock *graphicsNullPixelOverrideSet;
  /// V2 block, new graphics use set, uses shader class names and codes. This set has pipelines where the paired pixel shader uses a
  /// differen shader class name and codes than the vertex shader.
  const DataBlock *graphicsPixelOverrideSet;
};

enum ResourceBarrier : int;

struct Drv3dMakeTextureParams
{
  void *tex;
  const char *name;
  int flg;
  int w, h, layers, mips;
  ResourceBarrier currentState;
};

namespace d3d
{
/// send specific command to driver
int driver_command(Drv3dCommand command, void *par1 = nullptr, void *par2 = nullptr, void *par3 = nullptr);
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline int driver_command(Drv3dCommand command, void *p1, void *p2, void *p3) { return d3di.driver_command(command, p1, p2, p3); }
} // namespace d3d
#endif
