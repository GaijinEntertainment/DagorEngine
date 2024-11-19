// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_imgui.h>

#include "imguiRenderer.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/implot.h>
#include <imgui/misc/freetype/imgui_freetype.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <perfMon/dag_cpuFreq.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>

static ImGuiState imgui_state = ImGuiState::OFF;
static ImGuiState requested_state = ImGuiState::OFF;
static bool is_state_change_requested = false;
static eastl::unique_ptr<eastl::vector<OnStateChangeHandlerFunc>> on_state_change_functions;

static bool is_initialized = false;
static bool is_bold_font_set = false;
static bool is_mono_font_set = false;
static eastl::unique_ptr<DagImGuiRenderer> renderer;
static eastl::unique_ptr<DataBlock> imgui_blk;
static eastl::unique_ptr<DataBlock> override_imgui_blk;
static const char *imgui_blk_path = "imgui.blk";
static String full_blk_path;
static String full_ini_path;
static String full_log_path;

static eastl::optional<String> custom_blk_path;
static String custom_ini_path;
static String custom_log_path;


static float active_window_bg_alpha = 1.0f;
static float overlay_window_bg_alpha = 0.5f;
static ImFont *imgui_bold_font = nullptr;
static ImFont *imgui_mono_font = nullptr;
static eastl::unique_ptr<ImFontConfig> requested_font_cfg;
static eastl::unique_ptr<ImFontConfig> requested_bold_font_cfg;
static eastl::unique_ptr<ImFontConfig> requested_mono_font_cfg;
static constexpr float MIN_SCALE = 1.0f;
static constexpr float MAX_SCALE = 4.0f;
static bool frameEnded = true;

static bool imguiSubmenuEnabled = true;

void imgui_set_override_blk(const DataBlock &imgui_blk_)
{
  G_ASSERTF(!is_initialized, "imgui_set_override_blk() should be called before imgui_init_on_demand()");
  override_imgui_blk = eastl::make_unique<DataBlock>();
  override_imgui_blk->setFrom(&imgui_blk_);
}

void imgui_enable_imgui_submenu(bool enabled) { imguiSubmenuEnabled = enabled; }

static float get_default_scale()
{
  int w, h;
  d3d::get_screen_size(w, h);
  float scale = roundf(h / 1080.0f); // - Rounded scale on purpose, because non-integer scale produces blurry text with
                                     //   default ImGui font.
                                     // - We should determine default scaling based on DPI instead of raw resolution
                                     //   (e.g. by querying the built in scaling in Windows).
  return clamp(scale, MIN_SCALE, MAX_SCALE);
}

void imgui_apply_style_from_blk()
{
  float imguiScale = imgui_blk->getReal("imgui_scale", get_default_scale());
  imguiScale = clamp(imguiScale, MIN_SCALE, MAX_SCALE);

  ImGuiStyle scaledStyle;
  scaledStyle.ScaleAllSizes(imguiScale);
  ImGui::GetStyle() = scaledStyle;
  ImGui::StyleColorsDark(); // TODO: Apply custom style here if we ever wish to support it. Right now we use Dark style,
                            //       which is also the default.
  active_window_bg_alpha = imgui_blk->getReal("active_window_bg_alpha", ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w);
  overlay_window_bg_alpha = active_window_bg_alpha * 0.5f;

  unsigned int fontBuilderFlags = 0;
  if (imgui_blk->getBool("imgui_light_font_hinting", false))
    fontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;

  requested_font_cfg = eastl::make_unique<ImFontConfig>();
  requested_font_cfg->OversampleH = requested_font_cfg->OversampleV = 1;
  requested_font_cfg->PixelSnapH = true; // some fonts are blurry without this
  requested_font_cfg->SizePixels = floor(imgui_blk->getReal("imgui_font_size", 13.0f) * imguiScale);
  requested_font_cfg->GlyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesCyrillic();
  requested_font_cfg->FontBuilderFlags = fontBuilderFlags;

  requested_bold_font_cfg = eastl::make_unique<ImFontConfig>();
  requested_bold_font_cfg->OversampleH = requested_bold_font_cfg->OversampleV = 1;
  requested_bold_font_cfg->PixelSnapH = true; // some fonts are blurry without this
  requested_bold_font_cfg->SizePixels = floor(imgui_blk->getReal("imgui_bold_font_size", 13.0f) * imguiScale);
  requested_bold_font_cfg->GlyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesCyrillic();
  requested_bold_font_cfg->FontBuilderFlags = fontBuilderFlags;

  requested_mono_font_cfg = eastl::make_unique<ImFontConfig>();
  requested_mono_font_cfg->OversampleH = requested_mono_font_cfg->OversampleV = 1;
  requested_mono_font_cfg->PixelSnapH = true; // some fonts are blurry without this
  requested_mono_font_cfg->SizePixels = floor(imgui_blk->getReal("imgui_mono_font_size", 15.0f) * imguiScale);
  requested_mono_font_cfg->GlyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesCyrillic();
  requested_mono_font_cfg->FontBuilderFlags = fontBuilderFlags;
}

