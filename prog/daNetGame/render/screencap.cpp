// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "screencap.h"
#include <screenShotSystem/dag_screenShotSystem.h>
#include <image/dag_jpeg.h>
#include <image/dag_tga.h>
#include <image/dag_png.h>
#include <image/dag_exr.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <workCycle/dag_workCycle.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/string_view.h>
#include <EASTL/fixed_string.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_simpleString.h>
#include <util/dag_convar.h>
#include <util/dag_aviWriter.h>
#include <gui/dag_imgui.h>
#include <math/dag_mathBase.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_tex3d.h>
#include <debug/dag_debug3d.h>
#include <bindQuirrelEx/autoBind.h>
#include <util/dag_console.h>
#include <3d/dag_render.h>
#include <shaders/dag_postFxRenderer.h>
#include "video360/video360.h"
#include "capture360.h"
#include "captureGbuffer.h"
#include <render/renderer.h>
#include <ecs/camera/getActiveCameraSetup.h>

// Uncomment next line to automatically capture screenshot frames in RenderDoc/PIX
// #define CAPTURE_SCREENSHOT_FRAMES 1

#ifdef CAPTURE_SCREENSHOT_FRAMES
#include <perfMon/dag_pix.h>

static int pending_delay = 0;
#endif

CONSOLE_BOOL_VAL("screencap", uncompressed_screenshots, false);
CONSOLE_BOOL_VAL("screencap", hide_debug, true);

enum class Format
{
  JPEG,
  EXR,
  PNG,
  TGA
};

struct ScreencapSettings
{
  SimpleString screenshotDir;
  bool writeDepthToPng;
  int subSamples;
  int superSamples;
  int jpegQuality;
  bool movie_sequence;
  float aviDt;
  DataBlock aviSettings;
  Format format;

  ScreencapSettings() :
    format(Format::JPEG),
    writeDepthToPng(false),
    superSamples(2),
    subSamples(2),
    aviDt(1. / 30),
    movie_sequence(false),
    jpegQuality(98)
  {}
};

static bool screenshot_scheduled_pending = false;
static bool capturing_gui_pending = false;
static String scheduled_screenshot_name_pending;
static int sequence_num_pending = -1;

static ScreencapSettings settings;
static bool screenshot_scheduled = false;
static String scheduled_screenshot_name;
static int movie_sequence_scheduled = -1, movie_sequence_frame = 0;
static eastl::unique_ptr<AviWriter> avi_writer;
static bool capturing_gui = false;
static String comments;
static int sequence_num = -1;
static eastl::unique_ptr<Video360> video360;

void screencap::set_comments(const char *set_comments) { comments = set_comments; }

static bool save_jpeg32_shot(const char *fn, TexPixel32 *im, int wd, int ht, int stride)
{
  return ::save_jpeg32(im, wd, ht, stride, settings.jpegQuality, fn, comments.empty() ? NULL : comments.str());
}

static bool save_png32_shot(const char *fn, TexPixel32 *im, int wd, int ht, int stride)
{
  return save_png32(fn, im, wd, ht, stride, comments.empty() ? NULL : (uint8_t *)comments.str(), comments.size(),
    settings.writeDepthToPng);
}

static bool save_tga32_shot(const char *fn, TexPixel32 *im, int wd, int ht, int stride)
{
  return save_tga32(fn, im, wd, ht, stride, comments.empty() ? NULL : (uint8_t *)comments.str(), comments.size());
}

static bool save_exr_triplanar_shot(const char *fn, TexPixel32 *im, int wd, int ht, int stride)
{
  const char *planeNames[] = {"B", "G", "R"};
  ht /= eastl::size(planeNames);
  uint8_t *planePixels[] = {(uint8_t *)im + 2 * stride * ht, (uint8_t *)im + stride * ht, (uint8_t *)im};
  return ::save_exr(fn, planePixels, wd, ht, eastl::size(planePixels), stride, planeNames, comments.empty() ? NULL : comments.str());
}

static const char *to_extension(Format format)
{
  switch (format)
  {
    case Format::JPEG: return "jpg";
    case Format::EXR: return "exr";
    case Format::PNG: return "png";
    case Format::TGA: return "tga";
    default: G_ASSERT(false); return nullptr; // unreachable
  }
}

static ScreenShotSystem::write_image_cb to_write_image_callback(Format format)
{
  switch (format)
  {
    case Format::JPEG: return save_jpeg32_shot;
    case Format::EXR: return save_exr_triplanar_shot;
    case Format::PNG: return save_png32_shot;
    case Format::TGA: return save_tga32_shot;
    default: G_ASSERT(false); return nullptr; // unreachable
  }
}

