//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

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

enum
{
  DRV3D_COMMAND_GETALLTEXS,

  DRV3D_COMMAND_SET_STAT3D_HANDLER,

  DRV3D_COMMAND_GETTEXTUREMEM,

  DRV3D_COMMAND_GETVISIBILITYBEGIN,
  DRV3D_COMMAND_GETVISIBILITYEND,
  DRV3D_COMMAND_GETVISIBILITYCOUNT,
  DRV3D_COMMAND_RELEASE_QUERY,

  DRV3D_COMMAND_ENABLEDEBUGTEXTURES,
  DRV3D_COMMAND_DISABLEDEBUGTEXTURES,

  //! enable/disable restricted multithreaded access to D3D (calls are nest-counted)
  DRV3D_COMMAND_ENABLE_MT,
  DRV3D_COMMAND_DISABLE_MT,

  //! acquire/release D3D lock (to perform rendering in non-primary thread)
  DRV3D_COMMAND_ACQUIRE_OWNERSHIP,
  DRV3D_COMMAND_RELEASE_OWNERSHIP,

  //! acquire/release a mutex to perform resource loading in non-primary thread and postpone GPU resets, the lock is nonexclusive.
  DRV3D_COMMAND_ACQUIRE_LOADING,
  DRV3D_COMMAND_RELEASE_LOADING,

  //! preallocates render targets, par1= &DataBlock
  DRV3D_COMMAND_PREALLOCATE_RT,

  //! suspend/resume D3D (wrappers for xbox360 D3D Suspend() and Resume())
  DRV3D_COMMAND_SUSPEND,
  DRV3D_COMMAND_RESUME,
  //! returns current suspend state (>0 - frame is suspended, 0 - ordinary rendering)
  DRV3D_COMMAND_GET_SUSPEND_COUNT,

  //! get AA level for RT (par1==0) or backbuf (par1==1)
  DRV3D_COMMAND_GET_AA_LEVEL,

  DRV3D_COMMAND_GET_GPU_FRAME_TIME,

  DRV3D_COMMAND_GET_VSYNC_REFRESH_RATE,

  // Enter / leave critical section on resource lock.
  DRV3D_COMMAND_ENTER_RESOURCE_LOCK_CS,
  DRV3D_COMMAND_LEAVE_RESOURCE_LOCK_CS,

  // set time limit (in usecs) per frame for pipeline compilation
  DRV3D_COMMAND_SET_PIPELINE_COMPILATION_TIME_BUDGET,
  // recives current amount of pipelines in compilation queue
  DRV3D_COMMAND_GET_PIPELINE_COMPILATION_QUEUE_LENGTH,

  DRV3D_COMMAND_INVALIDATE_STATES,

  D3V3D_COMMAND_TIMESTAMPFREQ,
  D3V3D_COMMAND_TIMESTAMPISSUE,
  D3V3D_COMMAND_TIMESTAMPGET,

  //! Gets CPU and GPU time nearly at the same moment
  // par1 - cpu timestamp
  // par2 - gpu timestamp
  // par3 (optional) - cpu frequency type (see below, DRV3D_CPU_FREQ_TYPE_*)
  // Warning! with time timestamp _will_ deviate. We need to recalibrate clock from time to time!
  D3V3D_COMMAND_TIMECLOCKCALIBRATION,


  DRV3D_COMMAND_FLUSH_STATES, // flush all states, except buffer. will resolve msaa, generate mips, etc - if supported!
  DRV3D_COMMAND_D3D_FLUSH,    // ID3D11DeviceContext::Flush or IDirect3DQuery9::GetData(D3DGETDATA_FLUSH).

  //! flush pipeline states to compile pipeline
  // par1: uintptr_t that stores ShaderStage
  //       if par1 == STAGE_PS, compiles graphics pipeline
  //       if par1 == STAGE_CS, compiles compute pipeline
  // par2: uintptr_t that stores topology if par1 == ShaderStage::STAGE_PS
  DRV3D_COMMAND_COMPILE_PIPELINE,