void imgui_set_blk_path(const char *path) { custom_blk_path = String(path); }
void imgui_set_ini_path(const char *path) { custom_ini_path = String(path); }
void imgui_set_log_path(const char *path) { custom_log_path = String(path); }

static bool init()
{
  if (is_initialized)
    return true;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();

  ImGuiIO &io = ImGui::GetIO();

  // Load blk
  imgui_blk = eastl::make_unique<DataBlock>();

  if (custom_blk_path.has_value())
    full_blk_path = custom_blk_path.value().empty() ? String() : String(0, "%s/%s", custom_blk_path.value(), imgui_blk_path);
  else
    full_blk_path = String(0, "%s", imgui_blk_path);

  if (!custom_ini_path.empty())
    full_ini_path = String(0, "%s/%s", custom_ini_path, io.IniFilename);
  else
    full_ini_path = String(0, "%s", io.IniFilename);

  if (!custom_log_path.empty())
    full_log_path = String(0, "%s/%s", custom_log_path, io.LogFilename);
  else
    full_log_path = String(0, "%s", io.LogFilename);

  if (!full_blk_path.empty() && dd_file_exist(full_blk_path.c_str()))
    imgui_blk->load(full_blk_path.c_str());

  if (override_imgui_blk)
    merge_data_block(*imgui_blk, *override_imgui_blk);

  imgui_apply_style_from_blk();

  io.IniFilename = full_ini_path.c_str();
  io.LogFilename = full_log_path.c_str();

  io.ConfigWindowsMoveFromTitleBarOnly = true;

  // Init our own renderer backend
  renderer = eastl::make_unique<DagImGuiRenderer>();
  renderer->setBackendFlags(io);

  is_initialized = true;
  return true;
}

bool imgui_init_on_demand()
{
  if (is_initialized)
    return true;
  bool success = init();
  if (!success)
  {
    logerr("Failed to initialize imgui.");
    return false;
  }
  return true;
}

static void handle_state_change_request()
{
  if (!is_state_change_requested)
    return;

  if (requested_state == imgui_state)
  {
    is_state_change_requested = false;
    return;
  }

  if (!imgui_init_on_demand())
    return;

  ImGuiState oldState = imgui_state;
  imgui_state = requested_state;
  is_state_change_requested = false;

  ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w =
    imgui_state == ImGuiState::OVERLAY ? overlay_window_bg_alpha : active_window_bg_alpha;

  if (on_state_change_functions != nullptr)
    for (OnStateChangeHandlerFunc &func : *on_state_change_functions)
      func(oldState, imgui_state);
}

void imgui_shutdown()
{
  if (is_initialized)
  {
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
  }

  imgui_save_blk();

  is_initialized = false;
  renderer.reset();
  imgui_blk.reset();
  override_imgui_blk.reset();
  requested_font_cfg.reset();
  on_state_change_functions.reset();
}

ImGuiState imgui_get_state() { return imgui_state; }

bool imgui_want_capture_mouse() { return ImGui::GetIO().WantCaptureMouse; }

void imgui_request_state_change(ImGuiState new_state)
{
  requested_state = new_state;
  is_state_change_requested = true;
}