static Format get_format_from_blk(const DataBlock *blk)
{
  const eastl::string_view formatStr = blk->getStr("format", "");
  static eastl::tuple<eastl::string_view, Format> extension_format_table[] = {
    {"png", Format::PNG}, {"tga", Format::TGA}, {"jpeg", Format::JPEG}, {"jpg", Format::JPEG}, {"exr", Format::EXR}};
  auto selected = eastl::find_if(eastl::begin(extension_format_table), eastl::end(extension_format_table),
    [&](const auto &f) { return formatStr == eastl::get<eastl::string_view>(f); });
  if (selected != eastl::end(extension_format_table))
  {
    return eastl::get<Format>(*selected);
  }

  return Format::JPEG;
}

void screencap::init(const DataBlock *blk)
{
  const DataBlock *screenBlk = blk->getBlockByNameEx("screenshots");
  settings.screenshotDir = screenBlk->getStr("dir", "Screenshots");
  settings.format = get_format_from_blk(screenBlk);
  settings.jpegQuality = clamp(screenBlk->getInt("jpegQuality", 98), 20, 100);
  settings.superSamples = screenBlk->getInt("superSamples", 2);
  settings.subSamples = screenBlk->getInt("subSamples", 4);
  settings.aviSettings = *blk->getBlockByNameEx("aviWriter");
  settings.aviDt = safeinv(settings.aviSettings.getReal("frameRate", 30.f));
  settings.movie_sequence = screenBlk->getBool("movie_sequence", false);
  settings.writeDepthToPng = screenBlk->getBool("writeDepthToPng", false);

  video360 = nullptr;
}

void screencap::term() { settings.aviSettings.resetAndReleaseRoNameMap(); }

static UniqueTex convert_linear_to_srgb(const ManagedTex &linear)
{
  TextureInfo ti;
  linear.getTex2D()->getinfo(ti);

  PostFxRenderer tonemap("tonemap");
  UniqueTex sdrTex = dag::create_tex(nullptr, ti.w, ti.h, TEXFMT_DEFAULT | TEXCF_RTARGET, 1, "sdr");
  UniqueTex sdrHostTex = dag::create_tex(nullptr, ti.w, ti.h, TEXFMT_DEFAULT | TEXCF_SYSMEM | TEXCF_UNORDERED, 1, "sdr_host");
  static int linearTexVarId = get_shader_variable_id("linear_tex", false);
  ShaderGlobal::set_texture(linearTexVarId, linear.getTexId());
  d3d::set_render_target(sdrTex.getTex2D(), 0);

  tonemap.render();

  d3d::resource_barrier({sdrTex.getTex2D(), RB_RO_COPY_SOURCE, 0, 0});
  d3d::resource_barrier({sdrTex.getTex2D(), RB_RW_COPY_DEST, 0, 0});

  sdrHostTex->update(sdrTex.getTex2D());

  return sdrHostTex;
}

static eastl::array<UniqueTex, 3> split_planes(const ManagedTex &linear)
{
  TextureInfo ti;
  linear.getTex2D()->getinfo(ti);

  PostFxRenderer splitPlanes("split_planes");

  eastl::array<UniqueTex, 3> planesTex;
  const char *planeNames[3] = {"plane_r", "plane_g", "plane_b"};
  eastl::transform(eastl::begin(planeNames), eastl::end(planeNames), eastl::begin(planesTex),
    [&](const char *name) { return dag::create_tex(nullptr, ti.w, ti.h, TEXFMT_R16F | TEXCF_RTARGET, 1, name); });

  eastl::array<UniqueTex, 3> planesHostTex;
  const char *planeHostNames[3] = {"plane_r_host", "plane_g_host", "plane_b_host"};
  eastl::transform(eastl::begin(planeHostNames), eastl::end(planeHostNames), eastl::begin(planesHostTex),
    [&](const char *name) { return dag::create_tex(nullptr, ti.w, ti.h, TEXFMT_R16F | TEXCF_SYSMEM | TEXCF_UNORDERED, 1, name); });

  static int linearTexVarId = get_shader_variable_id("linear_tex", false);
  ShaderGlobal::set_texture(linearTexVarId, linear.getTexId());

  for (int i : {0, 1, 2})
  {
    d3d::set_render_target(i, planesTex[i].getTex2D(), 0);
    d3d::resource_barrier({planesTex[i].getTex2D(), RB_RW_RENDER_TARGET | RB_STAGE_PIXEL, 0, 0});
  }

  splitPlanes.render();

  for (int i : {0, 1, 2})
  {
    d3d::resource_barrier({planesTex[i].getTex2D(), RB_RO_COPY_SOURCE, 0, 0});
    d3d::resource_barrier({planesHostTex[i].getTex2D(), RB_RW_COPY_DEST, 0, 0});
    planesHostTex[i]->update(planesTex[i].getTex2D());
  }

  return planesHostTex;
}

