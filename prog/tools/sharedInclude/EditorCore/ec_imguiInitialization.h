// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>
#include <math/dag_e3dColor.h>

class DataBlock;

// Initialize the HumanInput-based input handling.
// Currently only used on Linux.
// It must be called after dagor_init_keyboard_win() and dagor_init_mouse_win().
void editor_core_initialize_input_handler();

// The path of the ini file that ImGui will use to store window state.
// It is recommended to pass a full path.
// dock_settings_version: optional output parameter. Contains the [CustomDock][Data]DockVersion's value from the ini. It
//   can be used to check whether the loaded dock settings are the latest.
void editor_core_initialize_imgui(const char *imgui_ini_path, int *dock_settings_version = nullptr);

// dock_settings_version: optional parameter. The specified value will be written to [CustomDock][Data]DockVersion in
//   the ini if it is larger than 0.
void editor_core_save_imgui_settings(int dock_settings_version = 0);

// The tools don't use dag imgui's imgui.blk, instead they store hand-picked settings from it.
// This function saves these hand-picked settings from the DataBlock returned by imgui_get_blk() and stores them into
// dst_blk.
void editor_core_save_dag_imgui_blk_settings(DataBlock &dst_blk);

// Load the hand-picked settings from src_blk, and apply them to the DataBlock returned by imgui_get_blk().
void editor_core_load_dag_imgui_blk_settings(const DataBlock &src_blk);

void editor_core_load_imgui_theme(const char *fname);

// This color can be used as the window's background color (the bg_color parameter in EditorMainWindow::run) before d3d
// takes over the rendering.
// theme_filename: name of the theme file (e.g.: light.blk)
E3DCOLOR editor_core_load_window_background_color(const char *theme_filename);

// Displays ImGui's built-in style editor window.
void editor_core_update_imgui_style_editor();

// Corresponds to ImGui::Begin() but it in addition:
// - auto focuses the window on hover
// - limits the maximum size of the window
// - it sets tab title color for the daEditorX Classic style
bool editor_core_imgui_begin(const char *name, bool *open, unsigned window_flags);

// This is the cursor that is shown when clicking on the viewport will just activate the viewport,
// and the next click will be the first one that actually "counts".
static constexpr int EDITOR_CORE_CURSOR_ADDITIONAL_CLICK = ImGuiMouseCursor_COUNT + 1;