void imgui_register_on_state_change_handler(OnStateChangeHandlerFunc func)
{
  if (on_state_change_functions == nullptr)
    on_state_change_functions = eastl::make_unique<eastl::vector<OnStateChangeHandlerFunc>>();
  on_state_change_functions->push_back(func);
}

ImFont *imgui_get_bold_font() { return imgui_bold_font; }

ImFont *imgui_get_mono_font() { return imgui_mono_font; }

void imgui_set_bold_font()
{
  if (is_bold_font_set || !imgui_bold_font)
    return;

  imgui_set_default_font();

  ImGui::PushFont(imgui_bold_font);
  is_bold_font_set = true;
}

void imgui_set_mono_font()
{
  if (is_mono_font_set || !imgui_mono_font)
    return;

  imgui_set_default_font();

  ImGui::PushFont(imgui_mono_font);
  is_mono_font_set = true;
}

void imgui_set_default_font()
{
  if (is_bold_font_set)
  {
    ImGui::PopFont();
    is_bold_font_set = false;
  }

  if (is_mono_font_set)
  {
    ImGui::PopFont();
    is_mono_font_set = false;
  }
}

static ImFont *add_imgui_font(const char *file_name, const ImFontConfig *cfg)
{
  if (!file_name || !file_name[0])
    return nullptr;

  ImGuiIO &io = ImGui::GetIO();

  int imguiFontFileSize = 0;
  FullFileLoadCB cb(file_name, DF_READ);
  if (!cb.fileHandle)
    logwarn("ImGui: Cannot read font file '%s', dafault font will be used", file_name);
  else if (const void *fileData = df_mmap(cb.fileHandle, &imguiFontFileSize))
  {
    G_ASSERT(imguiFontFileSize < (10 << 20)); // 10 MB is too much for ImGui font
    if (imguiFontFileSize > (11 << 20))
    {
      logwarn("ImGui: Font file '%s', is too large", file_name);
      df_unmap(fileData, imguiFontFileSize);
      return nullptr;
    }
    void *fontData = IM_ALLOC(imguiFontFileSize); // fontData will be freed inside ImGui
    memcpy(fontData, fileData, imguiFontFileSize);
    df_unmap(fileData, imguiFontFileSize);
    return io.Fonts->AddFontFromMemoryTTF(fontData, imguiFontFileSize, cfg->SizePixels, cfg);
  }

  return nullptr;
}

void imgui_update(int display_width, int display_height)
{
  handle_state_change_request();

  if (imgui_state == ImGuiState::OFF)
    return;

  ImGuiIO &io = ImGui::GetIO();

  if (requested_font_cfg)
  {
    io.Fonts->Clear();
    io.FontDefault = add_imgui_font(imgui_blk->getStr("imgui_font_name", ""), requested_font_cfg.get());
    if (!io.FontDefault)
      io.FontDefault = io.Fonts->AddFontDefault(requested_font_cfg.get());

    imgui_bold_font = add_imgui_font(imgui_blk->getStr("imgui_bold_font_name", ""), requested_bold_font_cfg.get());
    if (!imgui_bold_font)
      imgui_bold_font = io.FontDefault;

    imgui_mono_font = add_imgui_font(imgui_blk->getStr("imgui_mono_font_name", ""), requested_mono_font_cfg.get());
    if (!imgui_mono_font)
      imgui_mono_font = io.FontDefault;

    renderer->createAndSetFontTexture(io);
    requested_font_cfg.reset();
    requested_bold_font_cfg.reset();
    requested_mono_font_cfg.reset();
  }

  if (display_width <= 0 || display_height <= 0)
    d3d::get_screen_size(display_width, display_height);
  io.DisplaySize = ImVec2(display_width, display_height);

  static int64_t reft = ref_time_ticks();
  int64_t curt = ref_time_ticks();
  io.DeltaTime = ref_time_delta_to_usec(curt - reft) * 1e-6f;
  reft = curt;

  // work-around to handle issue when fullscreen game loses focus and throw endless assertion:
  // "(g.FrameCount == 0 || g.FrameCountEnded == g.FrameCount) && "Forgot to call Render() or EndFrame() at the end of the previous
  // frame?""
  if (!frameEnded)
    imgui_endframe();
  ImGui::NewFrame();
  frameEnded = false;
}

