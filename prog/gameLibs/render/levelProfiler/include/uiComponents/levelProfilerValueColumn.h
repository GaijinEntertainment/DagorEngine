// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdio.h>
#include <EASTL/functional.h>
#include <EASTL/utility.h>
#include <debug/dag_assert.h>
#include "levelProfilerTableBase.h"

namespace levelprofiler
{

// The cell text of a value column is cut at this size, so a value column holds short values only.
inline constexpr int LP_CELL_TEXT_SIZE = 64;

template <typename T>
struct LpValueFormat
{
  // Writes the UI text of one value; the cell and its copy show this text.
  void (*ui)(char *text, int size, const T &value) = nullptr;
  // Writes the exported number without its unit; null exports the UI text.
  void (*exportText)(char *text, int size, const T &value) = nullptr;
  // The unit of the exported text, added to the export header as "<name>, <unit>". A format whose UI text holds a unit
  // gives an exportText without it.
  const char *exportUnit = nullptr;
  // The printf formats of the range filter bound labels.
  const char *rangeMinFormat = nullptr;
  const char *rangeMaxFormat = nullptr;
};

inline void lp_format_int(char *text, int size, const int &value) { snprintf(text, size, "%d", value); }
inline void lp_format_float(char *text, int size, const float &value) { snprintf(text, size, "%.2f", value); }
inline void lp_format_percent(char *text, int size, const float &value) { snprintf(text, size, "%.2f%%", value); }
inline void lp_format_text(char *text, int size, const ProfilerString &value) { snprintf(text, size, "%s", value.c_str()); }

inline void lp_format_memory_mb(char *text, int size, const float &size_mb)
{
  if (size_mb < 1.0f)
    snprintf(text, size, "%.2f KB", size_mb * 1024.0f);
  else
    snprintf(text, size, "%.2f MB", size_mb);
}

inline void lp_format_mb_as_kb(char *text, int size, const float &size_mb) { snprintf(text, size, "%.2f", size_mb * 1024.0f); }

inline constexpr LpValueFormat<int> LP_INT_FORMAT{.ui = lp_format_int, .rangeMinFormat = "Min: %d", .rangeMaxFormat = "Max: %d"};
inline constexpr LpValueFormat<float> LP_FLOAT_FORMAT{
  .ui = lp_format_float, .rangeMinFormat = "Min: %.2f", .rangeMaxFormat = "Max: %.2f"};
inline constexpr LpValueFormat<float> LP_DISTANCE_FORMAT{
  .ui = lp_format_float, .exportUnit = "m", .rangeMinFormat = "Min: %.2f", .rangeMaxFormat = "Max: %.2f"};
inline constexpr LpValueFormat<float> LP_PERCENT_FORMAT{.ui = lp_format_percent,
  .exportText = lp_format_float,
  .exportUnit = "%",
  .rangeMinFormat = "Min: %.2f%%",
  .rangeMaxFormat = "Max: %.2f%%"};
inline constexpr LpValueFormat<ProfilerString> LP_TEXT_FORMAT{.ui = lp_format_text};
inline constexpr LpValueFormat<float> LP_MEMORY_MB_FORMAT{.ui = lp_format_memory_mb,
  .exportText = lp_format_mb_as_kb,
  .exportUnit = "KB",
  .rangeMinFormat = "Min: %.2f MB",
  .rangeMaxFormat = "Max: %.2f MB"};

// Returns false when the row has no value: the cell is then empty (or shows the missing text of the value), the sort puts the
// row last in both directions, and the range filter lets the row pass unless the check box of its missing text is cleared.
template <typename Row, typename T>
using LpValueGetter = eastl::function<bool(const Row &, T &)>;

template <typename Row, typename T>
LpValueGetter<Row, T> lp_member_getter(T Row::*field)
{
  return [field](const Row &row, T &value) {
    value = row.*field;
    return true;
  };
}

// Draws the tooltip content of a cell. The data does not depend on the view of the column, only its styling may, for example
// a highlight of what the cell shows.
template <typename Row>
using LpCellTooltip = eastl::function<void(const Row &)>;

template <typename Row>
void lp_draw_cell_tooltip(const LpCellTooltip<Row> &draw_content, const Row &row)
{
  if (draw_content && ImGui::BeginTooltip())
  {
    draw_content(row);
    ImGui::EndTooltip();
  }
}

// One value of a row with its format.
template <typename Row, typename T>
class LpValuePart
{
public:
  LpValuePart(LpValueGetter<Row, T> getter, const LpValueFormat<T> &value_format, const char *missing_text = nullptr) :
    getValue(eastl::move(getter)), format(value_format), missingText(missing_text ? missing_text : "")
  {}