  DRV3D_COMMAND_GET_SECONDARY_BACKBUFFER,

  DRV3D_COMMAND_GET_VENDOR,

  DRV3D_COMMAND_GET_RESOLUTION,

  DRV3D_COMMAND_OVERRIDE_MAX_ANISOTROPY_LEVEL,

  // Creates an engine texture object from a raw resource
  DRV3D_COMMAND_MAKE_TEXTURE,
  DRV3D_COMMAND_GET_TEXTURE_HANDLE,

  // Start / Finish exclusive access for working with graphical resources and rendering context.
  // It is useful in cases when part of some rendering is performed beyond the borders of the driver
  DRV3D_COMMAND_BEGIN_EXTERNAL_ACCESS,
  DRV3D_COMMAND_END_EXTERNAL_ACCESS,

  DRV3D_COMMAND_SET_VS_DEBUG_INFO,
  DRV3D_COMMAND_SET_PS_DEBUG_INFO,

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
  // to set the index before commiting with DRV3D_COMMAND_END_MRT_CLEAR_SEQUENCE.
  // Each clearview after this incrments the index of the next render target clear
  // color.
  // Param2 is an optinal parameter to an uint32_t that is set to 1 if the backend
  // implements mrt clear sequence support.
  DRV3D_COMMAND_BEGIN_MRT_CLEAR_SEQUENCE,
  // Ends a mrt clear sequence. The next command that triggers a clear will then
  // use the color values (and depth stencil values for color target 0) to do
  // the clear the currently set render targets.
  // Param1 is used as a hint if a immediate clear with this call is required or not,
  // if Param1 is NULL then a clear with this call is executed, if Param1 is not NULL
  // then the clear is delayed as long as possible.
  DRV3D_COMMAND_END_MRT_CLEAR_SEQUENCE,
  // Queries the selected render dervices driver cache format uuid.
  // First parameter has to be a pointer to a 128 byte big memory block to store the uuid.
  // All other parameters are unused.
  // Returns 1 if the uuid was written to the first parameter.
  DRV3D_GET_SHADER_CACHE_UUID,

  DRV3D_COMMAND_AFTERMATH_MARKER, // Nvidia Aftermath marker. Pointer must be valid until the crash. Does nothing when Aftermath is
                                  // disabled.

  DRV3D_COMMAND_START_CAPTURE_FRAME,

  DRV3D_COMMAND_IS_HDR_AVAILABLE,
  DRV3D_COMMAND_IS_HDR_ENABLED,
  DRV3D_COMMAND_INT10_HDR_BUFFER,
  DRV3D_COMMAND_HDR_OUTPUT_MODE,
  DRV3D_COMMAND_GET_LUMINANCE,

  DRV3D_COMMAND_MEM_STAT, // Single line tex and buffers statistics.

  // Lets the D3D backend know about a ScriptedShadersBinDumpOwner that has just been loaded.
  // This command allows the backend to prepare all shaders of this bin dump for faster loading,
  // including usage of caches of previous runs of the application.
  //
  // par1 is a pointer to the ScriptedShadersBinDumpOwner
  DRV3D_COMMAND_REGISTER_SHADER_DUMP,

  // par1 is external shader id (shader number in the dump)
  // par2 is shader stage (ShaderCodeType)
  // [out] par3 is pointer to internal shader id (if driver doesn't support this command, then par3 remains unchanged)
  DRV3D_COMMAND_GET_SHADER,

  DRV3D_COMMAND_PIX_GPU_BEGIN_CAPTURE,
  DRV3D_COMMAND_PIX_GPU_END_CAPTURE,
  DRV3D_COMMAND_PIX_GPU_CAPTURE_NEXT_FRAMES,
  DRV3D_COMMAND_PIX_GPU_CAPTURE_AFTER_LONG_FRAMES,

  // Ask driver by what factor it was limited when rendering last frame
  DRV3D_COMMAND_GET_FRAMERATE_LIMITING_FACTOR,

