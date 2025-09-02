// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_input.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_wndGlobal.h>
#include <gui/dag_imgui.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <propPanel/commonWindow/dialogManager.h>
#include <propPanel/c_util.h>
#include <propPanel/colors.h>
#include <propPanel/constants.h>
#include <propPanel/propPanel.h>
#include <util/dag_string.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

static void start_non_busy();
static void end_non_busy();

class EditorCoreModalDialogEventHandler : public PropPanel::IModalDialogEventHandler
{
public:
  void beforeModalDialogShown() override { start_non_busy(); }
  void afterModalDialogShown() override { end_non_busy(); }
};

class ImguiNativeModalDialogEventHandler : public wingw::INativeModalDialogEventHandler
{
public:
  void beforeModalDialogShown() override { start_non_busy(); }

  void afterModalDialogShown() override
  {
    end_non_busy();

    // Workaround for stuck keys after displaying a native modal dialog or message box in reaction to a key press.
    // This is needed because in case of modal dialogs and message boxes the focus loss and focus gain happen in
    // the same ImGui frame, so as far as ImGui is concerned nothing happened, and it would report the key pressed
    // forever.
    // https://github.com/ocornut/imgui/issues/7264
    if (ImGui::GetCurrentContext())
    {
      ImGui::GetIO().ClearInputKeys();
      ImGui::GetIO().ClearInputMouse();
      ImGui::GetIO().ClearEventsQueue();
    }
  }
};

static EditorCoreModalDialogEventHandler modal_dialog_event_handler;
static ImguiNativeModalDialogEventHandler imgui_native_modal_dialog_event_handler;
static dag::Vector<bool> busy_stack;
static String imgui_ini_file_path;
static int imgui_dock_settings_version = 0;

static void start_non_busy() { busy_stack.push_back(ec_set_busy(false)); }

static void end_non_busy()
{
  G_ASSERT(!busy_stack.empty());
  ec_set_busy(busy_stack.back());
  busy_stack.pop_back();
}

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

  PropPanel::applyClassicOverrides();
}

static const char *get_imgui_style_var_name(ImGuiStyleVar idx)
{
  G_STATIC_ASSERT(ImGuiStyleVar_COUNT == 34);
  switch (idx)
  {
    case ImGuiStyleVar_Alpha: return "Alpha";
    case ImGuiStyleVar_DisabledAlpha: return "DisabledAlpha";
    case ImGuiStyleVar_WindowPadding: return "WindowPadding";
    case ImGuiStyleVar_WindowRounding: return "WindowRounding";
    case ImGuiStyleVar_WindowBorderSize: return "WindowBorderSize";
    case ImGuiStyleVar_WindowMinSize: return "WindowMinSize";
    case ImGuiStyleVar_WindowTitleAlign: return "WindowTitleAlign";
    case ImGuiStyleVar_ChildRounding: return "ChildRounding";
    case ImGuiStyleVar_ChildBorderSize: return "ChildBorderSize";
    case ImGuiStyleVar_PopupRounding: return "PopupRounding";
    case ImGuiStyleVar_PopupBorderSize: return "PopupBorderSize";
    case ImGuiStyleVar_FramePadding: return "FramePadding";
    case ImGuiStyleVar_FrameRounding: return "FrameRounding";
    case ImGuiStyleVar_FrameBorderSize: return "FrameBorderSize";
    case ImGuiStyleVar_ItemSpacing: return "ItemSpacing";
    case ImGuiStyleVar_ItemInnerSpacing: return "ItemInnerSpacing";
    case ImGuiStyleVar_IndentSpacing: return "IndentSpacing";
    case ImGuiStyleVar_CellPadding: return "CellPadding";
    case ImGuiStyleVar_ScrollbarSize: return "ScrollbarSize";
    case ImGuiStyleVar_ScrollbarRounding: return "ScrollbarRounding";
    case ImGuiStyleVar_GrabMinSize: return "GrabMinSize";
    case ImGuiStyleVar_GrabRounding: return "GrabRounding";
    case ImGuiStyleVar_TabRounding: return "TabRounding";
    case ImGuiStyleVar_TabBorderSize: return "TabBorderSize";
    case ImGuiStyleVar_TabBarBorderSize: return "TabBarBorderSize";
    case ImGuiStyleVar_TabBarOverlineSize: return "TabBarOverlineSize";
    case ImGuiStyleVar_TableAngledHeadersAngle: return "TableAngledHeadersAngle";
    case ImGuiStyleVar_TableAngledHeadersTextAlign: return "TableAngledHeadersTextAlign";
    case ImGuiStyleVar_ButtonTextAlign: return "ButtonTextAlign";
    case ImGuiStyleVar_SelectableTextAlign: return "SelectableTextAlign";
    case ImGuiStyleVar_SeparatorTextBorderSize: return "SeparatorTextBorderSize";
    case ImGuiStyleVar_SeparatorTextAlign: return "SeparatorTextAlign";
    case ImGuiStyleVar_SeparatorTextPadding: return "SeparatorTextPadding";
    case ImGuiStyleVar_DockingSeparatorSize: return "DockingSeparatorSize";
  }
  G_ASSERT(0);
  return "Unknown";
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

    const char *styleVarName = get_imgui_style_var_name(i);
    String varName = String::mk_str_cat("Style_", styleVarName);

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
    const char *styleColorName = ImGui::GetStyleColorName(i);
    String varName = String::mk_str_cat("Color_", styleColorName);
    const Point4 *value = reinterpret_cast<const Point4 *>(&ImGui::GetCurrentContext()->Style.Colors[i]);
    block.addPoint4(varName, *value);
  }

  for (int i = 0; i < PropPanel::ColorOverride::COUNT; ++i)
  {
    PropPanel::ColorOverride &c = PropPanel::getColorOverride(i);
    String varName = String::mk_str_cat("ColorOverride_", c.name);
    const Point4 *value = reinterpret_cast<Point4 *>(&c.color);
    block.addPoint4(varName, *value);
  }

  const bool succeeded = block.saveToTextFile(get_style_blk_path());
  if (!succeeded)
    logerr("ImGui: failed to save style file '%s'.", get_style_blk_path().c_str());
  return succeeded;
}

