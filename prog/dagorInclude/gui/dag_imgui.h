//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// Visit "Dear ImGui and ImPlot" wiki page for information and examples on how to use ImGui in our tech:
// https://dagor.rtd.gaijin.lan/en/latest/dagor-tools/dear-imgui/dear_imgui.html
//
// Feel free to extend the documentation I linked above, or add additional features to our ImGui integration. Add me as
// reviewer if you do so (g.szaloki@gaijin.team). Write me on Pararam (@gabor_szaloki1) in case you have any ImGui-
// related questions or suggestions.

#include <EASTL/functional.h>
#include <EASTL/optional.h>
#include <util/dag_preprocessor.h>

class DataBlock;
struct ImFont;
struct ImDrawData;
class BaseTexture;

enum class ImGuiState
{
  OFF,
  ACTIVE,
  OVERLAY,
  _COUNT, // For iteration purposes only, do not use!
};

using OnStateChangeHandlerFunc = eastl::function<void(ImGuiState, ImGuiState)>;

bool imgui_init_on_demand();
void imgui_set_override_blk(const DataBlock &imgui_blk); // call this before imgui_init_on_demand() called
void imgui_set_main_window_override(void *hwnd);         // call this before imgui_init_on_demand() called
void imgui_enable_imgui_submenu(bool enabled);
void imgui_shutdown();
ImGuiState imgui_get_state();
bool imgui_want_capture_mouse();
void imgui_request_state_change(ImGuiState new_state);
void imgui_register_on_state_change_handler(OnStateChangeHandlerFunc func);
void imgui_update(int display_width = 0, int display_height = 0);
void imgui_endframe();
void imgui_render();
void imgui_render_drawdata_to_texture(ImDrawData *draw_data, BaseTexture *rt);
DataBlock *imgui_get_blk();
void imgui_save_blk();
void imgui_window_set_visible(const char *group, const char *name, const bool visible);
bool imgui_window_is_visible(const char *group, const char *name);
bool imgui_window_is_collapsed(const char *group, const char *name);
void imgui_perform_registered(bool with_menu_bar = true);
void imgui_cascade_windows();
void imgui_set_bold_font();
void imgui_set_mono_font();
void imgui_set_default_font();
ImFont *imgui_get_bold_font();
ImFont *imgui_get_mono_font();
void imgui_apply_style_from_blk();
int imgui_get_menu_bar_height();
void imgui_set_blk_path(const char *path); // Setting path to null or an empty string will disable blk load/save.
void imgui_set_ini_path(const char *path);
void imgui_set_log_path(const char *path);
enum ImGuiKey : int;
eastl::optional<ImGuiKey> map_dagor_key_to_imgui(int humap_key);
int map_imgui_key_to_dagor(int imgui_key);

typedef eastl::function<void(void)> ImGuiFuncPtr;

struct ImGuiFunctionQueue
{
  ImGuiFunctionQueue *next = nullptr; // single-linked list
  ImGuiFuncPtr function = nullptr;
  const char *group = nullptr;
  const char *name = nullptr;
  const char *hotkey = nullptr;
  int priority = 0; // lower the number, earlier it will be in the list
  int flags = 0;
  bool opened = false;
  static ImGuiFunctionQueue *windowHead;
  static ImGuiFunctionQueue *functionHead;
  ImGuiFunctionQueue(const char *group_, const char *name_, const char *hotkey_, int priority_, int flags_, ImGuiFuncPtr func,
    bool is_window);
};

#define REGISTER_IMGUI_WINDOW(group, name, func) \
  static ImGuiFunctionQueue DAG_CONCAT(AutoImGuiWindow, __LINE__)(group, name, nullptr, 100, 0, func, true)
#define REGISTER_IMGUI_WINDOW_EX(group, name, hotkey, priority, flags, func) \
  static ImGuiFunctionQueue DAG_CONCAT(AutoImGuiWindow, __LINE__)(group, name, hotkey, priority, flags, func, true)
#define REGISTER_IMGUI_FUNCTION(group, name, func) \
  static ImGuiFunctionQueue DAG_CONCAT(AutoImGuiFunction, __LINE__)(group, name, nullptr, 100, 0, func, false)
#define REGISTER_IMGUI_FUNCTION_EX(group, name, hotkey, priority, func) \
  static ImGuiFunctionQueue DAG_CONCAT(AutoImGuiFunction, __LINE__)(group, name, hotkey, priority, 0, func, false)