  // Loads additional pipeline cache from path @par1
  DRV3D_COMMAND_LOAD_PIPELINE_CACHE,
  // Saves current pipeline cache on FS.
  DRV3D_COMMAND_SAVE_PIPELINE_CACHE,

  // Create the device that can render loading splash before d3d is fully initialized. Requires initialization of relevent settings.
  DRV3D_COMMAND_CREATE_SPLASH_DEVICE,

  // Returns DLSS state as a DlssState struct casted to int. No parameters required
  DRV3D_COMMAND_GET_DLSS_STATE,

  // Returns DLSS state as a DlssState struct casted to int. No parameters required
  DRV3D_COMMAND_GET_XESS_STATE,

  // Returns if a DLSS quality is available at a target resolution
  // par1: IPoint2* that contains target resolution width and height
  // par2: DLSS quality as int*
  // par3: bool* for: is RTX raytracing on?
  DRV3D_COMMAND_IS_DLSS_QUALITY_AVAILABLE_AT_RESOLUTION,

  // Returns if a XeSS quality is available at a target resolution
  // par1: IPoint2* that contains target resolution width and height
  // par2: XeSS quality as int*
  DRV3D_COMMAND_IS_XESS_QUALITY_AVAILABLE_AT_RESOLUTION,

  // Return current DLSS rendering resolution in par1 and par2
  // par1: int *width
  // par2: int *height
  DRV3D_COMMAND_GET_DLSS_RESOLUTION,

  // Return current XESS rendering resolution in par1 and par2
  // par1: int *width
  // par2: int *height
  DRV3D_COMMAND_GET_XESS_RESOLUTION,

  // Executes DLSS
  // par1: DlssParams *
  // par2: int *view_index
  DRV3D_COMMAND_EXECUTE_DLSS,

  // Executes DLSS
  // par1: XessParams *
  DRV3D_COMMAND_EXECUTE_XESS,

  // Set velocity scaling for XeSS,
  // par1: float *x
  // par2: float *y
  DRV3D_COMMAND_SET_XESS_VELOCITY_SCALE,

  // Execute FSR2
  // par1: Fsr2Params *
  DRV3D_COMMAND_EXECUTE_FSR2,

  // Returns current FSR2 state as Fsr2State casted to int. No parameters required
  DRV3D_COMMAND_GET_FSR2_STATE,

  // Return current FSR2 rendering resolution in par1 and par2
  // par1: int *width
  // par2: int *height
  DRV3D_COMMAND_GET_FSR2_RESOLUTION,

  // Returns state of MetalFX upscale availability (int)
  DRV3D_COMMAND_GET_METALFX_UPSCALE_STATE,

  // Execute MetalFX upscale
  // par1: MtlFxUpscaleParams *
  DRV3D_COMMAND_EXECUTE_METALFX_UPSCALE,

  // Queries the current timing statistics of the driver.
  // par1: A pointer to a Drv3dTimings variable to receive the timing data.
  // par2: Optional intptr_t (not a pointer!) as an index into the list of past timings
  //       0 is current, 1 is one frame in the past and so on.
  // Return: Number of timings available.
  DRV3D_COMMAND_GET_TIMINGS,

  // Simple command, prints the string into the debug log.
  // The message is printed as the driver executes the command stream it might has recorded.
  // This command is designed primarily for postmortem log analysis, to have a orientation point
  // for other messages, to easier track down locations of interest.
  // Takes up to 3 parameters:
  // First is the string, second is the string length as intptr_t (<=0 means use strlen),
  // and third is severity level as intptr_t (0-debug, 1-warning, 2-error).
  DRV3D_COMMAND_DEBUG_MESSAGE,

  // Returns unique, identifiable names of active monitors (eg. "\\.\DISPLAY1"), which are attached to the active adapter
  // par1: Tab<String> *monitorNames
  DRV3D_COMMAND_GET_MONITORS,

  // Returns some user-friendly info about a monitor (friendly name, unique index)
  // par1: const char *uniqueMonitorName
  // par1: String *friendlyMonitorName (optional)
  // par2: int *uniqueMonitorIndex (optional)
  DRV3D_COMMAND_GET_MONITOR_INFO,

