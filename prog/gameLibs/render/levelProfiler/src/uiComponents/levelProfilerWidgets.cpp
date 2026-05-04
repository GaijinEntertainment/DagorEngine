// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "levelProfilerInterface.h"
#include "levelProfilerTableBase.h"
#include "levelProfilerWidgets.h"

namespace levelprofiler
{

// === Base Widgets ===

// --- MultiSelect Base Component ---
LpMultiSelectIO LpMultiSelect::multiSelectIoStatic;

LpMultiSelectIO *LpMultiSelect::begin_multi_select(LpMultiSelectFlags flags, int dirty_selection_index, int item_count)
{
  ImGuiIO &imguiIo = ImGui::GetIO();
  multiSelectIoStatic.flags = flags;
  multiSelectIoStatic.dirtySelection = dirty_selection_index;
  multiSelectIoStatic.itemCount = item_count;
  multiSelectIoStatic.isEscapePressed = ImGui::IsKeyPressed(ImGuiKey_Escape);
  const bool isCtrlPressed = imguiIo.KeyCtrl;
  multiSelectIoStatic.isSelectAllRequested = isCtrlPressed && ImGui::IsKeyPressed(ImGuiKey_A);
  multiSelectIoStatic.isClearAllRequested = isCtrlPressed && ImGui::IsKeyPressed(ImGuiKey_D);
  multiSelectIoStatic.isInvertAllRequested = isCtrlPressed && ImGui::IsKeyPressed(ImGuiKey_I);
  return &multiSelectIoStatic;
}


LpMultiSelectIO *LpMultiSelect::end_multi_select()
{
  if (multiSelectIoStatic.isEscapePressed && (multiSelectIoStatic.flags & LpMultiSelectFlags::CLEAR_ON_ESCAPE))
    multiSelectIoStatic.isClearAllRequested = true;

  return &multiSelectIoStatic;
}


void LpSelectionExternalStorage::applyRequests(LpMultiSelectIO *multi_select_io)
{
  if (!multi_select_io || !adapter_set_item_selected)
    return;
  if (multi_select_io->isSelectAllRequested)
    for (int i = 0; i < multi_select_io->itemCount; i++)
      adapter_set_item_selected(this, i, true);

  else if (multi_select_io->isClearAllRequested)
    for (int i = 0; i < multi_select_io->itemCount; i++)
      adapter_set_item_selected(this, i, false);
}

// --- Search Box Base Component ---
LpSearchBox::LpSearchBox(const char *identifier, const char *hint_text, SearchTextChangedCallback search_callback) :
  id(identifier), hint(hint_text), callback(search_callback)
{
  buffer.clear();
}


void LpSearchBox::draw()
{
  ImGui::PushID(id);

  constexpr size_t TEMP_BUFFER_SIZE = 256;
  char tempBuffer[TEMP_BUFFER_SIZE];
  strncpy(tempBuffer, buffer.c_str(), TEMP_BUFFER_SIZE - 1);
  tempBuffer[TEMP_BUFFER_SIZE - 1] = '\0';

  if (bool hasTextChanged = ImGui::InputTextWithHint("##search", hint, tempBuffer, TEMP_BUFFER_SIZE))
  {
    buffer = tempBuffer;
    callback(tempBuffer);
  }

  if (!buffer.empty())
  {
    ImGui::SameLine();
    if (ImGui::SmallButton("x"))
    {
      buffer.clear();
      callback("");
    }
  }
  ImGui::PopID();
}


const char *LpSearchBox::getText() const { return buffer.c_str(); }


void LpSearchBox::setText(const char *text)
{
  buffer = text ? text : "";
  callback(text ? text : "");
}


void LpSearchBox::clear()
{
  buffer.clear();
  callback("");
}

// --- Search Widget ---
LpSearchWidget::LpSearchWidget(LpProfilerTable * /*profiler_table*/, ColumnIndex /*column_index*/, const char *identifier,
  const char *hint_text, SearchTextChangedCallback on_search_callback, SearchEnterCallback apply_filters_callback)
{
  searchBox = eastl::make_unique<LpSearchBox>(identifier, hint_text, [on_search_callback, apply_filters_callback](const char *text) {
    on_search_callback(text);
    apply_filters_callback();
  });
}


LpSearchWidget::~LpSearchWidget() {}


const char *LpSearchWidget::getText() const { return searchBox ? searchBox->getText() : ""; }

void LpSearchWidget::draw()
{
  if (searchBox)
    searchBox->draw();
}


void LpSearchWidget::clear()
{
  if (searchBox)
    searchBox->clear();
}

// === Selection Widgets ===

// === Range Widgets ===

// === Splitter Widgets ===

namespace
{
constexpr float SPLITTER_THICKNESS = 4.0f;
constexpr float SPLITTER_LINE_OFFSET = 2.0f;
constexpr float SPLITTER_LINE_THICKNESS = 1.0f;
} // namespace

LpSplitter::LpSplitter(LpSplitterDirection dir, float initial_ratio, float min_ratio, float max_ratio) :
  direction(dir), ratio(initial_ratio), minRatio(min_ratio), maxRatio(max_ratio)
{}

bool LpSplitter::draw(const ImVec2 &available_area)
{
  bool ratioChanged = false;

  const bool isHorizontal = (direction == LpSplitterDirection::HORIZONTAL);
  const char *buttonId = isHorizontal ? "##HSplitter" : "##VSplitter";
  const ImVec2 buttonSize = isHorizontal ? ImVec2(SPLITTER_THICKNESS, eastl::max(available_area.y, 1.0f))
                                         : ImVec2(eastl::max(available_area.x, 1.0f), SPLITTER_THICKNESS);
  const ImGuiMouseCursor cursor = isHorizontal ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS;

  ImGui::InvisibleButton(buttonId, buttonSize);

  if (ImGui::IsItemHovered())
    ImGui::SetMouseCursor(cursor);

  if (ImGui::IsItemActive())
  {
    const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
    const float delta = isHorizontal ? mouseDelta.x / available_area.x : mouseDelta.y / available_area.y;

    ratio += delta;
    ratio = eastl::clamp(ratio, minRatio, maxRatio);
    ratioChanged = true;
  }

  const ImVec2 lineStart = ImGui::GetItemRectMin();
  const ImVec2 lineEnd = ImGui::GetItemRectMax();
  const ImU32 lineColor = ImGui::IsItemHovered() || ImGui::IsItemActive() ? ImGui::GetColorU32(ImGuiCol_SeparatorHovered)
                                                                          : ImGui::GetColorU32(ImGuiCol_Separator);

  const ImVec2 p1 = isHorizontal ? ImVec2(lineStart.x + SPLITTER_LINE_OFFSET, lineStart.y) : lineStart;
  const ImVec2 p2 = isHorizontal ? ImVec2(lineStart.x + SPLITTER_LINE_OFFSET, lineEnd.y) : lineEnd;

  ImGui::GetWindowDrawList()->AddLine(p1, p2, lineColor, SPLITTER_LINE_THICKNESS);

  return ratioChanged;
}

void LpSplitter::setRatio(float new_ratio) { ratio = eastl::clamp(new_ratio, minRatio, maxRatio); }

} // namespace levelprofiler