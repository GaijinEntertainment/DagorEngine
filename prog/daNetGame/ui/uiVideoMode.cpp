// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqrat.h>

#include "uiVideoMode.h"
#include <sqmodules/sqmodules.h>

#include <util/dag_parseResolution.h>
#include <math/dag_mathUtils.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_delayedAction.h>
#include <generic/dag_sort.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_winVersionQuery.h>
#include <startup/dag_globalSettings.h>
#include <frameTimeMetrics/aggregator.h>

#include "main/main.h"
#include "main/settings.h"
#include "squirrel.h"
#include "ui/userUi.h"
#include <daRg/dag_guiScene.h>

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_nvFeatures.h>
#include <gui/dag_guiStartup.h>
#include <3d/dag_lowLatency.h>
#include <drv/3d/dag_platform.h>

#include <render/hdrRender.h>
#include <3d/dag_render.h>
#include <render/resolution.h>
#include <render/rendinstTessellation.h>
#include <main/settingsOverride.h>
#include <daGI2/settingSupport.h>
#include <render/world/bvh.h>
#include <render/antialiasing.h>
#include <render/autoGraphics.h>

namespace ui
{

namespace videomode
{

#if _TARGET_PC
static int compare_video_modes(const IPoint2 *a, const IPoint2 *b)
{
  int sx = sign(a->x - b->x);
  if (sx)
    return sx;
  return sign(a->y - b->y);
}
#endif

static inline void make_resolution(HSQUIRRELVM vm, Sqrat::Array &out, int index, int w, int h)
{
  Sqrat::Array resolution(vm, 2);
  resolution.SetValue(0, w);
  resolution.SetValue(1, h);
  out.SetValue(SQInteger(index), resolution);
}

static SQInteger get_video_modes(HSQUIRRELVM vm)
{
#if _TARGET_PC
  const char *displayName;
  sq_getstring(vm, SQInteger(2), &displayName);

  Tab<String> strList(framemem_ptr());

  if (!d3d::driver_command(Drv3dCommand::GET_RESOLUTIONS_FROM_MONITOR, &displayName, &strList))
    d3d::get_video_modes_list(strList); // fallback

  Tab<IPoint2> modes;
  modes.reserve(strList.size());

  const char *curResStr = ::dgs_get_settings()->getBlockByNameEx("video")->getStr("resolution", "auto");
  bool curResSpecified = false;

  int curW = 0, curH = 0;
  if (strcmp(curResStr, "auto") != 0 && get_resolution_from_str(curResStr, curW, curH))
    curResSpecified = true;

  const int minW = 1024, minH = 720;

  for (int i = 0; i < strList.size(); ++i)
  {
    int w = 0, h = 0;
    if (get_resolution_from_str(strList[i].str(), w, h))
    {
      if (w >= minW && h >= minH)
        modes.push_back(IPoint2(w, h));
    }
  }

  sort(modes, &compare_video_modes);

  Sqrat::Array arr(vm, 1 + modes.size());
  arr.SetValue(0, "auto");
  for (int i = 0; i < modes.size(); ++i)
  {
    Sqrat::Array resolution(vm, 2);
    resolution.SetValue(0, modes[i].x);
    resolution.SetValue(1, modes[i].y);
    arr.SetValue(SQInteger(i) + 1, resolution);
  }


  Sqrat::Table data(vm);
  data.SetValue("list", arr);

  if (curResSpecified)
  {
    Sqrat::Array arrCur(vm, 2);
    arrCur.SetValue(0, curW);
    arrCur.SetValue(1, curH);
    data.SetValue("current", arrCur);
  }
  else
    data.SetValue("current", "auto");

#else

  Sqrat::Array arr(vm, 2);
  arr.SetValue(0, "auto");
  arr.SetValue(1, "1920 x 1080");
  switch (get_console_model())
  {
    case ConsoleModel::PS4:
      make_resolution(vm, arr, 0, 1600, 900);
      make_resolution(vm, arr, 1, 1920, 1080);
      break;
    case ConsoleModel::PS4_PRO:
      make_resolution(vm, arr, 0, 2240, 1260);
      make_resolution(vm, arr, 1, 3200, 1800);
      break;
    case ConsoleModel::PS5:
      make_resolution(vm, arr, 0, 2560, 1440);
      make_resolution(vm, arr, 1, 3840, 2160);
      break;
    case ConsoleModel::PS5_PRO:
      make_resolution(vm, arr, 0, 3840, 2160);
      make_resolution(vm, arr, 1, 3840, 2160);
      break;
    case ConsoleModel::XBOXONE:
    case ConsoleModel::XBOXONE_S:
      make_resolution(vm, arr, 0, 1600, 900);
      make_resolution(vm, arr, 1, 1920, 1080);
      break;
    case ConsoleModel::XBOX_LOCKHART:
      make_resolution(vm, arr, 0, 2560, 1440);
      make_resolution(vm, arr, 1, 2560, 1440);
      break;
    case ConsoleModel::XBOXONE_X:
    case ConsoleModel::XBOX_ANACONDA:
      make_resolution(vm, arr, 0, 2560, 1440);
      make_resolution(vm, arr, 1, 3840, 2160);
      break;
    case ConsoleModel::NINTENDO_SWITCH: make_resolution(vm, arr, 0, 1280, 720); make_resolution(vm, arr, 1, 1920, 1080);
    default: break;
  }

  Sqrat::Table data(vm);
  data.SetValue("list", arr);
  data.SetValue("current", "auto");
#endif

  Sqrat::PushVar(vm, data);

  return 1;
}

static SQInteger get_available_monitors(HSQUIRRELVM vm)
{
#if _TARGET_PC
  Tab<String> strList(framemem_ptr());
  if (d3d::driver_command(Drv3dCommand::GET_MONITORS, &strList))
  {
    const char *displayName = ::dgs_get_settings()->getBlockByNameEx("video")->getStr("monitor", nullptr);
    if (!displayName)
      displayName = ::dgs_get_settings()->getBlockByNameEx("directx")->getStr("displayName", "auto");
    String curMonitor(displayName);

    if (curMonitor != "auto" && find_value_idx(strList, curMonitor) == -1)
      curMonitor = "auto";

    Sqrat::Array arr(vm, 1 + strList.size());
    arr.SetValue(0, "auto");
    for (int i = 0; i < strList.size(); ++i)
      arr.SetValue(SQInteger(i) + 1, strList[i].c_str());

    Sqrat::Table data(vm);
    data.SetValue("list", arr);
    data.SetValue("current", curMonitor.c_str());

    Sqrat::PushVar(vm, data);
    return 1;
  }
#endif

  Sqrat::Array arr(vm, 1);
  arr.SetValue(0, "auto");

  Sqrat::Table data(vm);
  data.SetValue("list", arr);
  data.SetValue("current", "auto");

  Sqrat::PushVar(vm, data);

  return 1;
}

static SQInteger get_monitor_info(HSQUIRRELVM vm)
{
  const char *displayName;
  sq_getstring(vm, SQInteger(2), &displayName);
#if _TARGET_PC
  if (displayName)
  {
    String friendlyMonitorName;
    int monitorId;
    if (d3d::driver_command(Drv3dCommand::GET_MONITOR_INFO, &displayName, &friendlyMonitorName, &monitorId))
    {
      bool isHdravailable =
        d3d::get_driver_code().is(d3d::dx12) && d3d::driver_command(Drv3dCommand::IS_HDR_AVAILABLE, (void *)displayName);

      Sqrat::Table data(vm);
      data.SetValue(SQInteger(0), friendlyMonitorName.c_str());
      data.SetValue(SQInteger(1), SQInteger(monitorId));
      data.SetValue(SQInteger(2), SQBool(isHdravailable));
      Sqrat::PushVar(vm, data);
      return 1;
    }
  }
#endif

  sq_pushnull(vm);
  return 1;
}

static void apply_video_settings_sq(Sqrat::Array changed_fields)
{
  FastNameMap nameMapChanges;
  for (SQInteger i = 0, n = changed_fields.Length(); i < n; ++i)
  {
    Sqrat::Object fieldNameObj = changed_fields.GetSlot(i);
    const char *fieldName = sq_objtostring(&fieldNameObj.GetObject());
    if (fieldName && !is_setting_overriden(fieldName))
      nameMapChanges.addNameId(fieldName);
  }

  auto applySettingsChange = [nmc = eastl::move(nameMapChanges)]() { apply_settings_changes(nmc); };
  run_action_on_main_thread(applySettingsChange);
}

static SQInteger is_fullscreen_enabled(HSQUIRRELVM vm)
{
  sq_pushbool(vm, dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE);
  return 1;
}

static SQInteger get_current_window_resolution(HSQUIRRELVM vm)
{
  int w = 0;
  int h = 0;
  d3d::get_screen_size(w, h);

  Sqrat::Array resolution(vm, 2);
  resolution.SetValue(0, w);
  resolution.SetValue(1, h);
  Sqrat::PushVar(vm, resolution);

  return 1;
}

static SQInteger get_antialiasing_options(HSQUIRRELVM vm)
{
  const char *options = render::antialiasing::get_available_methods(false, false);

  sq_pushstring(vm, options, -1);

  return 1;
}

static SQInteger set_frame_generation_suppressed(HSQUIRRELVM vm)
{
  SQBool suppressed;
  sq_getbool(vm, SQInteger(2), &suppressed);

  render::antialiasing::suppress_frame_generation(suppressed);

  return 1;
}

static SQInteger get_antialiasing_upscaling_options(HSQUIRRELVM vm)
{
  const char *method = nullptr;
  sq_getstring(vm, SQInteger(2), &method);

  const char *options = render::antialiasing::get_available_upscaling_options(method);

  sq_pushstring(vm, options, -1);

  return 1;
}

static SQInteger get_supported_generated_frames(HSQUIRRELVM vm)
{
  const char *method = nullptr;
  sq_getstring(vm, SQInteger(2), &method);

  SQBool exclusiveFullscreen = false;
  sq_getbool(vm, SQInteger(3), &exclusiveFullscreen);

  sq_pushinteger(vm, render::antialiasing::get_supported_generated_frames(method, exclusiveFullscreen));

  return 1;
}

static SQInteger get_frame_generation_unsupported_reason(HSQUIRRELVM vm)
{
  const char *method = nullptr;
  sq_getstring(vm, SQInteger(2), &method);

  SQBool exclusiveFullscreen = false;
  sq_getbool(vm, SQInteger(3), &exclusiveFullscreen);

  const char *reason = render::antialiasing::get_frame_generation_unsupported_reason(method, exclusiveFullscreen);

  if (reason)
    sq_pushstring(vm, reason, -1);
  else
    sq_pushnull(vm);

  return 1;
}

static SQInteger is_dlss_rr_supported(HSQUIRRELVM vm)
{
  nv::Streamline *streamline = nullptr;
  d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
  bool supported = streamline && streamline->isDlssRRSupported() == nv::SupportState::Supported;

  sq_pushbool(vm, supported);
  return 1;
}

static SQInteger get_low_latency_modes(HSQUIRRELVM vm)
{
  int m = lowlatency::get_supported_latency_modes();
  sq_pushinteger(vm, m);
  return 1;
}

static SQInteger get_performance_display_mode_support(HSQUIRRELVM vm)
{
  SQInteger mode_i;
  sq_getinteger(vm, 2, &mode_i);
  PerfDisplayMode mode = static_cast<PerfDisplayMode>(mode_i);
  switch (mode)
  {
    case PerfDisplayMode::OFF:
    case PerfDisplayMode::FPS: sq_pushinteger(vm, 1 /*true*/); break;
    case PerfDisplayMode::COMPACT:
    case PerfDisplayMode::FULL: sq_pushinteger(vm, lowlatency::is_available()); break;
  }
  return 1;
}

static SQInteger is_inline_rt_supported(HSQUIRRELVM vm)
{
  auto &caps = d3d::get_driver_desc().caps;
  bool inlineRTSupported = caps.hasRayQuery;
  sq_pushbool(vm, inlineRTSupported);
  return 1;
}

static SQInteger is_dx12(HSQUIRRELVM vm)
{
  bool isDX12Supported = d3d::get_driver_code().is(d3d::dx12);
  sq_pushbool(vm, isDX12Supported);
  return 1;
}

static SQInteger is_vulkan(HSQUIRRELVM vm)
{
  bool isVulkan = d3d::get_driver_code().is(d3d::vulkan);
  sq_pushbool(vm, isVulkan);
  return 1;
}

static SQInteger is_gui_driver_select_enabled(HSQUIRRELVM vm)
{
#if _TARGET_PC_WIN
  auto winVer = *get_windows_version(true);
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  bool enabled = winVer.major >= 10 ? blk_video.getBool("guiDriverSelectEnabled", false) : false;
#else
  constexpr bool enabled = false;
#endif
  sq_pushbool(vm, enabled);
  return 1;
}

static SQInteger is_hdr_available(HSQUIRRELVM vm)
{
  const char *displayName = nullptr;
  sq_getstring(vm, SQInteger(2), &displayName);

  bool available = d3d::driver_command(Drv3dCommand::IS_HDR_AVAILABLE, (void *)displayName);

  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  const DataBlock *hdrSupport = blk_video.getBlockByNameEx("hdrSupport");
  bool gameSupportsHDROnDriver = hdrSupport->getBool(d3d::get_driver_name(), true);

  available = available && gameSupportsHDROnDriver;

  sq_pushbool(vm, available);
  return 1;
}

static SQInteger is_hdr_enabled(HSQUIRRELVM vm)
{
  bool enabled = d3d::driver_command(Drv3dCommand::IS_HDR_ENABLED);
  sq_pushbool(vm, enabled);
  return 1;
}

static void change_paper_white_nits(Sqrat::Object field)
{
  uint32_t paper_white_nits = field.Cast<uint32_t>();
  hdrrender::update_paper_white_nits(paper_white_nits);
}

static void change_gamma(Sqrat::Object field)
{
  float gamma = field.Cast<float>();
  set_gamma_shadervar(hdrrender::is_hdr_enabled() ? 1.0f : gamma);
}

static SQInteger is_vrr_supported(HSQUIRRELVM vm)
{
  sq_pushbool(vm, d3d::get_vrr_supported());
  return 1;
}

static SQInteger get_default_static_resolution_scale_sq(HSQUIRRELVM vm)
{
  sq_pushfloat(vm, get_default_static_resolution_scale());
  return 1;
}

static SQInteger is_rendinst_tessellation_supported_sq(HSQUIRRELVM vm)
{
  sq_pushbool(vm, is_rendinst_tessellation_supported());
  return 1;
}

static SQInteger is_only_low_gi_supported_sq(HSQUIRRELVM vm)
{
  sq_pushbool(vm, is_only_low_gi_supported());
  return 1;
}

static SQInteger is_hfr_supported(HSQUIRRELVM vm)
{
  sq_pushbool(vm, d3d::driver_command(Drv3dCommand::GET_CONSOLE_HFR_SUPPORTED));
  return 1;
}

static SQInteger is_rt_supported_sq(HSQUIRRELVM vm)
{
  sq_pushbool(vm, is_rt_supported());
  return 1;
}

static SQInteger rt_support_error_code_sq(HSQUIRRELVM vm)
{
  sq_pushinteger(vm, rt_support_error_code());
  return 1;
}

static SQInteger is_nvidia_gpu(HSQUIRRELVM vm)
{
  bool isNvidia = d3d::get_driver_desc().info.vendor == GpuVendor::NVIDIA;
  sq_pushbool(vm, isNvidia);
  return 1;
}

static SQInteger is_amd_gpu(HSQUIRRELVM vm)
{
  bool isAmd = d3d::get_driver_desc().info.vendor == GpuVendor::AMD;
  sq_pushbool(vm, isAmd);
  return 1;
}

static SQInteger is_intel_gpu(HSQUIRRELVM vm)
{
  bool isIntel = d3d::get_driver_desc().info.vendor == GpuVendor::INTEL;
  sq_pushbool(vm, isIntel);
  return 1;
}

///@module videomode
void bind_script(SqModules *moduleMgr)
{
  Sqrat::Table aTable(moduleMgr->getVM());
  aTable //
    .Func("apply_video_settings", apply_video_settings_sq)
    .SquirrelFunc("get_video_modes", get_video_modes, 2, ".s")
    .SquirrelFunc("get_available_monitors", get_available_monitors, 1)
    .SquirrelFunc("get_monitor_info", get_monitor_info, 2, ".s")
    .SquirrelFunc("get_current_window_resolution", get_current_window_resolution, 1)
    .SquirrelFunc("is_fullscreen_enabled", is_fullscreen_enabled, 1)
    .SquirrelFunc("set_frame_generation_suppressed", set_frame_generation_suppressed, 2, ".b")
    .SquirrelFunc("get_antialiasing_options", get_antialiasing_options, 1)
    .SquirrelFunc("get_antialiasing_upscaling_options", get_antialiasing_upscaling_options, 2, ".s")
    .SquirrelFunc("get_supported_generated_frames", get_supported_generated_frames, 3, ".sb")
    .SquirrelFunc("get_frame_generation_unsupported_reason", get_frame_generation_unsupported_reason, 3, ".sb")
    .SquirrelFunc("get_performance_display_mode_support", get_performance_display_mode_support, 2, ".i")
    .Func("get_auto_selected_antialiasing_method", auto_graphics::auto_antialiasing::get_auto_selected_method)
    .Func("get_settings_override_for_auto_resolution", auto_graphics::auto_resolution::get_antialiasing_settings)
    .Func("should_use_upscaling_method", auto_graphics::auto_antialiasing::should_use_upscaling_method)
    .SquirrelFunc("get_low_latency_modes", get_low_latency_modes, 1)
    .SquirrelFunc("is_nvidia_gpu", is_nvidia_gpu, 1)
    .SquirrelFunc("is_amd_gpu", is_amd_gpu, 1)
    .SquirrelFunc("is_intel_gpu", is_intel_gpu, 1)
    .SquirrelFunc("is_inline_rt_supported", is_inline_rt_supported, 1)
    .SquirrelFunc("is_dx12", is_dx12, 1)
    .SquirrelFunc("is_vulkan", is_vulkan, 1)
    .SquirrelFunc("is_gui_driver_select_enabled", is_gui_driver_select_enabled, 1)
    .Func("is_upsampling", is_upsampling)
    .SquirrelFunc("is_hdr_available", is_hdr_available, 2, ".s")
    .SquirrelFunc("is_hdr_enabled", is_hdr_enabled, 1)
    .Func("change_paper_white_nits", change_paper_white_nits)
    .Func("change_gamma", change_gamma)
    .SquirrelFunc("is_vrr_supported", is_vrr_supported, 1)
    .SquirrelFunc("get_default_static_resolution_scale", get_default_static_resolution_scale_sq, 1)
    .SquirrelFunc("is_rendinst_tessellation_supported", is_rendinst_tessellation_supported_sq, 1)
    .SquirrelFunc("is_only_low_gi_supported", is_only_low_gi_supported_sq, 1)
    .SquirrelFunc("is_hfr_supported", is_hfr_supported, 1)
    .SquirrelFunc("is_rt_supported", is_rt_supported_sq, 1)
    .SquirrelFunc("rt_support_error_code", rt_support_error_code_sq, 1)
    .SquirrelFunc("is_dlss_rr_supported", is_dlss_rr_supported, 1)
    /**/;
  moduleMgr->addNativeModule("videomode", aTable);
}

} // namespace videomode

} // namespace ui
