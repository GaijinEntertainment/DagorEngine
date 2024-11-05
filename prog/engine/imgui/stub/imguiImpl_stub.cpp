// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_imgui.h>

void imgui_shutdown() {}
ImGuiState imgui_get_state() { return ImGuiState::OFF; }
bool imgui_want_capture_mouse() { return false; }
void imgui_request_state_change(ImGuiState new_state) {}
void imgui_register_on_state_change_handler(OnStateChangeHandlerFunc func) {}
void imgui_update(int, int) {}
void imgui_endframe() {}
void imgui_render() {}
DataBlock *imgui_get_blk() { return nullptr; }
void imgui_save_blk() {}
void imgui_window_set_visible(const char *, const char *, const bool) {}
bool imgui_window_is_visible(const char *, const char *) { return false; }
void imgui_perform_registered(bool) {}
void imgui_set_bold_font() {}
void imgui_set_mono_font() {}
void imgui_set_default_font() {}
ImFont *imgui_get_bold_font() { return nullptr; }
ImFont *imgui_get_mono_font() { return nullptr; }
void imgui_apply_style_from_blk() {}
int imgui_get_menu_bar_height() { return 0; }

ImGuiFunctionQueue *ImGuiFunctionQueue::windowHead = nullptr;
ImGuiFunctionQueue *ImGuiFunctionQueue::functionHead = nullptr;
ImGuiFunctionQueue::ImGuiFunctionQueue(const char *group_, const char *name_, const char *hotkey_, int priority_, int flags_,
  ImGuiFuncPtr func, bool is_window)
{}