// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>
#include <math/dag_e3dColor.h>

class DataBlock;

// Initialize the HumanInput-based input handling.
// Currently only used on Linux.
// It must be called after dagor_init_keyboard_win() and dagor_init_mouse_win().
void editor_core_initialize_input_handler();

void editor_core_initialize_imgui();

// The tools don't use dag imgui's imgui.blk, instead they store hand-picked settings from it.
// This function saves these hand-picked settings from the DataBlock returned by imgui_get_blk() and stores them into
// dst_blk.
void editor_core_save_dag_imgui_blk_settings(DataBlock &dst_blk);

// Load the hand-picked settings from src_blk, and apply them to the DataBlock returned by imgui_get_blk().
void editor_core_load_dag_imgui_blk_settings(const DataBlock &src_blk);

// Close all open dag imgui windows.
void editor_core_close_dag_imgui_windows();

// Save the list of open dag imgui windows to the specified blk.
void editor_core_save_dag_imgui_windows(DataBlock &blk);

// Close all open dag imgui windows, and open those that were previously saved to the specified blk.
void editor_core_load_dag_imgui_windows(const DataBlock &blk);

// Same as imgui_perform_registered() but renders the windows with themed title bars.
void editor_core_render_dag_imgui_windows();

// Save the last used display options for ImGui's built-in color editor control (ColorEdit4).
// Display option can be RGB or HSV with range 0-255 or 0-1, or hexadecimal text.
void editor_core_save_imgui_color_editor_options(DataBlock &dst_blk);

// Load the display options for ImGui's built-in color editor control (ColorEdit4).
// Display option can be RGB or HSV with range 0-255 or 0-1, or hexadecimal text.
void editor_core_load_imgui_color_editor_options(const DataBlock &src_blk);

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

// The default size of the fonts used everywhere.
static constexpr int EDITOR_CORE_DEFAULT_FONT_SIZE = 16;
