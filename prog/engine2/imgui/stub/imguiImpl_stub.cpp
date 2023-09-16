#include <gui/dag_imgui.h>

void imgui_shutdown() {}
ImGuiState imgui_get_state() { return ImGuiState::OFF; }
bool imgui_want_capture_mouse() { return false; }
void imgui_request_state_change(ImGuiState new_state) {}
void imgui_register_on_state_change_handler(OnStateChangeHandlerFunc func) {}
void imgui_update() {}
void imgui_endframe() {}
void imgui_render() {}
DataBlock *imgui_get_blk() { return nullptr; }
void imgui_save_blk() {}
void imgui_window_set_visible(const char *, const char *, const bool) {}
bool imgui_window_is_visible(const char *, const char *) { return false; }
void imgui_perform_registered() {}

ImGuiFunctionQueue *ImGuiFunctionQueue::windowHead = nullptr;
ImGuiFunctionQueue *ImGuiFunctionQueue::functionHead = nullptr;
ImGuiFunctionQueue::ImGuiFunctionQueue(const char *group_, const char *name_, const char *hotkey_, int priority_, int flags_,
  ImGuiFuncPtr func, bool is_window)
{}