#include <3d/dag_nvLowLatency.h>

#include <3d/dag_drv3dReset.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include "nvLowLatency.h"

#include <debug/dag_debug.h>

#include <d3d11.h>
#include <nvapi.h>
// TODO re-enable ReflexStats after it works properly on win32 as well
// #include <reflexstats.h>

static IUnknown *dx_device = nullptr;
static bool deferred_submission = false;
static bool latency_indicator_enabled = false;

// NVSTATS_DEFINE()

bool nvlowlatency::feed_latency_input(uint32_t frame_id, unsigned int msg)
{
  G_UNUSED(frame_id);
  G_UNUSED(msg);
  return false;
  // bool pingMsg = NVSTATS_IS_PING_MSG_ID(msg);
  // if (pingMsg)
  //   NVSTATS_MARKER(NVSTATS_PC_LATENCY_PING, frame_id);
  // return pingMsg;
}

void nvlowlatency::init(void *device, bool deferred_render_submission)
{
  latency_indicator_enabled = false;
  deferred_submission = deferred_render_submission;
  dx_device = nullptr;
  NV_GET_SLEEP_STATUS_PARAMS_V1 params = {};
  params.version = NV_GET_SLEEP_STATUS_PARAMS_VER1;
  NvAPI_Status status = NvAPI_D3D_GetSleepStatus(static_cast<IUnknown *>(device), &params);
  if (status == NVAPI_OK)
    dx_device = static_cast<IUnknown *>(device);
  if (dx_device)
  {
    NvU32 driverVersion;
    NvAPI_ShortString driverAndBranchString;
    if (NvAPI_SYS_GetDriverAndBranchVersion(&driverVersion, driverAndBranchString) == NVAPI_OK)
    {
      if (driverVersion >= 51179)
      {
        // Older driver versions are not compatible with the flash indicator's api of this Reflex SDK version
        latency_indicator_enabled = true;
      }
    }
    debug("NV low latency initialized successfully, driver version:%d, latency flash indicator:%s", driverVersion,
      latency_indicator_enabled ? "on" : "off");
    // NVSTATS_INIT(0, 0) //-V513
  }
  else
    debug("NV low latency unavailable");
}

void nvlowlatency::close()
{
  if (dx_device)
  {
    debug("NV low latency closed");
    dx_device = nullptr;
    // NVSTATS_SHUTDOWN()
  }
}

bool nvlowlatency::is_available() { return dx_device != nullptr; }

void nvlowlatency::set_latency_mode(lowlatency::LatencyMode mode, float min_interval_ms)
{
  if (dx_device)
  {
    NV_SET_SLEEP_MODE_PARAMS_V1 params = {};
    params.version = NV_SET_SLEEP_MODE_PARAMS_VER1;
    switch (mode)
    {
      case lowlatency::LATENCY_MODE_OFF:
        params.bLowLatencyMode = false;
        params.bLowLatencyBoost = false;
        break;
      case lowlatency::LATENCY_MODE_NV_ON:
        params.bLowLatencyMode = true;
        params.bLowLatencyBoost = false;
        break;
      case lowlatency::LATENCY_MODE_NV_BOOST:
      case lowlatency::LATENCY_MODE_EXPERIMENTAL:
        params.bLowLatencyMode = true;
        params.bLowLatencyBoost = true;
        break;
    }
    params.minimumIntervalUs = static_cast<uint32_t>(min_interval_ms * 1000.f);
    debug("[NV] Set latency mode: latency mode: %d, boost: %d, interval: %dus", params.bLowLatencyMode, params.bLowLatencyBoost,
      params.minimumIntervalUs);
    NvAPI_Status status = NvAPI_D3D_SetSleepMode(dx_device, &params);
    static bool firstError = true;
    if (status != NVAPI_OK && firstError)
    {
      logerr("NVAPI: NvAPI_D3D_SetSleepMode has failed");
      firstError = false;
    }
  }
}

void nvlowlatency::sleep()
{
  if (dx_device)
  {
    TIME_D3D_PROFILE(nv_low_latency_sleep);
    NvAPI_Status status = NvAPI_D3D_Sleep(dx_device);

    static bool firstError = true;
    if (status != NVAPI_OK && firstError)
    {
      logerr("NVAPI: NvAPI_D3D_Sleep has failed");
      firstError = false;
    }
  }
}