  bool hasMissingText() const { return !missingText.empty(); }

  bool has(const Row &row) const
  {
    T value{};
    return getValue(row, value);
  }

  // False when the row has no value.
  bool formatUi(const Row &row, char (&text)[LP_CELL_TEXT_SIZE]) const { return formatWith(row, format.ui, text); }
  // The value, or the missing text for a row without the value; only has() tells them apart.
  ProfilerString getUiText(const Row &row) const
  {
    char text[LP_CELL_TEXT_SIZE];
    return formatUi(row, text) ? ProfilerString(text) : missingText;
  }
  // Draws the value, or the missing text as a chip, or nothing.
  void drawUi(const Row &row) const
  {
    char text[LP_CELL_TEXT_SIZE];
    if (formatUi(row, text))
      ImGui::TextUnformatted(text);
    else if (!missingText.empty())
      lp_draw_cell_chip(missingText.c_str());
  }
  ProfilerString getExportText(const Row &row) const
  {
    char text[LP_CELL_TEXT_SIZE];
    return formatWith(row, format.exportText ? format.exportText : format.ui, text) ? ProfilerString(text) : ProfilerString();
  }
  ProfilerString getExportHeader(const char *name) const
  {
    ProfilerString header(name);
    if (format.exportUnit)
      header.append_sprintf(", %s", format.exportUnit);
    return header;
  }

  bool getNumber(const Row &row, double &value) const
  {
    if constexpr (eastl::is_arithmetic_v<T>)
    {
      T rowValue{};
      if (!getValue(row, rowValue))
        return false;
      value = static_cast<double>(rowValue);
      return true;
    }
    else
      return false;
  }

  int compare(const Row &row1, const Row &row2) const
  {
    T value1{}, value2{};
    getValue(row1, value1);
    getValue(row2, value2);
    return lp_compare3(value1, value2);
  }

private:
  using FormatFunc = void (*)(char *text, int size, const T &value);

  bool formatWith(const Row &row, FormatFunc format_func, char (&text)[LP_CELL_TEXT_SIZE]) const
  {
    T value{};
    if (!getValue(row, value))
      return false;
    format_func(text, LP_CELL_TEXT_SIZE, value);
    return true;
  }

  LpValueGetter<Row, T> getValue;
  LpValueFormat<T> format;
  ProfilerString missingText;
};

template <typename Row, typename T>
struct LpValueColumnDesc
{
  const char *name = nullptr;
  float width = 0.0f;
  LpValueGetter<Row, T> getter;
  LpValueFormat<T> format;
  LpCellTooltip<Row> tooltip;      // Optional, for a cell with a value
  LpCellTooltip<Row> emptyTooltip; // Optional, for a cell without a value: why it has none
  // Optional, a short text in a chip for a cell without a value, for example "stub"; the range filter then has a check box
  // with this label for the rows without the value.
  const char *missingText = nullptr;
};

// A column that shows one value of a row.
template <typename Row, typename T>
class LpValueColumn final : public LpTypedTableColumn<Row>
{
public:
  explicit LpValueColumn(const LpValueColumnDesc<Row, T> &desc) :
    LpTypedTableColumn<Row>(desc.name, desc.width, ImGuiTableColumnFlags_WidthFixed),
    value(desc.getter, desc.format, desc.missingText),
    tooltip(desc.tooltip),
    emptyTooltip(desc.emptyTooltip)
  {}

  ProfilerString getExportHeader(int /* part */) const override { return value.getExportHeader(this->getName()); }
  bool getNumber(const void *item_data, double &number) const override
  {
    return value.getNumber(*static_cast<const Row *>(item_data), number);
  }

protected:
  bool rowHasValue(const Row &row) const override { return value.has(row); }

  void drawRowCell(const Row &row) const override { value.drawUi(row); }

