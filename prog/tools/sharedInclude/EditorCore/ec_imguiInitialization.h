// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>

// The path of the ini file that ImGui will use to store window state.
// It is recommended to pass a full path.
void editor_core_initialize_imgui(const char *imgui_ini_path);

void editor_core_save_imgui_settings();

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