void imgui_endframe()
{
  imgui_set_default_font();
  ImGui::EndFrame();
  frameEnded = true;
}

void imgui_render()
{
  ImGui::Render();
  renderer->render(ImGui::GetDrawData());
  frameEnded = true;
}

DataBlock *imgui_get_blk() { return imgui_blk.get(); }

void imgui_save_blk()
{
  if (imgui_blk != nullptr && !full_blk_path.empty())
    imgui_blk->saveToTextFile(full_blk_path);
}

static bool load_window_opened(const char *window_name)
{
  return imgui_blk && imgui_blk->getBlockByNameEx("windows")->getBool(window_name, false);
}

static void save_window_opened(const char *window_name, bool opened)
{
  if (imgui_blk)
    imgui_blk->addBlock("windows")->setBool(window_name, opened);
  imgui_save_blk();
}

void imgui_window_set_visible(const char *, const char *name, const bool visible)
{
  if (imgui_init_on_demand()) // instantiate imgui_blk
    save_window_opened(name, visible);
}

void imgui_cascade_windows()
{
  if (!imgui_init_on_demand())
    return;

  if (ImGuiContext *ctx = ImGui::GetCurrentContext())
  {
    dag::Vector<ImGuiWindow *> windows;
    for (ImGuiWindow *window : ctx->Windows)
    {
      if (window->IsFallbackWindow || window->Hidden || window->RootWindow != window)
        continue;
      if (window->Flags & ImGuiWindowFlags_NoMove &&
          (window->Collapsed || window->Flags & ImGuiWindowFlags_NoResize || window->Flags & ImGuiWindowFlags_AlwaysAutoResize))
        continue;
      windows.push_back(window);
    }
    if (!windows.empty())
    {
      ImGuiWindow *prevWindow = ctx->CurrentWindow;
      const Point2 topLeft = ImGui::GetMainViewport()->WorkPos;
      const Point2 viewSize = ImGui::GetMainViewport()->WorkSize;
      const Point2 maxStep = viewSize * 0.6f / float(windows.size() + 1);
      const Point2 step = Point2(eastl::min(maxStep.x, 50.f), eastl::min(maxStep.y, 50.f));
      int i = 1;
      for (ImGuiWindow *window : windows)
      {
        ctx->CurrentWindow = window;
        Point2 pos = Point2(ImGui::GetWindowSize());
        if (!(window->Flags & ImGuiWindowFlags_NoMove))
        {
          pos = topLeft + step * i;
          ImGui::SetWindowPos(pos);
        }
        if (!(window->Collapsed || window->Flags & ImGuiWindowFlags_NoResize || window->Flags & ImGuiWindowFlags_AlwaysAutoResize))
        {
          const Point2 size = ImGui::GetWindowSize();
          const Point2 newSize =
            Point2(eastl::clamp(size.x, 50.f, viewSize.x - pos.x), eastl::clamp(size.y, 50.f, viewSize.y - pos.y));
          ImGui::SetWindowSize(newSize);
        }
        i += 1;
      }
      ctx->CurrentWindow = prevWindow;
    }
  }
}