  int compareRows(const Row &row1, const Row &row2) const override { return value.compare(row1, row2); }
  ProfilerString getRowCellText(const Row &row) const override { return value.getUiText(row); }
  ProfilerString getRowExportText(const Row &row, int /* part */) const override { return value.getExportText(row); }
  void drawRowTooltip(const Row &row) const override { lp_draw_cell_tooltip(value.has(row) ? tooltip : emptyTooltip, row); }

private:
  LpValuePart<Row, T> value;
  LpCellTooltip<Row> tooltip;
  LpCellTooltip<Row> emptyTooltip;
};

template <typename Row, typename T>
struct LpValuePartDesc
{
  const char *name = nullptr; // The export header, for example "L1/L0 Tris"
  LpValueGetter<Row, T> getter;
  LpValueFormat<T> format;
};

// The part of a composite cell that the sort reads; the cell shows it in the text color and the other part dim.
enum class LpCompositeSort
{
  FIRST,
  SECOND
};

template <typename Row, typename First, typename Second>
struct LpCompositeColumnDesc
{
  const char *name = nullptr; // The header, for example "L1 Tris (/L0)"
  float width = 0.0f;
  LpValuePartDesc<Row, First> first;
  LpValuePartDesc<Row, Second> second;
  LpCellTooltip<Row> tooltip;      // Optional, for a cell with a first value
  LpCellTooltip<Row> emptyTooltip; // Optional, for a cell without a first value
  // Optional, a short text in a chip for a row without the first value, shown when the row has the second value.
  const char *firstMissingText = nullptr;
};

// A column that shows two values of a row in one cell, the second one right-aligned: "11040  92.00%". A cell without a first
// value is empty, unless the first part has a missing text and the row has a second value: "[stub]  4096". The export gives
// each value its own column.
template <typename Row, typename First, typename Second>
class LpCompositeColumn final : public LpTypedTableColumn<Row>
{
public:
  LpCompositeColumn(const LpCompositeColumnDesc<Row, First, Second> &desc, LpCompositeSort sort) :
    LpTypedTableColumn<Row>(desc.name, desc.width, ImGuiTableColumnFlags_WidthFixed),
    firstValue(desc.first.getter, desc.first.format, desc.firstMissingText),
    secondValue(desc.second.getter, desc.second.format),
    firstName(desc.first.name),
    secondName(desc.second.name),
    sortsFirst(sort == LpCompositeSort::FIRST),
    tooltip(desc.tooltip),
    emptyTooltip(desc.emptyTooltip)
  {}

  void setSecondName(const char *name_text) { secondName = name_text; }

  LpSortMarker getViewSortMarker(int /* view */) const override { return sortsFirst ? LpSortMarker::BEFORE : LpSortMarker::AFTER; }
  int getExportPartCount() const override { return PART_COUNT; }
  ProfilerString getExportHeader(int part) const override
  {
    return part == FIRST_PART ? firstValue.getExportHeader(firstName.c_str()) : secondValue.getExportHeader(secondName.c_str());
  }
  // The first value, whichever part the sort reads.
  bool getNumber(const void *item_data, double &number) const override
  {
    return firstValue.getNumber(*static_cast<const Row *>(item_data), number);
  }

protected:
  bool rowHasValue(const Row &row) const override { return sortsFirst ? firstValue.has(row) : secondValue.has(row); }

  void drawRowCell(const Row &row) const override
  {
    char firstText[LP_CELL_TEXT_SIZE];
    const bool hasFirst = firstValue.formatUi(row, firstText);
    if (!hasFirst && !canShowMissingFirst(row))
      return;
    const ImU32 sortedColor = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32 otherColor = ImGui::GetColorU32(ImGuiCol_Text, LP_DIM_TEXT_ALPHA);
    // One group item, so the row context menu opens over the whole cell.
    ImGui::BeginGroup();
    if (hasFirst)
    {
      ImGui::PushStyleColor(ImGuiCol_Text, sortsFirst ? sortedColor : otherColor);
      ImGui::TextUnformatted(firstText);
      ImGui::PopStyleColor();
    }
    else
      firstValue.drawUi(row);
    char secondText[LP_CELL_TEXT_SIZE];
    if (!secondValue.formatUi(row, secondText))
    {
      ImGui::EndGroup();
      return;
    }
    ImGui::SameLine();
    const float secondWidth = ImGui::CalcTextSize(secondText).x;
    const float naturalMaxX = ImGui::GetCursorScreenPos().x + secondWidth;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + eastl::max(0.0f, ImGui::GetContentRegionAvail().x - secondWidth));
    ImGui::PushStyleColor(ImGuiCol_Text, sortsFirst ? otherColor : sortedColor);
    ImGui::TextUnformatted(secondText);
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    lp_set_cell_natural_max_x(naturalMaxX);
  }