static NV_LATENCY_MARKER_TYPE convertMarkerTypeToNV(lowlatency::LatencyMarkerType marker_type)
{
  switch (marker_type)
  {
    case lowlatency::LatencyMarkerType::SIMULATION_START: return NV_LATENCY_MARKER_TYPE::SIMULATION_START;
    case lowlatency::LatencyMarkerType::SIMULATION_END: return NV_LATENCY_MARKER_TYPE::SIMULATION_END;
    case lowlatency::LatencyMarkerType::RENDERSUBMIT_START:
    case lowlatency::LatencyMarkerType::RENDERRECORD_START: return NV_LATENCY_MARKER_TYPE::RENDERSUBMIT_START;
    case lowlatency::LatencyMarkerType::RENDERSUBMIT_END:
    case lowlatency::LatencyMarkerType::RENDERRECORD_END: return NV_LATENCY_MARKER_TYPE::RENDERSUBMIT_END;
    case lowlatency::LatencyMarkerType::PRESENT_START: return NV_LATENCY_MARKER_TYPE::PRESENT_START;
    case lowlatency::LatencyMarkerType::PRESENT_END: return NV_LATENCY_MARKER_TYPE::PRESENT_END;
    case lowlatency::LatencyMarkerType::INPUT_SAMPLE_FINISHED: return NV_LATENCY_MARKER_TYPE::INPUT_SAMPLE;
    case lowlatency::LatencyMarkerType::TRIGGER_FLASH: return NV_LATENCY_MARKER_TYPE::TRIGGER_FLASH;
  }
  G_ASSERT_RETURN(false, NV_LATENCY_MARKER_TYPE::SIMULATION_START);
}

void nvlowlatency::set_marker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type)
{
  if (dx_device)
  {
    if (marker_type == lowlatency::LatencyMarkerType::TRIGGER_FLASH && !latency_indicator_enabled)
      return;
    if (deferred_submission && (marker_type == lowlatency::LatencyMarkerType::RENDERRECORD_START ||
                                 marker_type == lowlatency::LatencyMarkerType::RENDERRECORD_END))
      return;
    NV_LATENCY_MARKER_PARAMS_V1 params = {};
    NV_LATENCY_MARKER_TYPE makerType = convertMarkerTypeToNV(marker_type);
    params.markerType = makerType;
    params.version = NV_LATENCY_MARKER_PARAMS_VER1;
    params.frameID = frame_id;
    NvAPI_Status status = NvAPI_D3D_SetLatencyMarker(dx_device, &params);
    // NVSTATS_MARKER(makerType, frame_id);
    static bool firstError = true;
    if (status != NVAPI_OK && firstError)
    {
      logerr("NVAPI: SetLatencyMarked has failed with frame_id: %d, and marker: %d", frame_id, static_cast<uint32_t>(marker_type));
      firstError = false;
    }
  }
}

static bool is_report_valid(const NV_LATENCY_RESULT_PARAMS_V1::FrameReport &report)
{
  NvU64 maxDifference = 16667 * 100; // 100 frames
  const NvU64 startTime = report.simStartTime;
  if (report.simEndTime < startTime || report.simEndTime - startTime > maxDifference)
    return false;
  if (report.renderSubmitStartTime < startTime || report.renderSubmitStartTime - startTime > maxDifference)
    return false;
  if (report.renderSubmitEndTime < startTime || report.renderSubmitEndTime - startTime > maxDifference)
    return false;
  if (report.driverStartTime < startTime || report.driverStartTime - startTime > maxDifference)
    return false;
  if (report.driverEndTime < startTime || report.driverEndTime - startTime > maxDifference)
    return false;
  if (report.osRenderQueueStartTime < startTime || report.osRenderQueueStartTime - startTime > maxDifference)
    return false;
  if (report.osRenderQueueEndTime < startTime || report.osRenderQueueEndTime - startTime > maxDifference)
    return false;
  if (report.gpuRenderStartTime < startTime || report.gpuRenderStartTime - startTime > maxDifference)
    return false;
  if (report.gpuRenderEndTime < startTime || report.gpuRenderEndTime - startTime > maxDifference)
    return false;
  return true;
}