  // Returns available resolutions for a monitor
  // par1: Tab<String> *monitorNames
  // par2: Tab<String> *availableResolutions
  DRV3D_COMMAND_GET_RESOLUTIONS_FROM_MONITOR,

  // Returns video memory budget in kb
  // par1: optional: current memory available
  // par2: optional: current memory used
  // Return: amount of memory available at game start
  DRV3D_COMMAND_GET_VIDEO_MEMORY_BUDGET,

  // Returns a handle for the platform specific graphics command queue
  // par1: void **handleHolder
  DRV3D_COMMAND_GET_RENDERING_COMMAND_QUEUE,

  // Returns the corresponding vulkan objects on par1
  DRV3D_COMMAND_GET_INSTANCE,
  DRV3D_COMMAND_GET_PHYSICAL_DEVICE,
  DRV3D_COMMAND_GET_QUEUE_FAMILY_INDEX,
  DRV3D_COMMAND_GET_QUEUE_INDEX,

  // Should be called when work cycle update loop decides
  // that no drawing will be performed due to app being hidden/inactive/minimized
  DRV3D_COMMAND_PROCESS_APP_INACTIVE_UPDATE,

  // Starts special BB rotate transform pass
  DRV3D_COMMAND_PRE_ROTATE_PASS,

  // par1 is a pointer to an os_event_t, which is going to be
  // signaled once the rendering of the current frame is completed
  // on the GPU.
  DRV3D_COMMAND_REGISTER_FRAME_COMPLETION_EVENT,

  // par1 is a pointer to a FrameEvents object.
  // par2 is understood as a boolean, specifying if events should coming from frontend or backend
  DRV3D_COMMAND_REGISTER_ONE_TIME_FRAME_EXECUTION_EVENT_CALLBACKS,

  // The driver will report all resource state transitions, of the given resource, to the debug log.
  // par1 pointer to resource to be tracked.
  // Returns 1 if the driver supports this command.
  DRV3D_COMMAND_REPORT_RESOURCE_STATE_TRANSITIONS,

  // par1 is a pointer to a DeviceResetEventHandler object.
  DRV3D_COMMAND_REGISTER_DEVICE_RESET_EVENT_HANDLER,
  DRV3D_COMMAND_UNREGISTER_DEVICE_RESET_EVENT_HANDLER,

  // Tells the driver to transition the textures passed into a state that it is consumeable by VR
  // runtimes. This should be RB_RW_RENDER_TARGET or RB_RW_DEPTH_STENCIL_TARGET.
  // par1 is an array of BaseTexture* pointers.
  // par2 is the number of textures in the array.
  DRV3D_COMMAND_PREPARE_TEXTURES_FOR_VR_CONSUMPTION,

  // Informs driver about current app name & version/build number
  // par1 is app name string,
  // par2 is app ver uint32_t
  DRV3D_COMMAND_SET_APP_INFO,

  // Uses the provided data to simulate a GPU crash, where we capture a dump and send it to logs server.
  // Logs send by this will have a extra meta data field "was-manually-send" set to true.
  // par1: const char *dump_type: currently only "nv-gpudmp" is supported, any other will be ignored.
  // par2: const void *dump_data: pointer to dump data to use.
  // par3: uintptr_t dump_data_size: size of the data of 'dump_data'.
  DRV3D_COMMAND_SEND_GPU_CRASH_DUMP,

  // Allows telling driver to perform needed perf-frame update (when used from wait cycle on main thread)
  // return 1 when pending ops performed, 0 otherwise
  DRV3D_COMMAND_PROCESS_PENDING_RESOURCE_UPDATED,

  DRV3D_COMMAND_GET_PS5_HFR_STATUS,

  DRV3D_COMMAND_SET_FREQ_LEVEL,

  // For debug purpose.
  // If this returns true then the render thread will be wait after each draw and dispatch calls.
  // This helps investigating which command causes error on GPU.
  DRV3D_COMMAND_ENABLE_IMMEDIATE_FLUSH,
  DRV3D_COMMAND_DISABLE_IMMEDIATE_FLUSH,