static bool load_imgui_style_from(const char *path)
{
  DataBlock block;
  const bool succeeded = block.load(path);
  if (!succeeded)
  {
    logerr("ImGui: failed to load style file '%s'.", path);
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

    const char *styleVarName = get_imgui_style_var_name(i);
    String varName = String::mk_str_cat("Style_", styleVarName);
    int paramIndex = block.findParam(varName);
    if (paramIndex < 0)
    {
      varName = String(32, "Style_%d", i);
      paramIndex = block.findParam(varName);
      if (paramIndex < 0)
      {
        logerr("ImGui: style variable 'Style_%s' (nor 'Style_%d') is not in the blk.", styleVarName, i);
        continue;
      }
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
    const char *styleColorName = ImGui::GetStyleColorName(i);
    String varName = String::mk_str_cat("Color_", styleColorName);
    int paramIndex = block.findParam(varName);
    if (paramIndex < 0)
    {
      varName = String(32, "Color_%d", i);
      paramIndex = block.findParam(varName);
      if (paramIndex < 0)
      {
        logerr("ImGui: color variable 'Color_%s' (nor 'Color_%d') is not in the blk.", styleColorName, i);
        continue;
      }
    }

    if (block.getParamType(paramIndex) != DataBlock::TYPE_POINT4)
    {
      logerr("ImGui: expected point4 type for '%s'! Variable will not be loaded.", varName.c_str());
      continue;
    }

    Point4 *value = reinterpret_cast<Point4 *>(&ImGui::GetCurrentContext()->Style.Colors[i]);
    *value = block.getPoint4(varName);
  }

  for (int i = 0; i < PropPanel::ColorOverride::COUNT; ++i)
  {
    PropPanel::ColorOverride &c = PropPanel::getColorOverride(i);
    String varName = String::mk_str_cat("ColorOverride_", c.name);
    int paramIndex = block.findParam(varName);
    if (paramIndex < 0)
    {
      logerr("ImGui: color override variable 'ColorOverride_%s' is not in the blk.", c.name);
      continue;
    }

    Point4 *value = reinterpret_cast<Point4 *>(&c.color);
    *value = block.getPoint4(varName);
  }

  return succeeded;
}

static bool load_imgui_style() { return load_imgui_style_from(get_style_blk_path()); }

static float get_imgui_scale() { return clamp(win32_system_dpi / 96.0f, 1.0f, 5.0f); }

static void *custom_dock_read_open(ImGuiContext *ctx, ImGuiSettingsHandler *handler, const char *name)
{
  return strcmp(name, "Data") == 0 ? (void *)1 : 0;
}

static void custom_dock_read_line(ImGuiContext *ctx, ImGuiSettingsHandler *handler, void *entry, const char *line)
{
  line = ImStrSkipBlank(line);
  sscanf(line, "DockSettingsVersion=%d", &imgui_dock_settings_version);
}

static void custom_dock_write_all(ImGuiContext *ctx, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
{
  if (imgui_dock_settings_version <= 0)
    return;

  buf->appendf("[CustomDock][Data]\n");
  buf->appendf("DockSettingsVersion=%d\n", imgui_dock_settings_version);
}

static void register_custom_dock_settings_handler()
{
  ImGuiSettingsHandler settingsHandler;
  settingsHandler.TypeName = "CustomDock";
  settingsHandler.TypeHash = ImHashStr(settingsHandler.TypeName);
  settingsHandler.ReadOpenFn = custom_dock_read_open;
  settingsHandler.ReadLineFn = custom_dock_read_line;
  settingsHandler.WriteAllFn = custom_dock_write_all;
  ImGui::AddSettingsHandler(&settingsHandler);
}

void editor_core_initialize_imgui(const char *imgui_ini_path, int *dock_settings_version)
{
  DataBlock overrideBlk;
  overrideBlk.setReal("active_window_bg_alpha", 1.0f);

  const float scale = get_imgui_scale();
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

#if !_TARGET_PC_WIN
  overrideBlk.setBool("imgui_multiview", false);
#endif

  imgui_set_override_blk(overrideBlk);

  imgui_set_blk_path(nullptr); // Disable imgui.blk load/save.

  // In tools the main window is different from win32_get_main_wnd().
  imgui_set_main_window_override(PropPanel::p2util::get_main_parent_handle());

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
  imgui_dock_settings_version = 0;
  if (!imgui_ini_file_path.empty())
  {
    logdbg("Loading ImGui settings from '%s'.", imgui_ini_file_path);
    register_custom_dock_settings_handler();
    ImGui::LoadIniSettingsFromDisk(imgui_ini_file_path);
  }

  if (imgui_get_state() != ImGuiState::ACTIVE)
    imgui_request_state_change(ImGuiState::ACTIVE);

  PropPanel::set_modal_dialog_events(&modal_dialog_event_handler);
  wingw::set_native_modal_dialog_events(&imgui_native_modal_dialog_event_handler);

  if (dock_settings_version)
    *dock_settings_version = imgui_dock_settings_version;
}

void editor_core_save_imgui_settings(int dock_settings_version)
{
  if (imgui_ini_file_path.empty())
    return;

  imgui_dock_settings_version = dock_settings_version;
  ImGui::SaveIniSettingsToDisk(imgui_ini_file_path);
}

void editor_core_save_dag_imgui_blk_settings(DataBlock &dst_blk)
{
  const DataBlock *imguiBlk = imgui_get_blk();
  if (!imguiBlk)
    return;

  DataBlock *dstDagImguiBlk = dst_blk.addBlock("dagImgui");
  for (int i = 0; i < imguiBlk->blockCount(); ++i)
  {
    const DataBlock *imguiBlkChild = imguiBlk->getBlock(i);
    const char *blkName = imguiBlkChild->getBlockName();
    if (strcmp(blkName, "ConsoleWindow") == 0)
      dstDagImguiBlk->setBlock(imguiBlkChild);
    else if (strcmp(blkName, "animgraph_viewer_bookmarked_states") == 0)
      dstDagImguiBlk->setBlock(imguiBlkChild);
    else if (strcmp(blkName, "animgraph_viewer_bookmarked_params") == 0)
      dstDagImguiBlk->setBlock(imguiBlkChild);
  }
}

void editor_core_load_dag_imgui_blk_settings(const DataBlock &src_blk)
{
  const DataBlock *srcDagImguiBlk = src_blk.getBlockByName("dagImgui");
  DataBlock *imguiBlk = imgui_get_blk();
  if (srcDagImguiBlk && imguiBlk)
    for (int i = 0; i < srcDagImguiBlk->blockCount(); ++i)
      imguiBlk->setBlock(srcDagImguiBlk->getBlock(i));
}

void editor_core_load_imgui_theme(const char *fname)
{
  String path(256, "%s%s%s", sgg::get_exe_path_full(), "../commonData/themes/", fname);
  simplify_fname(path);

  ImGui::GetStyle() = ImGuiStyle();
  apply_imgui_style();
  if (!load_imgui_style_from(path))
  {
    logerr("ImGui: Error loading theme \"%s\", falling back to classic style!", path.c_str());

    ImGui::GetStyle() = ImGuiStyle();
    apply_imgui_style();
  }

  ImGui::GetStyle().ScaleAllSizes(get_imgui_scale());

  String iconDirPath(512, "%s../commonData/icons", sgg::get_exe_path_full());
  simplify_fname(iconDirPath);
  append_slash(iconDirPath);

  const String themeName = get_file_name_wo_ext(fname);
  String iconThemeDirPath(512, "%s../commonData/icons/%s", sgg::get_exe_path_full(), themeName.c_str());
  simplify_fname(iconThemeDirPath);
  append_slash(iconThemeDirPath);

  if (dd_dir_exists(iconThemeDirPath))
  {
    PropPanel::p2util::set_icon_path(iconThemeDirPath.c_str());
    PropPanel::p2util::set_icon_fallback_path(iconDirPath.c_str());
  }
  else
  {
    PropPanel::p2util::set_icon_path(iconDirPath.c_str());
    PropPanel::p2util::set_icon_fallback_path(nullptr);
  }

  PropPanel::reload_all_icons();
}

E3DCOLOR editor_core_load_window_background_color(const char *theme_filename)
{
  const String themePath = make_full_path(sgg::get_exe_path_full(), String::mk_str_cat("../commonData/themes/", theme_filename));
  DataBlock themeBlock;
  dblk::load(themeBlock, themePath, dblk::ReadFlag::ROBUST);
  const Point4 bgColor = themeBlock.getPoint4("Color_ChildBg", Point4(0.9f, 0.9f, 0.9f, 1.0f));
  return E3DCOLOR(real2uchar(bgColor.x), real2uchar(bgColor.y), real2uchar(bgColor.z));
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
static void editor_core_imgui_help_marker(const char *desc)
{
  ImGui::TextDisabled("(?)");
  if (ImGui::BeginItemTooltip())
  {
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
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

  PropPanel::updateDebugToolFlashColorOverride();

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

  if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
  {
    if (ImGui::BeginTabItem("Built-in"))
    {
      ImGui::ShowStyleEditor();

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Overrides"))
    {
      ImGuiStyle &style = ImGui::GetStyle();

      static ImVec4 ref[PropPanel::ColorOverride::COUNT];
      static bool init = false;
      if (!init)
      {
        for (int i = 0; i < PropPanel::ColorOverride::COUNT; ++i)
          ref[i] = PropPanel::getColorOverride(i).color;
        init = true;
      }

      static ImGuiTextFilter filter;
      filter.Draw("Filter colors", ImGui::GetFontSize() * 16);

      static ImGuiColorEditFlags alpha_flags = 0;
      if (ImGui::RadioButton("Opaque", alpha_flags == ImGuiColorEditFlags_None))
      {
        alpha_flags = ImGuiColorEditFlags_None;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Alpha", alpha_flags == ImGuiColorEditFlags_AlphaPreview))
      {
        alpha_flags = ImGuiColorEditFlags_AlphaPreview;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Both", alpha_flags == ImGuiColorEditFlags_AlphaPreviewHalf))
      {
        alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf;
      }
      ImGui::SameLine();

      editor_core_imgui_help_marker("In the color list:\n"
                                    "Left-click on color square to open color picker,\n"
                                    "Right-click to open edit options menu.");

      ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 10), ImVec2(FLT_MAX, FLT_MAX));
      ImGui::BeginChild("##colors", ImVec2(0, 0), ImGuiChildFlags_Border | ImGuiChildFlags_NavFlattened,
        ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
      ImGui::PushItemWidth(ImGui::GetFontSize() * -16);
      for (int i = 0; i < PropPanel::ColorOverride::COUNT; i++)
      {
        PropPanel::ColorOverride &colorOverride = PropPanel::getColorOverride(i);
        if (!filter.PassFilter(colorOverride.name))
          continue;
        ImGui::PushID(i);

        if (ImGui::Button("?"))
          PropPanel::debugFlashColorOverride(i);
        ImGui::SetItemTooltip("Flash given color to identify places where it is used.");
        ImGui::SameLine();

        ImGui::ColorEdit4("##color", (float *)&colorOverride.color, ImGuiColorEditFlags_AlphaBar | alpha_flags);
        if (colorOverride.color != ref[i])
        {
          ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
          if (ImGui::Button("Save"))
          {
            ref[i] = colorOverride.color;
          }
          ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
          if (ImGui::Button("Revert"))
          {
            colorOverride.color = ref[i];
          }
        }
        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        ImGui::TextUnformatted(colorOverride.name);
        const int idx = colorOverride.defaultIdx;
        if (0 <= idx && idx < ImGuiCol_COUNT)
        {
          ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
          String overriddenName = String::mk_str_cat(" - ", ImGui::GetStyleColorName(idx));
          ImGui::TextUnformatted(overriddenName);
        }
        ImGui::PopID();
      }
      ImGui::PopItemWidth();
      ImGui::EndChild();

      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  ImGui::End();
}

bool editor_core_imgui_begin(const char *name, bool *open, unsigned window_flags)
{
  ImGui::SetNextWindowSizeConstraints(ImVec2(200.0f, 200.0f), ImVec2(FLT_MAX, FLT_MAX));

  // Change the tab title color for the daEditorX Classic style.
  PropPanel::pushDialogTitleBarColorOverrides();

  const bool result = ImGui::Begin(name, open, window_flags | ImGuiWindowFlags_NoFocusOnAppearing);

  PropPanel::popDialogTitleBarColorOverrides();

  // Auto focus the window.
  if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
      !ImGui::IsAnyItemActive() && !ImGui::GetIO().WantTextInput)
    ImGui::FocusWindow(ImGui::GetCurrentWindow(), ImGuiFocusRequestFlags_RestoreFocusedChild);

  return result;
}
