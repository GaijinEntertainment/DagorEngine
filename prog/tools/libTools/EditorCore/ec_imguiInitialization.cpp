// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS
#include <EditorCore/ec_imGuiInitialization.h>
#include <EditorCore/ec_interface.h>
#include <gui/dag_imgui.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <propPanel/constants.h>
#include <sepGui/wndGlobal.h>
#include <util/dag_string.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

// Workaround for stuck keys after displaying a native modal dialog or message box in reaction to a key press.
// This is needed because in case of modal dialogs and message boxes the focus loss and focus gain happen in
// the same ImGui frame, so as far as ImGui is concerned nothing happened, and it would report the key pressed
// forever.
// https://github.com/ocornut/imgui/issues/7264
class ImguiNativeModalDialogEventHandler : public wingw::INativeModalDialogEventHandler
{
public:
  virtual void beforeModalDialogShown() override {}

  virtual void afterModalDialogShown() override
  {
    if (!ImGui::GetCurrentContext())
      return;

    ImGui::GetIO().ClearInputKeys();
    ImGui::GetIO().ClearEventsQueue();
  }
};

static ImguiNativeModalDialogEventHandler imgui_native_modal_dialog_event_handler;
static String imgui_ini_file_path;

// daEditorClassic style
static void apply_imgui_style()
{
  ImGuiStyle &style = ImGui::GetStyle();

  style.Alpha = 1.0f;
  style.DisabledAlpha = 0.6000000238418579f;
  style.WindowPadding = ImVec2(8.0f, 8.0f);
  style.WindowRounding = 0.0f;
  style.WindowBorderSize = 1.0f;
  style.WindowMinSize = ImVec2(20.0f, 20.0f);
  style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
  style.WindowMenuButtonPosition = ImGuiDir_None;
  style.ChildRounding = 0.0f;
  style.ChildBorderSize = 1.0f;
  style.PopupRounding = 0.0f;
  style.PopupBorderSize = 1.0f;
  style.FramePadding = ImVec2(4.0f, 3.0f);
  style.FrameRounding = 0.0f;
  style.FrameBorderSize = 1.0f;
  style.ItemSpacing = ImVec2(4.0f, 3.0f);
  style.ItemInnerSpacing = ImVec2(4.0f, 3.0f);
  style.CellPadding = ImVec2(4.0f, 2.0f);
  style.IndentSpacing = 21.0f;
  style.ColumnsMinSpacing = 6.0f;
  style.ScrollbarSize = 18.0f;
  style.ScrollbarRounding = 0.0f;
  style.GrabMinSize = 14.0f;
  style.GrabRounding = 0.0f;
  style.TabRounding = 2.0f;
  style.TabBorderSize = 1.0f;
  style.TabMinWidthForCloseButton = 20.0f;
  style.ColorButtonPosition = ImGuiDir_Left;
  style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
  style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_Text] = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.427f, 0.427f, 0.427f, 1.000f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.773f, 0.773f, 0.773f, 1.000f);
  colors[ImGuiCol_ChildBg] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.941f, 0.941f, 0.941f, 1.000f);
  colors[ImGuiCol_Border] = ImVec4(0.000f, 0.000f, 0.000f, 0.125f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
  colors[ImGuiCol_FrameBg] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.000f, 0.314f, 0.588f, 1.000f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.000f, 0.314f, 0.588f, 1.000f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.502f, 0.502f, 0.502f, 1.000f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.859f, 0.859f, 0.859f, 1.000f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.941f, 0.941f, 0.941f, 1.000f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.804f, 0.804f, 0.804f, 1.000f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.651f, 0.651f, 0.651f, 1.000f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.376f, 0.376f, 0.376f, 1.000f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.651f, 0.651f, 0.651f, 1.000f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.376f, 0.376f, 0.376f, 1.000f);
  colors[ImGuiCol_Button] = ImVec4(0.941f, 0.941f, 0.941f, 1.000f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.627f, 0.627f, 0.627f, 1.000f);
  colors[ImGuiCol_Header] = ImVec4(0.502f, 0.502f, 0.502f, 1.000f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.000f, 0.541f, 1.000f, 0.251f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.502f, 0.502f, 0.502f, 1.000f);
  colors[ImGuiCol_Separator] = ImVec4(0.627f, 0.627f, 0.627f, 1.000f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.627f, 0.627f, 0.627f, 1.000f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.627f, 0.627f, 0.627f, 1.000f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.376f, 0.376f, 0.376f, 1.000f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.667f, 0.667f, 0.667f, 1.000f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.941f, 0.941f, 0.941f, 1.000f);
  colors[ImGuiCol_Tab] = ImVec4(0.502f, 0.502f, 0.502f, 0.251f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.000f, 0.541f, 1.000f, 0.502f);
  colors[ImGuiCol_TabSelected] = ImVec4(0.875f, 0.942f, 1.000f, 0.251f);
  colors[ImGuiCol_TabDimmed] = ImVec4(0.502f, 0.502f, 0.502f, 0.251f);
  colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.875f, 0.942f, 1.000f, 0.251f);
  colors[ImGuiCol_DockingPreview] = ImVec4(0.267f, 0.664f, 1.000f, 0.702f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.376f, 0.376f, 0.376f, 1.000f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.898f, 0.953f, 1.000f, 1.000f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.800f, 0.910f, 1.000f, 1.000f);
  colors[ImGuiCol_TableHeaderBg] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.376f, 0.376f, 0.376f, 1.000f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.773f, 0.773f, 0.773f, 1.000f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.941f, 0.941f, 0.941f, 1.000f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.941f, 0.941f, 0.941f, 1.000f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.000f, 0.476f, 0.843f, 0.502f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(0.800f, 0.910f, 1.000f, 1.000f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.800f, 0.922f, 1.000f, 1.000f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.941f, 0.941f, 0.941f, 0.376f);
}