lowlatency::LatencyData nvlowlatency::get_statistics_since(uint32_t frame_id, uint32_t max_count)
{
  lowlatency::LatencyData ret;
  if (dx_device)
  {
    NV_LATENCY_RESULT_PARAMS_V1 params = {};
    params.version = NV_LATENCY_RESULT_PARAMS_VER1;
    NvAPI_Status status = NvAPI_D3D_GetLatency(dx_device, &params);
    if (status != NVAPI_OK)
      return ret; // frameCount = 0
    constexpr uint32_t FRAME_COUNT = sizeof(params.frameReport) / sizeof(params.frameReport[0]);
    if (max_count == 0 || max_count > FRAME_COUNT)
      max_count = FRAME_COUNT;
    G_STATIC_ASSERT(FRAME_COUNT == 64); // FRAME_COUNT should be 64 elements long. Check if the
                                        // calculation is correct

    for (uint32_t i = FRAME_COUNT; i > FRAME_COUNT - max_count && params.frameReport[i - 1].frameID > frame_id; --i)
    {
      const NV_LATENCY_RESULT_PARAMS_V1::FrameReport &report = params.frameReport[i - 1];
      if (!is_report_valid(report))
        continue;
      const NvU64 startTime = report.simStartTime;
      const float gpuRenderEndTime = (report.gpuRenderEndTime - startTime) / 1000.f;
      const float renderSubmitEndTime = (report.renderSubmitEndTime - startTime) / 1000.f;
      const float osRenderQueueStartTime = (report.osRenderQueueStartTime - startTime) / 1000.f;

      ret.gameToRenderLatency.apply(gpuRenderEndTime);
      ret.gameLatency.apply(renderSubmitEndTime);
      ret.renderLatency.apply(gpuRenderEndTime - osRenderQueueStartTime);

      ret.inputSampleTime.apply((report.inputSampleTime - startTime) / 1000.f);
      ret.simEndTime.apply((report.simEndTime - startTime) / 1000.f);
      ret.renderSubmitStartTime.apply((report.renderSubmitStartTime - startTime) / 1000.f);
      ret.renderSubmitEndTime.apply(renderSubmitEndTime);
      ret.presentStartTime.apply((report.presentStartTime - startTime) / 1000.f);
      ret.presentEndTime.apply((report.presentEndTime - startTime) / 1000.f);
      ret.driverStartTime.apply((report.driverStartTime - startTime) / 1000.f);
      ret.driverEndTime.apply((report.driverEndTime - startTime) / 1000.f);
      ret.osRenderQueueStartTime.apply(osRenderQueueStartTime);
      ret.osRenderQueueEndTime.apply((report.osRenderQueueEndTime - startTime) / 1000.f);
      ret.gpuRenderStartTime.apply((report.gpuRenderStartTime - startTime) / 1000.f);
      ret.gpuRenderEndTime.apply(gpuRenderEndTime);
      ret.frameCount++;
    }
    ret.gameToRenderLatency.finalize(ret.frameCount);
    ret.gameLatency.finalize(ret.frameCount);
    ret.renderLatency.finalize(ret.frameCount);
    ret.inputSampleTime.finalize(ret.frameCount);
    ret.simEndTime.finalize(ret.frameCount);
    ret.renderSubmitStartTime.finalize(ret.frameCount);
    ret.renderSubmitEndTime.finalize(ret.frameCount);
    ret.presentStartTime.finalize(ret.frameCount);
    ret.presentEndTime.finalize(ret.frameCount);
    ret.driverStartTime.finalize(ret.frameCount);
    ret.driverEndTime.finalize(ret.frameCount);
    ret.osRenderQueueStartTime.finalize(ret.frameCount);
    ret.osRenderQueueEndTime.finalize(ret.frameCount);
    ret.gpuRenderStartTime.finalize(ret.frameCount);
    ret.gpuRenderEndTime.finalize(ret.frameCount);
  }

  return ret;
}