eastl::optional<ImGuiKey> map_dagor_key_to_imgui(int humap_key)
{
  switch (humap_key)
  {
    case HumanInput::DKEY_TAB: return ImGuiKey_Tab;
    case HumanInput::DKEY_LEFT: return ImGuiKey_LeftArrow;
    case HumanInput::DKEY_RIGHT: return ImGuiKey_RightArrow;
    case HumanInput::DKEY_UP: return ImGuiKey_UpArrow;
    case HumanInput::DKEY_DOWN: return ImGuiKey_DownArrow;
    case HumanInput::DKEY_PRIOR: return ImGuiKey_PageUp;
    case HumanInput::DKEY_NEXT: return ImGuiKey_PageDown;
    case HumanInput::DKEY_HOME: return ImGuiKey_Home;
    case HumanInput::DKEY_END: return ImGuiKey_End;
    case HumanInput::DKEY_INSERT: return ImGuiKey_Insert;
    case HumanInput::DKEY_DELETE: return ImGuiKey_Delete;
    case HumanInput::DKEY_BACK: return ImGuiKey_Backspace;
    case HumanInput::DKEY_SPACE: return ImGuiKey_Space;
    case HumanInput::DKEY_RETURN: return ImGuiKey_Enter;
    case HumanInput::DKEY_ESCAPE: return ImGuiKey_Escape;
    case HumanInput::DKEY_NUMPADENTER: return ImGuiKey_KeypadEnter;
    case HumanInput::DKEY_LSHIFT: return ImGuiKey_LeftShift;
    case HumanInput::DKEY_LCONTROL: return ImGuiKey_LeftCtrl;
    case HumanInput::DKEY_LALT: return ImGuiKey_LeftAlt;
    case HumanInput::DKEY_RSHIFT: return ImGuiKey_RightShift;
    case HumanInput::DKEY_RCONTROL: return ImGuiKey_RightCtrl;
    case HumanInput::DKEY_RALT: return ImGuiKey_RightAlt;
    case HumanInput::DKEY_A: return ImGuiKey_A;
    case HumanInput::DKEY_B: return ImGuiKey_B;
    case HumanInput::DKEY_C: return ImGuiKey_C;
    case HumanInput::DKEY_D: return ImGuiKey_D;
    case HumanInput::DKEY_E: return ImGuiKey_E;
    case HumanInput::DKEY_F: return ImGuiKey_F;
    case HumanInput::DKEY_G: return ImGuiKey_G;
    case HumanInput::DKEY_H: return ImGuiKey_H;
    case HumanInput::DKEY_I: return ImGuiKey_I;
    case HumanInput::DKEY_J: return ImGuiKey_J;
    case HumanInput::DKEY_K: return ImGuiKey_K;
    case HumanInput::DKEY_L: return ImGuiKey_L;
    case HumanInput::DKEY_M: return ImGuiKey_M;
    case HumanInput::DKEY_N: return ImGuiKey_N;
    case HumanInput::DKEY_O: return ImGuiKey_O;
    case HumanInput::DKEY_P: return ImGuiKey_P;
    case HumanInput::DKEY_Q: return ImGuiKey_Q;
    case HumanInput::DKEY_R: return ImGuiKey_R;
    case HumanInput::DKEY_S: return ImGuiKey_S;
    case HumanInput::DKEY_T: return ImGuiKey_T;
    case HumanInput::DKEY_U: return ImGuiKey_U;
    case HumanInput::DKEY_V: return ImGuiKey_V;
    case HumanInput::DKEY_W: return ImGuiKey_W;
    case HumanInput::DKEY_X: return ImGuiKey_X;
    case HumanInput::DKEY_Y: return ImGuiKey_Y;
    case HumanInput::DKEY_Z: return ImGuiKey_Z;
    case HumanInput::DKEY_F1: return ImGuiKey_F1;
    case HumanInput::DKEY_F2: return ImGuiKey_F2;
    case HumanInput::DKEY_F3: return ImGuiKey_F3;
    case HumanInput::DKEY_F4: return ImGuiKey_F4;
    case HumanInput::DKEY_F5: return ImGuiKey_F5;
    case HumanInput::DKEY_F6: return ImGuiKey_F6;
    case HumanInput::DKEY_F7: return ImGuiKey_F7;
    case HumanInput::DKEY_F8: return ImGuiKey_F8;
    case HumanInput::DKEY_F9: return ImGuiKey_F9;
    case HumanInput::DKEY_F10: return ImGuiKey_F10;
    case HumanInput::DKEY_F11: return ImGuiKey_F11;
    case HumanInput::DKEY_F12: return ImGuiKey_F12;
    case HumanInput::DKEY_NUMPAD0: return ImGuiKey_Keypad0;
    case HumanInput::DKEY_NUMPAD1: return ImGuiKey_Keypad1;
    case HumanInput::DKEY_NUMPAD2: return ImGuiKey_Keypad2;
    case HumanInput::DKEY_NUMPAD3: return ImGuiKey_Keypad3;
    case HumanInput::DKEY_NUMPAD4: return ImGuiKey_Keypad4;
    case HumanInput::DKEY_NUMPAD5: return ImGuiKey_Keypad5;
    case HumanInput::DKEY_NUMPAD6: return ImGuiKey_Keypad6;
    case HumanInput::DKEY_NUMPAD7: return ImGuiKey_Keypad7;
    case HumanInput::DKEY_NUMPAD8: return ImGuiKey_Keypad8;
    case HumanInput::DKEY_NUMPAD9: return ImGuiKey_Keypad9;

    default: return eastl::optional<ImGuiKey>();
  }
}