static String get_style_blk_path()
{
  String path(256, "%s%s", sgg::get_exe_path_full(), "../commonData/imgui_daEditorX_style.blk");
  simplify_fname(path);
  return path;
}

static bool save_imgui_style()
{
  DataBlock block;

  G_STATIC_ASSERT(sizeof(Point2) == sizeof(ImVec2));
  for (int i = 0; i < ImGuiStyleVar_COUNT; ++i)
  {
    const ImGuiDataVarInfo *varInfo = ImGui::GetStyleVarInfo(i);
    if (varInfo->Type != ImGuiDataType_Float)
    {
      logerr("ImGui: skipping style variable %d, it has unsupported type!", i);
      continue;
    }

    String varName(32, "Style_%d", i);

    if (varInfo->Count == 1)
    {
      float *value = (float *)varInfo->GetVarPtr(&ImGui::GetStyle());
      block.addReal(varName, *value);
    }
    else if (varInfo->Count == 2)
    {
      Point2 *value = (Point2 *)varInfo->GetVarPtr(&ImGui::GetStyle());
      block.addPoint2(varName, *value);
    }
    else
    {
      logerr("ImGui: skipping style variable %d, it has unsupported type!", i);
    }
  }

  G_STATIC_ASSERT(sizeof(Point4) == sizeof(ImVec4));
  for (int i = 0; i < ImGuiCol_COUNT; ++i)
  {
    String varName(32, "Color_%d", i);
    const Point4 *value = reinterpret_cast<const Point4 *>(&ImGui::GetCurrentContext()->Style.Colors[i]);
    block.addPoint4(varName, *value);
  }

  const bool succeeded = block.saveToTextFile(get_style_blk_path());
  if (!succeeded)
    logerr("ImGui: failed to save style file '%s'.", get_style_blk_path().c_str());
  return succeeded;
}

static bool load_imgui_style()
{
  DataBlock block;
  const bool succeeded = block.load(get_style_blk_path());
  if (!succeeded)
  {
    logerr("ImGui: failed to load style file '%s'.", get_style_blk_path().c_str());
    return succeeded;
  }

  G_STATIC_ASSERT(sizeof(Point2) == sizeof(ImVec2));
  for (int i = 0; i < ImGuiStyleVar_COUNT; ++i)
  {
    const ImGuiDataVarInfo *varInfo = ImGui::GetStyleVarInfo(i);
    if (varInfo->Type != ImGuiDataType_Float)
    {
      logerr("ImGui: skipping style variable %d, it has unsupported type!", i);
      continue;
    }

    String varName(32, "Style_%d", i);
    const int paramIndex = block.findParam(varName);
    if (paramIndex < 0)
    {
      logerr("ImGui: style variable %s is not in the blk.", i);
      continue;
    }

    if (varInfo->Count == 1 && block.getParamType(paramIndex) == DataBlock::TYPE_REAL)
    {
      float *value = (float *)varInfo->GetVarPtr(&ImGui::GetStyle());
      *value = block.getReal(varName);
    }
    else if (varInfo->Count == 2 && block.getParamType(paramIndex) == DataBlock::TYPE_POINT2)
    {
      Point2 *value = (Point2 *)varInfo->GetVarPtr(&ImGui::GetStyle());
      *value = block.getPoint2(varName);
    }
    else
    {
      logerr("ImGui: blk type and style type for %s is different! Variable will not be loaded.", i);
    }
  }

  G_STATIC_ASSERT(sizeof(Point4) == sizeof(ImVec4));
  for (int i = 0; i < ImGuiCol_COUNT; ++i)
  {
    String varName(32, "Color_%d", i);
    const int paramIndex = block.findParam(varName);
    if (paramIndex < 0)
    {
      logerr("ImGui: color variable %s is not in the blk.", i);
      continue;
    }

    if (block.getParamType(paramIndex) != DataBlock::TYPE_POINT4)
    {
      logerr("ImGui: expected point4 type for %s! Variable will not be loaded.", i);
      continue;
    }

    Point4 *value = reinterpret_cast<Point4 *>(&ImGui::GetCurrentContext()->Style.Colors[i]);
    *value = block.getPoint4(varName);
  }

  return succeeded;
}