  DRV3D_COMMAND_GET_WORKER_CPU_CORE,

  // The PS5 driver returns the index of the current backbuffer in the swapchain.
  // This is needed for PSVR2.
  DRV3D_COMMAND_GET_PS5_BACK_BUFFER_INDEX,

  // The PS5 swapchain needs this value to synchronize flipping with PSVR2. It is reset after each flip.
  DRV3D_COMMAND_SET_PS5_FLIPARG,

  // The PS5 swapchain needs to be recreated when switcing between VR and flatscreen.
  DRV3D_COMMAND_RESET_PS5_SWAPCHAIN_VR_MODE,

  // It tells Swappy on android, what is the target frame rate.
  // par1: int* frame rate
  DRV3D_COMMAND_SWAPPY_SET_TARGET_FRAME_RATE,
  // par1: int* status ok\failed to setup frame rate
  // status would be written later on by worker thread
  DRV3D_COMMAND_SWAPPY_GET_STATUS,
  // par1: bool enable/disable HDR
  DRV3D_COMMAND_SET_HDR,

  // This raises a debug break event / signal in the driver when it is executed.
  // Drivers with backend workers this will be executed in that thread.
  // This command allows to debug break into the backend to help with issues during execution of d3d commands.
  DRV3D_COMMAND_DEBUG_BREAK,
  // Adds a string that is searched in the resolved call stack string and if found a debug break event is raised.
  DRV3D_COMMAND_ADD_DEBUG_BREAK_STRING_SEARCH,
  // Removes a string previously added with DRV3D_COMMAND_ADD_DEBUG_BREAK_STRING_SEARCH
  DRV3D_COMMAND_REMOVE_DEBUG_BREAK_STRING_SEARCH,

  DRV3D_COMMAND_USER = 1000,
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

enum ResourceBarrier : int;

struct Drv3dMakeTextureParams
{
  void *tex;
  const char *name;
  int flg;
  int w, h, layers, mips;
  ResourceBarrier currentState;
};

class D3dEventQuery;

#if !_TARGET_D3D_MULTI
namespace d3d
{
typedef D3dEventQuery EventQuery;

// May return NULL if not supported.
EventQuery *create_event_query();

void release_event_query(EventQuery *query);

// Returns true if successfully issued.
bool issue_event_query(EventQuery *query);

// Returns false when query is issued but not yet signaled (waiting).
// Returns true otherwise - signaled, not issued, or bad query.
bool get_event_query_status(EventQuery *query, bool force_flush);


/// send specific command to driver (see dag_drv3dCmd.h)
int driver_command(int command, void *par1, void *par2, void *par3);
} // namespace d3d
#else
#include <3d/dag_drv3d_multi.h>
#endif

namespace d3d
{
struct GpuAutoLock
{
  GpuAutoLock();
  ~GpuAutoLock();
};

struct LoadingAutoLock // Non-exclusive lock to protect a thread from GPU reset.
{
  LoadingAutoLock() { driver_command(DRV3D_COMMAND_ACQUIRE_LOADING, NULL, NULL, NULL); }
  ~LoadingAutoLock() { driver_command(DRV3D_COMMAND_RELEASE_LOADING, NULL, NULL, NULL); }
};

// scoped conditional GPU workload split
struct GPUWorkloadSplit
{
  bool needSplitAtEnd;

  GPUWorkloadSplit(bool do_split, bool split_at_end, const char *marker) : needSplitAtEnd(do_split && split_at_end)
  {
    if (!do_split)
      return;
    d3d::driver_command(DRV3D_COMMAND_D3D_FLUSH, (void *)marker, (void *)0, NULL);
  }

  ~GPUWorkloadSplit()
  {
    if (!needSplitAtEnd)
      return;

    d3d::driver_command(DRV3D_COMMAND_D3D_FLUSH, (void *)"split_end", (void *)1, NULL);
  }
};
}; // namespace d3d