void screencap::make_screenshot(
  const ManagedTex &frame, const char *file_name_override, bool force_tga, ColorSpace colorspace, const char *additional_postfix)
{
  DagorDateTime time;
  ::get_local_time(&time);

  String fname;

  Format format = settings.format;
  if (force_tga)
    format = Format::TGA;
  else if (uncompressed_screenshots.get())
    format = Format::PNG;

  const char *ext = to_extension(format);
  const char *dir = settings.screenshotDir.str();
  const char *postfix = additional_postfix == nullptr ? "" : additional_postfix;
  if (file_name_override == nullptr && !scheduled_screenshot_name.empty())
    file_name_override = scheduled_screenshot_name.str();

  if (file_name_override == nullptr)
  {
    if (movie_sequence_scheduled > 0)
      fname.printf(256, "%s/frame_%d_%d%s.%s", dir, movie_sequence_scheduled, movie_sequence_frame, postfix, ext);
    else if (sequence_num >= 0)
      fname.printf(256, "%s/shot_%04d%s.%s", dir, sequence_num, postfix, ext);
    else
      fname.printf(256, "%s/shot_%04d.%02d.%02d_%02d.%02d.%02d%s.%s", dir, time.year, time.month, time.day, time.hour, time.minute,
        time.second, postfix, ext);
  }
  else
  {
    fname.printf(256, "%s/%s%s.%s", dir, file_name_override, postfix, ext);
  }

  ScreenShotSystem::setWriteImageCB(to_write_image_callback(format), ext);

  ScreenShotSystem::ScreenShot screen;
  bool success = false;
  if (frame)
  {
    if (format == Format::EXR)
    {
      auto planes = split_planes(frame);
      // EXR stores each color channel separately
      success = ScreenShotSystem::makeTriplanarTexScreenShot(screen, planes[0].getTex2D(), planes[1].getTex2D(), planes[2].getTex2D());
    }
    else if (colorspace == ColorSpace::Linear)
      success = ScreenShotSystem::makeTexScreenShot(screen, convert_linear_to_srgb(frame).getTex2D());
    else
      success = ScreenShotSystem::makeTexScreenShot(screen, frame.getTex2D());
  }
  else
  {
    success = ScreenShotSystem::makeScreenShot(screen);
  }

  if (success)
  {
    if (dd_mkpath(fname.str()) && ScreenShotSystem::saveScreenShotTo(screen, fname))
      debug("screenshot saved to %s", fname.str());
    else
      debug("could not save screenshot to %s", fname.str());
    if (format != Format::EXR)
      ScreenShotSystem::saveScreenShotToClipboard(screen);
  }
}

bool screencap::is_screenshot_scheduled()
{
  if (capture360::is_360_capturing_in_progress() || capture_gbuffer::is_gbuffer_capturing_in_progress())
    return true;
  return screenshot_scheduled;
}

void screencap::schedule_screenshot(bool with_gui, int sequence_number, const char *name_override)
{
  debug("schedule_screenshot: with_gui:%d, sequence_number:%d", with_gui, sequence_number);
  screenshot_scheduled_pending = true;
  scheduled_screenshot_name_pending.setStr(name_override);
  capturing_gui_pending = with_gui || !get_world_renderer();
  sequence_num_pending = sequence_number;

#ifdef CAPTURE_SCREENSHOT_FRAMES
  pending_delay = 3;
  PIX_GPU_CAPTURE_NEXT_FRAMES(D3D11X_PIX_CAPTURE_API, L"", 2);
#endif
}

void screencap::start_pending_request()
{
#ifdef CAPTURE_SCREENSHOT_FRAMES
  if (--pending_delay > 0)
    return;
#endif
  if (!eastl::exchange(screenshot_scheduled_pending, false))
    return;
  screenshot_scheduled = true;
  scheduled_screenshot_name = scheduled_screenshot_name_pending;
  capturing_gui = capturing_gui_pending;
  sequence_num = sequence_num_pending;
}

void screencap::toggle_avi_writer()
{
  capturing_gui = false;
  if (settings.movie_sequence)
  {
    if (movie_sequence_scheduled < 0)
      movie_sequence_scheduled = -movie_sequence_scheduled;
    else
      movie_sequence_scheduled = -(movie_sequence_scheduled + 1);
    movie_sequence_frame = 0;
    return;
  }
  avi_writer.reset(!avi_writer ? new AviWriter(settings.aviSettings, NULL) : nullptr);
}

void screencap::screenshots_saved()
{
  screenshot_scheduled = false;
  clear_and_shrink(scheduled_screenshot_name);
}

