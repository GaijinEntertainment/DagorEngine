//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// Visit "Dear ImGui and ImPlot" wiki page for information and examples on how to use ImGui in our tech:
// https://info.gaijin.lan/display/EL/Dear+ImGui+and+ImPlot
//
// Feel free to extend the documentation I linked above, or add additional features to our ImGui integration. Add me as
// reviewer if you do so (g.szaloki@gaijin.team). Write me on Pararam (@gabor_szaloki1) in case you have any ImGui-
// related questions or suggestions.

#include <EASTL/functional.h>

class DataBlock;

enum class ImGuiState
{
  OFF,
  ACTIVE,
  OVERLAY,
  _COUNT, // For iteration purposes only, do not use!
};

using OnStateChangeHandlerFunc = eastl::function<void(ImGuiState, ImGuiState)>;

bool init_on_demand();
void imgui_set_override_blk(const DataBlock &imgui_blk); // call this before init_on_demand() called
void imgui_shutdown();
ImGuiState imgui_get_state();
bool imgui_want_capture_mouse();
void imgui_request_state_change(ImGuiState new_state);
void imgui_register_on_state_change_handler(OnStateChangeHandlerFunc func);
void imgui_update();
void imgui_endframe();
void imgui_render();
DataBlock *imgui_get_blk();
void imgui_save_blk();
void imgui_window_set_visible(const char *group, const char *name, const bool visible);
bool imgui_window_is_visible(const char *group, const char *name);
void imgui_perform_registered();
void imgui_cascade_windows();

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

#define DAG_IGQ_CC0(a, b) a##b
#define DAG_IGQ_CC1(a, b) DAG_IGQ_CC0(a, b)
#define REGISTER_IMGUI_WINDOW(group, name, func) \
  static ImGuiFunctionQueue DAG_IGQ_CC1(AutoImGuiWindow, __LINE__)(group, name, nullptr, 100, 0, func, true)
#define REGISTER_IMGUI_WINDOW_EX(group, name, hotkey, priority, flags, func) \
  static ImGuiFunctionQueue DAG_IGQ_CC1(AutoImGuiWindow, __LINE__)(group, name, hotkey, priority, flags, func, true)
#define REGISTER_IMGUI_FUNCTION(group, name, func) \
  static ImGuiFunctionQueue DAG_IGQ_CC1(AutoImGuiFunction, __LINE__)(group, name, nullptr, 100, 0, func, false)
#define REGISTER_IMGUI_FUNCTION_EX(group, name, hotkey, priority, func) \
  static ImGuiFunctionQueue DAG_IGQ_CC1(AutoImGuiFunction, __LINE__)(group, name, hotkey, priority, 0, func, false)