  int compareRows(const Row &row1, const Row &row2) const override
  {
    return sortsFirst ? firstValue.compare(row1, row2) : secondValue.compare(row1, row2);
  }

  ProfilerString getRowCellText(const Row &row) const override
  {
    if (!firstValue.has(row) && !canShowMissingFirst(row))
      return {};
    ProfilerString text = firstValue.getUiText(row);
    if (ProfilerString secondText = secondValue.getUiText(row); !secondText.empty())
      text.append_sprintf("  %s", secondText.c_str());
    return text;
  }

  ProfilerString getRowExportText(const Row &row, int part) const override
  {
    return part == FIRST_PART ? firstValue.getExportText(row) : secondValue.getExportText(row);
  }

  void drawRowTooltip(const Row &row) const override { lp_draw_cell_tooltip(firstValue.has(row) ? tooltip : emptyTooltip, row); }

private:
  // For a row without the first value: the cell shows the missing text when the row has the second value.
  bool canShowMissingFirst(const Row &row) const { return firstValue.hasMissingText() && secondValue.has(row); }

  static constexpr int FIRST_PART = 0;
  static constexpr int PART_COUNT = 2;

  LpValuePart<Row, First> firstValue;
  LpValuePart<Row, Second> secondValue;
  ProfilerString firstName;
  ProfilerString secondName;
  bool sortsFirst = true;
  LpCellTooltip<Row> tooltip;
  LpCellTooltip<Row> emptyTooltip;
};

// The filters below read the getter of the column; the range filter label is the name of the column.
template <typename Row, typename T>
eastl::unique_ptr<ILpColumnFilter> lp_range_filter(const LpValueColumnDesc<Row, T> &desc, float speed)
{
  return eastl::make_unique<LpRangeColumnFilter<Row, T>>(desc.getter, desc.name, desc.format.rangeMinFormat,
    desc.format.rangeMaxFormat, speed, desc.missingText);
}

template <typename Row, typename T>
eastl::unique_ptr<LpProfilerTableColumn> lp_value_column(const LpValueColumnDesc<Row, T> &desc)
{
  return eastl::make_unique<LpValueColumn<Row, T>>(desc);
}

template <typename Row, typename T>
LpColumnSetup lp_range_column(const LpValueColumnDesc<Row, T> &desc, float speed)
{
  return {lp_value_column(desc), lp_range_filter(desc, speed)};
}

// The two views of a pair of values, the first one sorts by the first part and the second one by the second part. Both use
// the value filters first_filter_index and second_filter_index (see LpViewColumnBuilder). The glyph is two single-byte (ASCII)
// letters, one for each part, as the base splits it by bytes; the views have no short names, as the sort marker of the glyph names the
// sort.
template <typename Row, typename First, typename Second>
eastl::pair<LpColumnViewDesc, LpColumnViewDesc> lp_composite_views(const LpCompositeColumnDesc<Row, First, Second> &desc,
  const char *glyph, const char *first_title, const char *second_title, int first_filter_index, int second_filter_index)
{
  LpColumnViewDesc firstView{glyph, "", first_title,
    eastl::make_unique<LpCompositeColumn<Row, First, Second>>(desc, LpCompositeSort::FIRST),
    {first_filter_index, second_filter_index}};
  LpColumnViewDesc secondView{glyph, "", second_title,
    eastl::make_unique<LpCompositeColumn<Row, First, Second>>(desc, LpCompositeSort::SECOND),
    {first_filter_index, second_filter_index}};
  return {eastl::move(firstView), eastl::move(secondView)};
}

// to_key maps a value to its key in the filter, for example a texture size to its size class. The getter of a check column
// gives a value for every row.
template <typename Key, typename Compare = eastl::less<Key>, typename Row, typename T, typename ToKey>
LpColumnSetup lp_check_column(const LpValueColumnDesc<Row, T> &desc, ToKey to_key,
  typename LpCheckColumnFilter<Row, Key, Compare>::LabelFunc label_func)
{
  auto keyGetter = [getter = desc.getter, to_key](const Row &row) {
    T value{};
    G_VERIFY(getter(row, value));
    return Key(to_key(value));
  };
  return {eastl::make_unique<LpValueColumn<Row, T>>(desc),
    eastl::make_unique<LpCheckColumnFilter<Row, Key, Compare>>(keyGetter, label_func)};
}

} // namespace levelprofiler
