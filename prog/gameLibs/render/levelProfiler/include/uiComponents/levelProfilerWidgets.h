// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>
#include <cstdio>
#include <EASTL/type_traits.h>
#include <EASTL/unique_ptr.h>
#include "levelProfilerInterface.h"
#include "levelProfilerFilter.h"

namespace levelprofiler
{

using SearchTextChangedCallback = eastl::function<void(const char *)>;
using SearchEnterCallback = eastl::function<void()>;

class LpProfilerTable;
template <typename T>
class LpRangeFilter;
template <typename T>
class CheckFilter;

// === Base Widgets ===

// --- MultiSelect Base Component - for checkbox lists with multi-selection support ---

enum LpMultiSelectFlags : int
{
  MULTISELECT_NONE = 0,
  NO_AUTO_SELECT = 1 << 0,
  NO_AUTO_CLEAR = 1 << 1,
  CLEAR_ON_ESCAPE = 1 << 2
};

inline LpMultiSelectFlags operator|(LpMultiSelectFlags a, LpMultiSelectFlags b)
{
  return static_cast<LpMultiSelectFlags>(static_cast<int>(a) | static_cast<int>(b));
}

inline LpMultiSelectFlags operator&(LpMultiSelectFlags a, LpMultiSelectFlags b)
{
  return static_cast<LpMultiSelectFlags>(static_cast<int>(a) & static_cast<int>(b));
}

struct LpMultiSelectIO
{
  LpMultiSelectFlags flags = LpMultiSelectFlags::MULTISELECT_NONE;
  int dirtySelection = -1; // First modified selection index, -1 if none
  int itemCount = 0;
  bool isEscapePressed = false;
  bool isSelectAllRequested = false;
  bool isClearAllRequested = false;
  bool isInvertAllRequested = false;
};

struct LpSelectionExternalStorage
{
  void *userData = nullptr; // User data for callbacks

  void (*adapter_set_item_selected)(LpSelectionExternalStorage *storage, int index, bool is_selected) = nullptr;

  void applyRequests(LpMultiSelectIO *multi_select_io);
};

// MultiSelect widget implementation - handles multi-selection checkbox lists
class LpMultiSelect
{
public:
  static LpMultiSelectIO *begin_multi_select(LpMultiSelectFlags multi_select_flags, int dirty_selection_index, int item_count);

  static LpMultiSelectIO *end_multi_select();

private:
  static LpMultiSelectIO multiSelectIoStatic; // Shared state for current widget
};

// --- Search Box - text input with clear button and callback support ---

class LpSearchBox
{
public:
  LpSearchBox(const char *identifier, const char *hint_text, SearchTextChangedCallback search_callback);

  void draw();

  const char *getText() const;

  void setText(const char *text);

  void clear();

private:
  const char *id;
  const char *hint;
  ProfilerString buffer;
  SearchTextChangedCallback callback;
};

// === Selection Widgets ===

// --- Label formatting helper functions for MultiSelectWidget ---

struct MultiSelectLabelFuncs
{
  // Format string label - direct pass-through
  static void ProfilerStringLabel(const ProfilerString &value, char *output_label, size_t buffer_size)
  {
    snprintf(output_label, buffer_size, "%s", value.c_str());
  }

  // Format int label with special handling for -1 as "Other"
  static void IntLabelWithOther(const int &value, char *output_label, size_t buffer_size)
  {
    if (value == -1)
      snprintf(output_label, buffer_size, "Other");
    else
      snprintf(output_label, buffer_size, "%d", value);
  }

  // Format int label - simple integer formatting
  static void IntLabel(const int &value, char *output_label, size_t buffer_size) { snprintf(output_label, buffer_size, "%d", value); }
};

// --- MultiSelect Widget - checkboxes with multi-selection support ---

template <typename T>
class LpMultiSelectWidget
{
public:
  using LabelFunc = void (*)(const T &value, char *output_label, size_t label_size);

  explicit LpMultiSelectWidget(LabelFunc label_function) : labelFunc(label_function) {}

