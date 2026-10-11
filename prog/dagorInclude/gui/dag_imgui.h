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
#include <util/dag_string.h>
#include <drv/3d/dag_resId.h>

class DataBlock;
struct ImFont;
struct ImDrawData;
class BaseTexture;
struct ImRect;

enum class ImGuiState
{
  OFF,
  ACTIVE,
  OVERLAY,
  _COUNT, // For iteration purposes only, do not use!
};

using OnStateChangeHandlerFunc = eastl::function<void(ImGuiState, ImGuiState)>;
using OnCaptureDrawDataFunc = eastl::function<void(int &, int &, int &, unsigned int *)>;

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
void imgui_capture_window_drawdata(const char *window_title, OnCaptureDrawDataFunc func);
DataBlock *imgui_get_blk();
void imgui_save_blk();
void imgui_window_set_visible(const char *group, const char *name, const bool visible);
bool imgui_window_is_visible(const char *group, const char *name);
bool imgui_window_is_collapsed(const char *group, const char *name);
void imgui_window_request_focus(const char *group, const char *name);
void imgui_perform_registered(bool with_menu_bar = true);
void imgui_cascade_windows();
void imgui_set_bold_font();
void imgui_set_mono_font();
void imgui_set_default_font();
ImFont *imgui_get_bold_font();
ImFont *imgui_get_mono_font();
// name: identifier for the custom font. Pass this name to imgui_get_custom_font() to get the font.
void imgui_add_custom_font(const char *name, const char *font_file_path, int font_size, unsigned int extra_loader_flags = 0);
ImFont *imgui_get_custom_font(const char *name);
void imgui_apply_fonts_from_blk();
void imgui_apply_style_from_blk();
int imgui_get_menu_bar_height();
void imgui_set_blk_path(const char *path); // Setting path to null or an empty string will disable blk load/save.
void imgui_set_ini_path(const char *path);
void imgui_set_log_path(const char *path);
enum ImGuiKey : int;
eastl::optional<ImGuiKey> map_dagor_key_to_imgui(int humap_key);
int map_imgui_key_to_dagor(int imgui_key);
void *convert_dag_res_id_to_imgui(D3DRESID res_id);

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

struct ImGuiTestRuntimeOptions
{
  void *engine = nullptr;
  bool drawHovered = false;
  uint32_t drawItemId = 0;
  bool drawFakeMousePointer = false;
  bool copyHoveredPath = false;
  String hoveredPath; // Set by imgui_endframe. Lags one frame behind the hovered item.
};

// Extra information the application attaches to an ImGui item for the test runtime.
struct ImGuiTestItemExternalInfo
{
  // Optional, human readable name of the item. The hovered item overlay and imgui.dump_tree console command
  // show it in place of the ImGui label. A query reports this name here and the ImGui label separately.
  // Copied by its contents, a temporary value is safe here.
  const char *displayName = nullptr;

  // Optional part name for a control that draws several sub-items.
  // For example the x and y component of Point2. Registered as "<displayName>.x" and "<displayName>.y".
  // Input only, a query returns the combined name in displayName.
  // Copied by its contents, a temporary value is safe here.
  const char *subcomponentName = nullptr;

  // Optional, application-defined type of the item, like "Button" or "Point3".
  // Copied by pointer. It must have a static storage duration. A temporary value is not safe here.
  const char *controlType = nullptr;

  // Optional, application-defined ID of the item. For example PropertyControlBase::mId, where 0
  // is the idiom for a control with no ID of its own, so read a queried 0 as "no ID".
  int controlId = 0;
};

// ImGui state of a queried item.
// hovered and active compare the item with the ImGui context, so they are valid only when the query runs after the
// frame submitted all its items (as the EditorCore test thread does). They have no value for an item that only the
// previous frame submitted, while visible and toggledOpen then describe that previous frame.
struct ImGuiTestItemState
{
  // ImGui HoveredId is this item, or for a window, ImGui HoveredWindow is this window. Only the innermost window under
  // the mouse is marked hovered, not its parent windows.
  eastl::optional<bool> hovered;

  // ImGui ActiveId is this item, for example a button that the mouse holds down. Always false for a window.
  eastl::optional<bool> active;

  // The item overlaps the clip rect of its window. Always false for a window, and for an item that ImGui registers
  // without ItemAdd() (ImGui::ButtonBehavior() does this for a caller that did not call ItemAdd()).
  bool visible = false;

  // A tree node changed its open state in the frame that submitted it.
  bool toggledOpen = false;

  // The item is a window. ImGui::EndChild() also adds a second item for the child window to its parent window. That
  // item is not a window.
  bool isWindow = false;
};

bool imgui_test_runtime_set(bool enabled, ImGuiTestRuntimeOptions *options = nullptr);

// Attaches the external information to the ImGui item with the given ImGui ID.
// Call this after the ImGui item's ItemAdd().
// Does nothing while the test runtime is off, or when the ID has no item in the current frame.
// ext.subcomponentName is only used if ext.displayName is set.
void imgui_test_runtime_set_item_info(uint32_t item_id, const ImGuiTestItemExternalInfo &ext);

// Attaches the external information to the last submitted ImGui item.
// Call this immediately after the ImGui item's draw.
// Does nothing while the test runtime is off, or in a window with SkipItems set.
// ext.subcomponentName is only used if ext.displayName is set.
void imgui_test_runtime_set_last_item_info(const ImGuiTestItemExternalInfo &ext);

// Finds the item at the given '/' separated path. At each level it takes the first item whose
// application set name or ImGui label matches, so duplicate names hide all but the first.
// A name that contains '/' must be escaped as "\/" in the path. (The hovered item overlay displays
// the escaped path.)
// The returned label and ext.displayName point into the runtime's per frame item tree and stay valid
// only until the next frame. ext.controlType is the static pointer the application registered, so it
// has no such limit.
bool imgui_test_runtime_query_item(const char *path, uint32_t &id, ImRect &bb, uint32_t &parent_id, const char **label = nullptr,
  ImGuiTestItemExternalInfo *ext = nullptr, ImGuiTestItemState *state = nullptr);
// The label and ext given to the callback have the same lifetime as the ones imgui_test_runtime_query_item() returns.
// The state is valid only until the callback returns.
using OnTestChildItemInfoFoundFunc =
  eastl::function<void(const char *, uint32_t, ImRect, uint32_t, const ImGuiTestItemExternalInfo &, const ImGuiTestItemState &)>;
int imgui_test_runtime_query_children(const char *path, OnTestChildItemInfoFoundFunc func);

#define REGISTER_IMGUI_WINDOW(group, name, func) \
  static ImGuiFunctionQueue DAG_CONCAT(AutoImGuiWindow, __LINE__)(group, name, nullptr, 100, 0, func, true)
#define REGISTER_IMGUI_WINDOW_EX(group, name, hotkey, priority, flags, func) \
  static ImGuiFunctionQueue DAG_CONCAT(AutoImGuiWindow, __LINE__)(group, name, hotkey, priority, flags, func, true)
#define REGISTER_IMGUI_FUNCTION(group, name, func) \
  static ImGuiFunctionQueue DAG_CONCAT(AutoImGuiFunction, __LINE__)(group, name, nullptr, 100, 0, func, false)
#define REGISTER_IMGUI_FUNCTION_EX(group, name, hotkey, priority, func) \
  static ImGuiFunctionQueue DAG_CONCAT(AutoImGuiFunction, __LINE__)(group, name, hotkey, priority, 0, func, false)