int map_imgui_key_to_dagor(int imgui_key)
{
  switch (imgui_key)
  {
    case ImGuiKey_Tab: return HumanInput::DKEY_TAB;
    case ImGuiKey_LeftArrow: return HumanInput::DKEY_LEFT;
    case ImGuiKey_RightArrow: return HumanInput::DKEY_RIGHT;
    case ImGuiKey_UpArrow: return HumanInput::DKEY_UP;
    case ImGuiKey_DownArrow: return HumanInput::DKEY_DOWN;
    case ImGuiKey_PageUp: return HumanInput::DKEY_PRIOR;
    case ImGuiKey_PageDown: return HumanInput::DKEY_NEXT;
    case ImGuiKey_Home: return HumanInput::DKEY_HOME;
    case ImGuiKey_End: return HumanInput::DKEY_END;
    case ImGuiKey_Insert: return HumanInput::DKEY_INSERT;
    case ImGuiKey_Delete: return HumanInput::DKEY_DELETE;
    case ImGuiKey_Backspace: return HumanInput::DKEY_BACK;
    case ImGuiKey_Space: return HumanInput::DKEY_SPACE;
    case ImGuiKey_Enter: return HumanInput::DKEY_RETURN;
    case ImGuiKey_Escape: return HumanInput::DKEY_ESCAPE;
    case ImGuiKey_KeypadEnter: return HumanInput::DKEY_NUMPADENTER;
    case ImGuiKey_LeftShift: return HumanInput::DKEY_LSHIFT;
    case ImGuiKey_LeftCtrl: return HumanInput::DKEY_LCONTROL;
    case ImGuiKey_LeftAlt: return HumanInput::DKEY_LALT;
    case ImGuiKey_RightShift: return HumanInput::DKEY_RSHIFT;
    case ImGuiKey_RightCtrl: return HumanInput::DKEY_RCONTROL;
    case ImGuiKey_RightAlt: return HumanInput::DKEY_RALT;
    case ImGuiKey_A: return HumanInput::DKEY_A;
    case ImGuiKey_B: return HumanInput::DKEY_B;
    case ImGuiKey_C: return HumanInput::DKEY_C;
    case ImGuiKey_D: return HumanInput::DKEY_D;
    case ImGuiKey_E: return HumanInput::DKEY_E;
    case ImGuiKey_F: return HumanInput::DKEY_F;
    case ImGuiKey_G: return HumanInput::DKEY_G;
    case ImGuiKey_H: return HumanInput::DKEY_H;
    case ImGuiKey_I: return HumanInput::DKEY_I;
    case ImGuiKey_J: return HumanInput::DKEY_J;
    case ImGuiKey_K: return HumanInput::DKEY_K;
    case ImGuiKey_L: return HumanInput::DKEY_L;
    case ImGuiKey_M: return HumanInput::DKEY_M;
    case ImGuiKey_N: return HumanInput::DKEY_N;
    case ImGuiKey_O: return HumanInput::DKEY_O;
    case ImGuiKey_P: return HumanInput::DKEY_P;
    case ImGuiKey_Q: return HumanInput::DKEY_Q;
    case ImGuiKey_R: return HumanInput::DKEY_R;
    case ImGuiKey_S: return HumanInput::DKEY_S;
    case ImGuiKey_T: return HumanInput::DKEY_T;
    case ImGuiKey_U: return HumanInput::DKEY_U;
    case ImGuiKey_V: return HumanInput::DKEY_V;
    case ImGuiKey_W: return HumanInput::DKEY_W;
    case ImGuiKey_X: return HumanInput::DKEY_X;
    case ImGuiKey_Y: return HumanInput::DKEY_Y;
    case ImGuiKey_Z: return HumanInput::DKEY_Z;
    case ImGuiKey_0: return HumanInput::DKEY_0;
    case ImGuiKey_1: return HumanInput::DKEY_1;
    case ImGuiKey_2: return HumanInput::DKEY_2;
    case ImGuiKey_3: return HumanInput::DKEY_3;
    case ImGuiKey_4: return HumanInput::DKEY_4;
    case ImGuiKey_5: return HumanInput::DKEY_5;
    case ImGuiKey_6: return HumanInput::DKEY_6;
    case ImGuiKey_7: return HumanInput::DKEY_7;
    case ImGuiKey_8: return HumanInput::DKEY_8;
    case ImGuiKey_9: return HumanInput::DKEY_9;
    case ImGuiKey_F1: return HumanInput::DKEY_F1;
    case ImGuiKey_F2: return HumanInput::DKEY_F2;
    case ImGuiKey_F3: return HumanInput::DKEY_F3;
    case ImGuiKey_F4: return HumanInput::DKEY_F4;
    case ImGuiKey_F5: return HumanInput::DKEY_F5;
    case ImGuiKey_F6: return HumanInput::DKEY_F6;
    case ImGuiKey_F7: return HumanInput::DKEY_F7;
    case ImGuiKey_F8: return HumanInput::DKEY_F8;
    case ImGuiKey_F9: return HumanInput::DKEY_F9;
    case ImGuiKey_F10: return HumanInput::DKEY_F10;
    case ImGuiKey_F11: return HumanInput::DKEY_F11;
    case ImGuiKey_F12: return HumanInput::DKEY_F12;
    case ImGuiKey_Keypad0: return HumanInput::DKEY_NUMPAD0;
    case ImGuiKey_Keypad1: return HumanInput::DKEY_NUMPAD1;
    case ImGuiKey_Keypad2: return HumanInput::DKEY_NUMPAD2;
    case ImGuiKey_Keypad3: return HumanInput::DKEY_NUMPAD3;
    case ImGuiKey_Keypad4: return HumanInput::DKEY_NUMPAD4;
    case ImGuiKey_Keypad5: return HumanInput::DKEY_NUMPAD5;
    case ImGuiKey_Keypad6: return HumanInput::DKEY_NUMPAD6;
    case ImGuiKey_Keypad7: return HumanInput::DKEY_NUMPAD7;
    case ImGuiKey_Keypad8: return HumanInput::DKEY_NUMPAD8;
    case ImGuiKey_Keypad9: return HumanInput::DKEY_NUMPAD9;

    default: return -1;
  }
}