void screencap::update(const ManagedTex &frame, ColorSpace colorspace)
{
  if (screenshot_scheduled || movie_sequence_scheduled > 0)
  {
    make_screenshot(frame, nullptr, false, colorspace);
    screenshot_scheduled = false;
  }
  if (avi_writer && movie_sequence_scheduled <= 0)
  {
    bool res = avi_writer->onEndOfFrame();
    if (!res || (avi_writer->fileLimit > 0 && avi_writer->fileLimit < avi_writer->getBytesWritten()))
    {
      toggle_avi_writer();
      if (res)
        toggle_avi_writer();
    }
  }
}

static bool is_capturing_screen() { return screenshot_scheduled || avi_writer || movie_sequence_scheduled > 0; }

bool screencap::should_hide_gui()
{
  return (is_capturing_screen() && !capturing_gui) || (video360) || capture_gbuffer::is_gbuffer_capturing_in_progress();
}

bool screencap::should_hide_debug()
{
  return (is_capturing_screen() && hide_debug.get()) || (video360) || capture_gbuffer::is_gbuffer_capturing_in_progress();
}

int screencap::subsamples() { return is_capturing_screen() ? settings.subSamples : 1; }

int screencap::supersamples() { return is_capturing_screen() ? settings.superSamples : 1; }

float screencap::fixed_act_rate()
{
  if (avi_writer)
    return settings.aviDt;

  return -1.f;
}

eastl::optional<CameraSetupPerspPair> screencap::get_camera()
{
  if (video360)
  {
    return video360->getCamera();
  }
  return eastl::nullopt;
}

void screencap::start_video360(int resolution, int convergence_frames, float fixed_exposure)
{
  console::command("app.timeSpeed 0");
  {
    eastl::string fixedExposureCmd{eastl::string::CtorSprintf{}, "postfx.fixedExposure %f", fixed_exposure};
    console::command(fixedExposureCmd.c_str());
  }
  auto cam = get_active_camera_setup();
  video360 = eastl::make_unique<Video360>(resolution, convergence_frames, cam.znear, cam.zfar, cam.transform);
  d3d::GpuAutoLock gpuLock;
  get_world_renderer()->overrideResolution({resolution, resolution});
}

void screencap::cleanup_video360()
{
  {
    d3d::GpuAutoLock gpuLock;
    get_world_renderer()->resetResolutionOverride();
  }

  video360.reset();
  console::command("app.timeSpeed 1");
  console::command("postfx.fixedExposure");
}

Video360 *screencap::get_video360() { return video360.get(); }

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_screencap, "screencap", sq::VM_ALL)
{
  Sqrat::Table tbl(vm);
  tbl //
    .Func("take_screenshot", [] { screencap::schedule_screenshot(true); })
    .Func("take_screenshot_nogui", [] { screencap::schedule_screenshot(false); })
    .Func("take_screenshot_name", [](const char *name) { screencap::schedule_screenshot(true, -1, name); })
    .Func("take_screenshot_nogui_name", [](const char *name) { screencap::schedule_screenshot(false, -1, name); })
    /**/;

  return tbl;
}

static bool screencap_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("screencap", "take_screenshot", 1, 1) { screencap::schedule_screenshot(true); }
  CONSOLE_CHECK_NAME("screencap", "take_screenshot_nogui", 1, 1) { screencap::schedule_screenshot(false); }
  CONSOLE_CHECK_NAME("screencap", "take_screenshot_ext", 4, 4)
  {
    screencap::schedule_screenshot(console::to_bool(argv[1]), console::to_int(argv[2]), argv[3]);
  }
  CONSOLE_CHECK_NAME_EX("screencap", "take_screenshot_360", 1, 4, "take screenshot 360 and cubemap",
    "[resolution=2048] [convergenceFrames=100] [fixedExposure=1.0]")
  {
    int resolution = argc > 1 ? console::to_int(argv[1]) : 2048;
    int convergenceFrames = argc > 2 ? console::to_int(argv[2]) : 100;
    float fixedExposure = argc > 3 ? console::to_real(argv[3]) : 1.0;
    screencap::start_video360(resolution, convergenceFrames, fixedExposure);
  }
  CONSOLE_CHECK_NAME("screencap", "take_screenshot_gbuffer", 1, 1) { capture_gbuffer::schedule_gbuffer_capture(); }
  return found;
}

REGISTER_CONSOLE_HANDLER(screencap_console_handler);

REGISTER_IMGUI_FUNCTION_EX("Screencap", "Take screenshot", nullptr, 0, [] { screencap::schedule_screenshot(true); });
REGISTER_IMGUI_FUNCTION_EX("Screencap", "Take screenshot (no GUI)", nullptr, 1, [] { screencap::schedule_screenshot(false); });