void editor_core_initialize_imgui(const char *imgui_ini_path)
{
  DataBlock overrideBlk;
  overrideBlk.setReal("active_window_bg_alpha", 1.0f);

  const float scale = clamp(win32_system_dpi / 96.0f, 1.0f, 5.0f);
  overrideBlk.setReal("imgui_scale", scale);
  overrideBlk.setReal("imgui_font_size", 16.0f);      // This gets scaled with imgui_scale in imgui_apply_style_from_blk.
  overrideBlk.setReal("imgui_bold_font_size", 16.0f); // This gets scaled with imgui_scale in imgui_apply_style_from_blk.

  String fontPath(256, "%s%s", sgg::get_exe_path_full(), "../commonData/fonts/roboto-regular.ttf");
  simplify_fname(fontPath);
  overrideBlk.setStr("imgui_font_name", fontPath);

  fontPath.printf(256, "%s%s", sgg::get_exe_path_full(), "../commonData/fonts/roboto-bold.ttf");
  simplify_fname(fontPath);
  overrideBlk.setStr("imgui_bold_font_name", fontPath);

  overrideBlk.setBool("imgui_font_light_hinting", true);

  imgui_set_override_blk(overrideBlk);

  imgui_set_blk_path(nullptr); // Disable imgui.blk load/save.
  imgui_init_on_demand();

  ImGuiIO &imguiIo = ImGui::GetIO();

  imguiIo.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  imguiIo.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::GetStyle() = ImGuiStyle(); // Reset the style because it has been scaled in imgui_apply_style_from_blk.
  apply_imgui_style();
  ImGui::GetStyle().ScaleAllSizes(scale);

  ImGui::GetStyle().HoverStationaryDelay = 0.5f; // Use the default Windows tooltip hover delay.

  // IniFilename is set by init() in prog/engine/imgui/imguiImpl.cpp. The assert is here to ensure that it was not
  // used yet.
  G_ASSERT(!ImGui::GetCurrentContext()->SettingsLoaded);
  imguiIo.IniFilename = nullptr;
  imgui_ini_file_path = imgui_ini_path;
  if (!imgui_ini_file_path.empty())
  {
    logdbg("Loading ImGui settings from '%s'.", imgui_ini_file_path);
    ImGui::LoadIniSettingsFromDisk(imgui_ini_file_path);
  }

  if (imgui_get_state() != ImGuiState::ACTIVE)
    imgui_request_state_change(ImGuiState::ACTIVE);

  wingw::set_native_modal_dialog_events(&imgui_native_modal_dialog_event_handler);
}

void editor_core_save_imgui_settings()
{
  if (imgui_ini_file_path.empty() || !ImGui::GetIO().WantSaveIniSettings)
    return;

  ImGui::SaveIniSettingsToDisk(imgui_ini_file_path);
}

// NOTE: ImGui porting: style editor will not be needed once we have a final theme. At least the load-save
// functionality will not be needed. The style editor itself is useful when making components.
void editor_core_update_imgui_style_editor()
{
  static bool imguiStyleEditorWindowsVisible = false;
  if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_F12))
    imguiStyleEditorWindowsVisible = !imguiStyleEditorWindowsVisible;
  if (!imguiStyleEditorWindowsVisible)
    return;

  ImGui::Begin("Dear ImGui Style Editor", &imguiStyleEditorWindowsVisible);

  static bool saveError = false;
  if (ImGui::Button("Save to blk"))
    saveError = !save_imgui_style();

  if (saveError)
  {
    ImGui::OpenPopup("Save error");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Save error", &saveError, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::Text("Error saving the file. See the log.");
      ImGui::EndPopup();
    }
  }

  ImGui::SameLine();

  static bool loadError = false;
  if (ImGui::Button("Load from blk"))
    loadError = !load_imgui_style();

  if (loadError)
  {
    ImGui::OpenPopup("Load error");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Load error", &loadError, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::Text("Error loading the file. See the log.");
      ImGui::EndPopup();
    }
  }

  ImGui::Separator();
  ImGui::ShowStyleEditor();

  ImGui::End();
}

bool editor_core_imgui_begin(const char *name, bool *open, unsigned window_flags)
{
  const ImVec2 minSize(200.0f, 200.0f);
  const ImVec2 maxSize(ImGui::GetIO().DisplaySize * 0.9f);
  ImGui::SetNextWindowSizeConstraints(minSize, ImVec2(max(minSize.x, maxSize.x), max(minSize.y, maxSize.y)));

  // Change the tab title color for the daEditorX Classic style.
  ImGui::PushStyleColor(ImGuiCol_Text, PropPanel::Constants::DIALOG_TITLE_COLOR);

  const bool result = ImGui::Begin(name, open, window_flags | ImGuiWindowFlags_NoFocusOnAppearing);

  ImGui::PopStyleColor();

  // Auto focus the window.
  if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
      !ImGui::IsAnyItemActive() && !ImGui::GetIO().WantTextInput)
    ImGui::FocusWindow(ImGui::GetCurrentWindow(), ImGuiFocusRequestFlags_RestoreFocusedChild);

  return result;
}