bool imgui_window_is_visible(const char *, const char *name) { return load_window_opened(name); }

static int menu_bar_height = 0;

int imgui_get_menu_bar_height() { return menu_bar_height; }

void imgui_perform_registered(bool with_menu_bar)
{
  // Construct main menu bar
  if (with_menu_bar && ImGui::BeginMainMenuBar())
  {
    // Function queue
    if (ImGuiFunctionQueue::functionHead != nullptr)
    {
      const char *currentGroup = ImGuiFunctionQueue::functionHead->group;
      bool currentGroupOpened = ImGui::BeginMenu(currentGroup);
      for (ImGuiFunctionQueue *q = ImGuiFunctionQueue::functionHead; q; q = q->next)
      {
        if (stricmp(currentGroup, q->group) != 0)
        {
          if (currentGroupOpened)
            ImGui::EndMenu();
          currentGroup = q->group;
          currentGroupOpened = ImGui::BeginMenu(currentGroup);
        }
        if (currentGroupOpened)
        {
          if (ImGui::MenuItem(q->name, q->hotkey))
          {
            G_ASSERTF_CONTINUE(q->function, "Registered ImGui function is null: %s/%s", q->group, q->name);
            q->function();
          }
        }
      }
      if (currentGroupOpened)
        ImGui::EndMenu();
    }

    if (imguiSubmenuEnabled && ImGui::BeginMenu("ImGui"))
    {
      ImGui::Separator();

      const float defaultScale = get_default_scale();
      float imguiScale = imgui_blk->getReal("imgui_scale", defaultScale);
      if (ImGui::Button(String(0, "Default (%.1fx)", defaultScale).c_str()))
      {
        imguiScale = defaultScale;
        imgui_blk->setReal("imgui_scale", imguiScale);
        imgui_save_blk();
        imgui_apply_style_from_blk();
      }
      ImGui::SameLine();
      if (ImGui::DragFloat("ImGui scale", &imguiScale, 0.005f, MIN_SCALE, MAX_SCALE, "%.1f"))
      {
        imguiScale = clamp(imguiScale, MIN_SCALE, MAX_SCALE);
        imgui_blk->setReal("imgui_scale", imguiScale);
        imgui_save_blk();
        imgui_apply_style_from_blk();
      }
      ImGui::EndMenu();
    }

    // Window queue
    if (ImGuiFunctionQueue::windowHead != nullptr)
    {
      if (ImGui::BeginMenu("Window"))
      {
        const char *currentGroup = ImGuiFunctionQueue::windowHead->group;
        bool currentGroupOpened = ImGui::BeginMenu(currentGroup);
        for (ImGuiFunctionQueue *q = ImGuiFunctionQueue::windowHead; q; q = q->next)
        {
          if (stricmp(currentGroup, q->group) != 0)
          {
            if (currentGroupOpened)
              ImGui::EndMenu();
            currentGroup = q->group;
            currentGroupOpened = ImGui::BeginMenu(currentGroup);
          }
          if (currentGroupOpened)
          {
            q->opened = load_window_opened(q->name);
            bool oldOpened = q->opened;
            ImGui::MenuItem(q->name, q->hotkey, &q->opened);
            if (q->opened != oldOpened)
              save_window_opened(q->name, q->opened);
          }
        }
        if (currentGroupOpened)
          ImGui::EndMenu();
        ImGui::EndMenu();
      }
    }

    menu_bar_height = ImGui::GetWindowSize().y;

    ImGui::EndMainMenuBar();
  }

  // Execute window functions
  for (ImGuiFunctionQueue *q = ImGuiFunctionQueue::windowHead; q; q = q->next)
  {
    q->opened = load_window_opened(q->name);
    if (q->opened)
    {
      G_ASSERTF_CONTINUE(q->function, "Registered ImGui window function is null: %s/%s", q->group, q->name);
      bool oldOpened = true; // q->opened == true here
      ImGui::Begin(q->name, &q->opened, q->flags);
      if (q->opened != oldOpened)
        save_window_opened(q->name, q->opened);
      q->function();
      ImGui::End();
    }
  }
}

ImGuiFunctionQueue *ImGuiFunctionQueue::windowHead = nullptr;
ImGuiFunctionQueue *ImGuiFunctionQueue::functionHead = nullptr;

ImGuiFunctionQueue::ImGuiFunctionQueue(const char *group_, const char *name_, const char *hotkey_, int priority_, int flags_,
  ImGuiFuncPtr func, bool is_window) :
  group(group_), name(name_), hotkey(hotkey_), priority(priority_), flags(flags_), function(func)
{
  ImGuiFunctionQueue **head = is_window ? &windowHead : &functionHead;
  if (*head == nullptr)
  {
    *head = this;
    return;
  }

  ImGuiFunctionQueue *n = *head, *p = nullptr;
  for (; n; p = n, n = n->next)
  {
    int cmp = stricmp(group, n->group);
    if (cmp < 0 || (cmp == 0 && priority < n->priority))
    {
      // insert before
      next = n;
      if (p)
        p->next = this;
      else
        *head = this;
      return;
    }
  }
  p->next = this;
}

static void after_device_reset(bool full_reset)
{
  if (full_reset && renderer)
    renderer->createAndSetFontTexture(ImGui::GetIO());
}

REGISTER_D3D_AFTER_RESET_FUNC(after_device_reset);