  void Draw(const eastl::vector<T> &unique_values, CheckFilter<T> &filter_object, bool &has_changed)
  {
    static int lastSelectedIdx = -1;

    if (ImGui::Button("All"))
    {
      filter_object.reset();
      has_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Invert"))
    {
      filter_object.invert(unique_values);
      has_changed = true;
    }

    ImGui::Separator();

    // Begin MultiSelect with appropriate flags
    LpMultiSelectFlags multiSelectFlagsCurrent =
      LpMultiSelectFlags::NO_AUTO_SELECT | LpMultiSelectFlags::NO_AUTO_CLEAR | LpMultiSelectFlags::CLEAR_ON_ESCAPE;

    LpMultiSelectIO *multiSelectIo = LpMultiSelect::begin_multi_select(multiSelectFlagsCurrent, -1, (int)unique_values.size());

    Context context{&unique_values, &filter_object};
    LpSelectionExternalStorage selectionStorage;
    selectionStorage.userData = &context;
    selectionStorage.adapter_set_item_selected = &LpMultiSelectWidget<T>::update_selection;
    selectionStorage.applyRequests(multiSelectIo);

    // Check for shift key (range selection)
    ImGuiIO &imguiIo = ImGui::GetIO();
    const bool isShiftHeld = imguiIo.KeyShift;

    // Draw items
    for (int i = 0, n = (int)unique_values.size(); i < n; ++i)
    {
      const T &currentValue = unique_values[i];
      bool isItemActive = filter_object.pass(currentValue);

      ImGui::PushID(i);
      ProfilerString labelBuffer;
      char tempBuffer[64];
      labelFunc(currentValue, tempBuffer, sizeof(tempBuffer));
      labelBuffer = tempBuffer;

      if (const bool wasClicked = ImGui::Checkbox(labelBuffer.c_str(), &isItemActive))
      {
        if (isShiftHeld && lastSelectedIdx >= 0 && lastSelectedIdx != i)
        {
          // Shift+Click: apply state to all items in between
          for (int j = (lastSelectedIdx < i ? lastSelectedIdx : i), end = (lastSelectedIdx < i ? i : lastSelectedIdx); j <= end; ++j)
            if (const T &valueInRange = unique_values[j]; filter_object.pass(valueInRange) != isItemActive)
              filter_object.toggle(valueInRange);
        }
        else
          filter_object.toggle(currentValue);

        lastSelectedIdx = i;
        has_changed = true;
      }

      ImGui::PopID();
    }

    multiSelectIo = LpMultiSelect::end_multi_select();
    selectionStorage.applyRequests(multiSelectIo);
  }

private:
  struct Context
  {
    const eastl::vector<T> *values = nullptr;
    CheckFilter<T> *filter = nullptr;
  };

  static void update_selection(LpSelectionExternalStorage *storage, int index, bool is_selected)
  {
    auto context = static_cast<Context *>(storage->userData);

    if (!context || !context->values || !context->filter)
      return;

    if (const T &valueToUpdate = (*context->values)[index]; is_selected)
      context->filter->getOffList().erase(valueToUpdate);
    else
      context->filter->toggle(valueToUpdate);
  }

  LabelFunc labelFunc;
};

// --- Search Widget - combines search box with filter system ---

class LpSearchWidget
{
public:
  LpSearchWidget(LpProfilerTable * /*profiler_table*/, ColumnIndex /*column_index*/, const char *identifier, const char *hint_text,
    SearchTextChangedCallback on_search_callback, SearchEnterCallback apply_filters_callback);

  ~LpSearchWidget();

  const char *getText() const;

  void clear();

  void draw();

private:
  ProfilerString searchBuffer;
  eastl::unique_ptr<LpSearchBox> searchBox;
};

// === Range Widgets ===

// --- Range Filter Controller & UI Adapter ---
template <typename T>
class LpRangeFilterController
{
  static_assert(eastl::is_arithmetic<T>::value, "T must be arithmetic");

public:
  explicit LpRangeFilterController(LpRangeFilter<T> *f = nullptr) : filter(f) {}
  void bind(LpRangeFilter<T> *f) { filter = f; }
  bool isBound() const { return filter != nullptr; }
  void resetToDefaults()
  {
    if (filter)
      filter->reset(filter->getDefaultMin(), filter->getDefaultMax());
  }
  void setRange(T mn, T mx)
  {
    if (filter)
      filter->reset(mn, mx);
  }
  bool isActive() const { return filter && !filter->isDefault(); }
  LpRangeFilter<T> *raw() { return filter; }

private:
  LpRangeFilter<T> *filter = nullptr; // non-owning
};

enum class LpSplitterDirection
{
  HORIZONTAL,
  VERTICAL
};

class LpSplitter
{
public:
  LpSplitter(LpSplitterDirection direction, float initial_ratio = 0.5f, float min_ratio = 0.2f, float max_ratio = 0.8f);

  bool draw(const ImVec2 &available_area);

  float getRatio() const { return ratio; }
  void setRatio(float new_ratio);

private:
  LpSplitterDirection direction;
  float ratio;
  float minRatio;
  float maxRatio;
};

template <typename T>
bool drawRangeFilter(const char *widget_id, const char *label, LpRangeFilterController<T> &ctrl, const char *format_min = nullptr,
  const char *format_max = nullptr, float speed = 1.0f, ImGuiSliderFlags flags = 0)
{
  if (!ctrl.isBound())
    return false;
  bool changed = false;
  ImGui::PushID(widget_id);
  if (ImGui::Button("Clear"))
  {
    ctrl.resetToDefaults();
    changed = true;
  }
  ImGui::Separator();
  const char *fmtMin = format_min ? format_min : (eastl::is_integral<T>::value ? "Min: %d" : "Min: %.3f");
  const char *fmtMax = format_max ? format_max : (eastl::is_integral<T>::value ? "Max: %d" : "Max: %.3f");
  if (auto *f = ctrl.raw())
  {
    T currentMin = f->getMin();
    T currentMax = f->getMax();
    bool localChanged = false;
    if constexpr (eastl::is_integral<T>::value)
    {
      localChanged = ImGui::DragIntRange2(label, reinterpret_cast<int *>(&currentMin), reinterpret_cast<int *>(&currentMax), speed,
        static_cast<int>(f->getAbsoluteMin()), static_cast<int>(f->getAbsoluteMax()), fmtMin, fmtMax, flags);
    }
    else if constexpr (eastl::is_floating_point<T>::value)
    {
      localChanged = ImGui::DragFloatRange2(label, reinterpret_cast<float *>(&currentMin), reinterpret_cast<float *>(&currentMax),
        speed, static_cast<float>(f->getAbsoluteMin()), static_cast<float>(f->getAbsoluteMax()), fmtMin, fmtMax, flags);
    }
    if (localChanged)
    {
      f->setRange(currentMin, currentMax);
      f->setUseMin(currentMin > f->getAbsoluteMin());
      f->setUseMax(currentMax < f->getAbsoluteMax());
      changed = true;
    }
  }
  ImGui::PopID();
  return changed;
}

} // namespace levelprofiler