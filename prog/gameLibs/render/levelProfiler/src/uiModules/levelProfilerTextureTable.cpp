// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdarg.h>
#include "levelProfilerUI.h"
#include "levelProfilerWidgets.h"
#include "riModule.h"
#include "levelProfilerTextureTable.h"
#include "levelProfilerValueColumn.h"

namespace levelprofiler
{
namespace UIConstants
{
static constexpr float COL_TEX_USAGE_WIDTH = 180.0f;
static constexpr float COL_SIZE_PAIR_WIDTH = 110.0f;
static constexpr float COL_MEM_PAIR_WIDTH = 150.0f;
static constexpr float COL_TEX_QUALITY_WIDTH = 90.0f;

// Filter widget speeds
static constexpr float MEM_SIZE_FILTER_SPEED = 0.1f;
static constexpr float MIPS_FILTER_SPEED = 0.25f;
static constexpr float SHARE_FILTER_SPEED = 0.5f;
static constexpr float TEX_USAGE_FILTER_SPEED = 1.0f;

// Quality chips: the quality of the graphics settings, the default
static constexpr LpChipStyle SETTINGS_QUALITY_CHIP{.background = ImVec4(0.5f, 0.5f, 0.5f, 0.2f), .textAlpha = 0.8f};
// Quality chips: a list of the level lowers the texture
static constexpr LpChipStyle FORCE_TEX_MQ_CHIP{.background = ImVec4(0.95f, 0.7f, 0.1f, 0.35f)};
static constexpr LpChipStyle FORCE_TEX_LQ_CHIP{.background = ImVec4(0.9f, 0.2f, 0.15f, 0.4f)};

static constexpr LpChipStyle NOT_STREAMED_CHIP{.textAlpha = LP_DIM_TEXT_ALPHA};

// Usage chips
static constexpr LpChipStyle UNIQUE_CHIP{.background = ImVec4(1.0f, 0.4f, 0.0f, 0.25f)};
static constexpr LpChipStyle SHARED_CHIP{.background = ImVec4(0.2f, 0.6f, 1.0f, 0.25f)};
static constexpr LpChipStyle NON_RI_CHIP{.textAlpha = LP_DIM_TEXT_ALPHA};
} // namespace UIConstants

static const ProfilerString &texture_name(const TextureTableItem &item) { return item.name; }

// --- LpTextureNameColumn implementation ---

LpTextureNameColumn::LpTextureNameColumn() :
  LpTextureTableColumn("Name", LP_NAME_COLUMN_WIDTH,
    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_NoHide)
{}

ProfilerString LpTextureNameColumn::getRowCellText(const TextureTableItem &item) const { return item.name; }

int LpTextureNameColumn::compareRows(const TextureTableItem &item1, const TextureTableItem &item2) const
{
  return item1.name.compare(item2.name);
}

void LpTextureNameColumn::drawRowCell(const TextureTableItem &item) const
{
  ImGui::Selectable(item.name.c_str(), item.isSelected, ImGuiSelectableFlags_SpanAllColumns);
}

// --- LpTextureUsageColumn implementation ---

static ProfilerString build_usage_list(const TextureTableItem &item, const RIModule &ri_module)
{
  if (!item.assetNames)
    return "Non-RI";

  ProfilerString result;
  result.reserve(item.assetNames->size() * LP_LIST_ENTRY_RESERVE_SIZE);
  for (const ProfilerString &assetName : *item.assetNames)
  {
    if (!result.empty())
      result += ", ";
    lp_append_name_count(result, assetName.c_str(), ri_module.getRiInstanceCount(assetName));
  }
  return result;
}

LpTextureUsageColumn::LpTextureUsageColumn(const RIModule &ri_module) :
  LpLongTextColumn("Usage", UIConstants::COL_TEX_USAGE_WIDTH, ImGuiTableColumnFlags_WidthFixed), riModule(ri_module)
{}

ProfilerString LpTextureUsageColumn::getRowCellText(const TextureTableItem &item) const { return item.usageLabel; }

ProfilerString LpTextureUsageColumn::getRowFullText(const TextureTableItem &item) const { return build_usage_list(item, riModule); }

void LpTextureUsageColumn::drawRowTooltip(const TextureTableItem &item) const
{
  if (item.assetNames)
    LpLongTextColumn::drawRowTooltip(item);
}

int LpTextureUsageColumn::compareRows(const TextureTableItem &item1, const TextureTableItem &item2) const
{
  return lp_compare3(item1.usageUniqueCount, item2.usageUniqueCount);
}

void LpTextureUsageColumn::drawRowCell(const TextureTableItem &item) const
{
  const int count = item.usageUniqueCount;
  lp_draw_cell_chip(item.usageLabel.c_str(), count == 1  ? UIConstants::UNIQUE_CHIP
                                             : count > 1 ? UIConstants::SHARED_CHIP
                                                         : UIConstants::NON_RI_CHIP);
}

// --- LpTextureUsageFilter ---

namespace
{
// The Usage column: the Non-RI / Unique / Shared kinds, the range of the shared usage count, and, for the textures of RI assets,
// the asset name search and masks and the range of the asset instance counts.
class LpTextureUsageFilter final : public ILpColumnFilter
{
public:
  explicit LpTextureUsageFilter(const RIModule &ri_module) : riModule(ri_module) {}

  bool isActive() const override
  {
    return !showNonRi || !showUnique || !showShared || sharedUsageRange.isActive() || assetNameFilter.isActive() ||
           instanceCountRange.isActive();
  }

  void reset() override
  {
    resetKindsAndRanges();
    assetNameFilter.reset();
  }

  bool pass(const void *item_ptr) const override
  {
    const TextureTableItem &item = *static_cast<const TextureTableItem *>(item_ptr);
    const int unique = item.usageUniqueCount;
    const bool isShared = unique >= SHARED_TEXTURE_USAGE_MIN;
    if ((unique == 0 && !showNonRi) || (unique == 1 && !showUnique) || (isShared && !showShared))
      return false;
    if (isShared && !sharedUsageRange.pass(unique))
      return false;

    // At least one asset that uses the texture must pass the name filter.
    if (assetNameFilter.isActive())
    {
      if (!item.assetNames || eastl::none_of(item.assetNames->begin(), item.assetNames->end(),
                                [this](const ProfilerString &asset_name) { return assetNameFilter.pass(asset_name); }))
        return false;
    }

    // All assets with a known instance count must be in the range.
    if (instanceCountRange.isActive())
    {
      if (!item.assetNames)
        return false;
      for (const auto &assetName : *item.assetNames)
        if (!instanceCountRange.pass(riModule.getRiInstanceCount(assetName)))
          return false;
    }
    return true;
  }

  bool drawPopup() override
  {
    // The riGen walk raises the counts with no item rebuild, so the bounds follow the RI module on each popup frame.
    sharedUsageRange.rebind(SHARED_TEXTURE_USAGE_MIN, riModule.getMaxUniqueTextureUsageCount());
    instanceCountRange.rebind(0, riModule.getMaxAssetInstanceCount());

    bool changed = false;
    if (ImGui::Button("Clear"))
    {
      resetKindsAndRanges();
      changed = true;
    }

    ImGui::Separator();
    changed |= ImGui::Checkbox("Non-RI", &showNonRi);
    changed |= ImGui::Checkbox("Unique", &showUnique);
    changed |= ImGui::Checkbox("Shared", &showShared);

    ImGui::Separator();
    ImGui::Text("Shared range:");
    ImGui::BeginDisabled(!showShared);
    LpRangeFilterController<int> usageController(&sharedUsageRange);
    changed |= drawRangeFilter<int>("UsageRange", "", usageController, "Min: %d", "Max: %d", UIConstants::TEX_USAGE_FILTER_SPEED);
    ImGui::EndDisabled();

    if (showUnique || showShared)
    {
      ImGui::Separator();
      ImGui::Text("RI name filter:");
      changed |= assetNameFilter.drawInline();

      ImGui::Separator();
      ImGui::Text("Asset instance count:");
      LpRangeFilterController<int> instanceController(&instanceCountRange);
      changed |=
        drawRangeFilter<int>("InstanceRange", "", instanceController, "Min: %d", "Max: %d", UIConstants::TEX_USAGE_FILTER_SPEED);
    }
    return changed;
  }

  const char *getSearchHint() const override { return "Search..."; }
  void setSearchText(const char *text) override { assetNameFilter.setSearchText(text); }

private:
  void resetKindsAndRanges()
  {
    showNonRi = showUnique = showShared = true;
    sharedUsageRange.reset();
    instanceCountRange.reset();
  }

  const RIModule &riModule;
  bool showNonRi = true, showUnique = true, showShared = true;
  LpRangeFilter<int> sharedUsageRange;
  LpIncludeExcludeFilter assetNameFilter;
  LpRangeFilter<int> instanceCountRange;
};
} // namespace

// --- Quality column ---

namespace
{
const char *const TEX_QUALITY_NAMES[] = {"High", "Medium", "Low", "UltraLow"};

const char *tex_quality_name(int quality)
{
  return quality >= 0 && quality < static_cast<int>(eastl::size(TEX_QUALITY_NAMES)) ? TEX_QUALITY_NAMES[quality] : "?";
}

const char *explicit_list_name(int explicit_list) { return explicit_list == ddsx::EXPLICIT_TEX_LIST_LQ ? "forceTexLQ" : "forceTexMQ"; }
// The source of the quality when no list of the level sets it: the graphics/texquality setting (dgs_tex_quality).
constexpr const char *SETTINGS_QUALITY_NAME = "Graphics settings";
constexpr const char *NOT_STREAMED_TEXT = "not streamed";

const char *max_tql_name(TexQL tql)
{
  switch (tql)
  {
    case TQL_thumb: return "thumbnail";
    case TQL_base: return "base";
    case TQL_high: return "HQ";
    case TQL_uhq: return "UHQ";
    default: return "stub";
  }
}

// In the order of the sort: a list that lowers the quality is the largest.
enum class QualityKind
{
  NOT_STREAMED,
  SETTINGS,
  LIST_WITHOUT_EFFECT,
  LIST_LOWERS
};

QualityKind quality_kind(const TextureData &data)
{
  if (!data.isStreamed)
    return QualityKind::NOT_STREAMED;
  if (data.quality.explicitList == ddsx::EXPLICIT_TEX_LIST_NONE)
    return QualityKind::SETTINGS;
  return data.quality.isLoweredByList() ? QualityKind::LIST_LOWERS : QualityKind::LIST_WITHOUT_EFFECT;
}

// The filter key: the source of the quality.
ProfilerString quality_reason(const TextureData &data)
{
  switch (quality_kind(data))
  {
    case QualityKind::NOT_STREAMED: return NOT_STREAMED_TEXT;
    case QualityKind::SETTINGS: return SETTINGS_QUALITY_NAME;
    case QualityKind::LIST_WITHOUT_EFFECT: return ProfilerString(explicit_list_name(data.quality.explicitList)) + " ignored";
    case QualityKind::LIST_LOWERS: return explicit_list_name(data.quality.explicitList);
  }
  return {};
}

void tooltip_row(const char *name, const char *value_format, ...) IM_FMTARGS(2);
void tooltip_row(const char *name, const char *value_format, ...)
{
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::TextDisabled("%s", name);
  ImGui::TableNextColumn();
  va_list args;
  va_start(args, value_format);
  ImGui::TextV(value_format, args);
  va_end(args);
}

void draw_quality_tooltip(const TextureData &data)
{
  const QualityKind kind = quality_kind(data);
  if (kind == QualityKind::NOT_STREAMED)
  {
    ImGui::TextUnformatted("Not streamed by the texture manager: no quality setting applies");
    return;
  }
  const TextureQuality &quality = data.quality;
  const TextureStateSize &full = data.getState(TextureState::FULL);
  const TextureStateSize &max = data.getState(TextureState::MAX);
  const int cutMips = full.mips - max.mips;
  constexpr int COLUMN_COUNT = 2;
  if (ImGui::BeginTable("##TextureQuality", COLUMN_COUNT, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
  {
    tooltip_row("Quality", "%s", tex_quality_name(quality.selectedQuality));
    tooltip_row(SETTINGS_QUALITY_NAME, "%s (texquality)", tex_quality_name(quality.settings));
    switch (kind)
    {
      case QualityKind::LIST_LOWERS:
        tooltip_row("List", "%s (sets %s)", explicit_list_name(quality.explicitList), tex_quality_name(quality.selectedQuality));
        break;
      case QualityKind::LIST_WITHOUT_EFFECT:
        tooltip_row("List", "%s (no effect at %s)", explicit_list_name(quality.explicitList), tex_quality_name(quality.settings));
        break;
      default: tooltip_row("List", "none"); break;
    }
    tooltip_row("Cut", "%d mip%s: %d -> %d", cutMips, cutMips == 1 ? "" : "s", eastl::max(full.w, full.h), eastl::max(max.w, max.h));
    tooltip_row("Packs", "%s", max_tql_name(quality.maxTql));
    ImGui::EndTable();
  }
  if (kind == QualityKind::SETTINGS)
    return;
  if (kind == QualityKind::LIST_LOWERS && cutMips == 0)
    ImGui::TextUnformatted("The list cuts no mips of this texture: its ddsx has no mips to skip at this quality, or the texture "
                           "was loaded before the level set the list");
  ImGui::TextDisabled("A forceTex list of the daEditorX export cuts the texture in the level data: Full is cut there.");
}

class LpTextureQualityColumn final : public LpTextureTableColumn
{
public:
  LpTextureQualityColumn() :
    LpTextureTableColumn("Quality", UIConstants::COL_TEX_QUALITY_WIDTH,
      ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending)
  {}

  const char *getViewDescription(int /* view */) const override
  {
    return "The quality that the graphics settings and the lists of the level select for a texture.\n"
           "A texture loaded before a list change keeps its quality until it is reloaded.\n"
           "A gray chip (High, Medium...): the texture quality of the graphics settings (texquality).\n"
           "forceTexMQ / forceTexLQ: the list of the level sets a lower quality for the texture.\n"
           "A list with no effect at these graphics settings shows the gray chip; the filter names it \"ignored\".\n"
           "A dim \"not streamed\" chip: the texture manager does not stream the texture (a render target).";
  }

protected:
  void drawRowCell(const TextureTableItem &item) const override
  {
    const TextureData &data = *item.data;
    switch (quality_kind(data))
    {
      case QualityKind::NOT_STREAMED: lp_draw_cell_chip(NOT_STREAMED_TEXT, UIConstants::NOT_STREAMED_CHIP); break;
      case QualityKind::SETTINGS:
      case QualityKind::LIST_WITHOUT_EFFECT:
        lp_draw_cell_chip(tex_quality_name(data.quality.selectedQuality), UIConstants::SETTINGS_QUALITY_CHIP);
        break;
      case QualityKind::LIST_LOWERS:
        lp_draw_cell_chip(explicit_list_name(data.quality.explicitList),
          data.quality.explicitList == ddsx::EXPLICIT_TEX_LIST_LQ ? UIConstants::FORCE_TEX_LQ_CHIP : UIConstants::FORCE_TEX_MQ_CHIP);
        break;
    }
  }
  int compareRows(const TextureTableItem &item1, const TextureTableItem &item2) const override
  {
    return lp_compare3(quality_kind(*item1.data), quality_kind(*item2.data));
  }
  // The shown quality and its source: "Medium (forceTexMQ)".
  ProfilerString getRowCellText(const TextureTableItem &item) const override
  {
    if (!item.data->isStreamed)
      return item.qualityReason;
    ProfilerString text(tex_quality_name(item.data->quality.selectedQuality));
    text.append_sprintf(" (%s)", item.qualityReason.c_str());
    return text;
  }
  void drawRowTooltip(const TextureTableItem &item) const override
  {
    if (ImGui::BeginTooltip())
    {
      draw_quality_tooltip(*item.data);
      ImGui::EndTooltip();
    }
  }
};
} // namespace

// --- Size columns: the texture states as views ---

namespace
{
constexpr double BYTES_TO_MEGABYTES = 1048576.0;
// The texture manager term: a texture that is not loaded is drawn with the stub texture.
constexpr const char *STUB_TEXT = "stub";

struct StateText
{
  const char *name;
  const char *glyph;
  const char *title;
  const char *description;
};
const StateText STATE_TEXTS[*TextureState::COUNT] = {
  {"Full", "F", "Full: the source", "Full: all mips of the source, with its HQ part."},
  {"Max", "M", "Max: at the quality settings",
    "Max: the largest size at the texture quality settings of the game, with the forceTexMQ and forceTexLQ lists of the level."},
  {"Res", "R", "Res: resident", "Res: what is in memory at Collect; stub = nothing loaded yet."},
};

// A pair view shows a state and, right, its base state (Mem: the share of the base). The difference view shows what the state
// lost against the base.
struct StatePair
{
  TextureState state;
  TextureState base;
  const char *glyph;
  const char *title;
  const char *differenceGlyph;
  const char *differenceShortName;
  const char *differenceTitle;
  const char *differenceSumLabel;
};
constexpr StatePair STATE_PAIRS[] = {
  {TextureState::MAX, TextureState::FULL, "MF", "Max and Full", "F-M", "Cut", "Cut by the quality settings: Full - Max",
    "Cut (Full - Max)"},
  {TextureState::RES, TextureState::MAX, "RM", "Res and Max", "M-R", "Not loaded", "Not loaded by the streaming: Max - Res",
    "Not loaded (Max - Res)"},
};
constexpr int STATE_PAIR_COUNT = static_cast<int>(eastl::size(STATE_PAIRS));

constexpr int PAIR_FIRST_VIEW = *TextureState::COUNT;
constexpr int PAIR_VIEW_COUNT = 2;
constexpr int DIFFERENCE_FIRST_VIEW = PAIR_FIRST_VIEW + STATE_PAIR_COUNT * PAIR_VIEW_COUNT;
constexpr int MAX_AND_FULL_PAIR = 0;
constexpr int RES_AND_MAX_PAIR = 1;
static_assert(STATE_PAIRS[MAX_AND_FULL_PAIR].state == TextureState::MAX && STATE_PAIRS[MAX_AND_FULL_PAIR].base == TextureState::FULL,
  "MAX_AND_FULL_PAIR must index the Max and Full pair of STATE_PAIRS");
static_assert(STATE_PAIRS[RES_AND_MAX_PAIR].state == TextureState::RES && STATE_PAIRS[RES_AND_MAX_PAIR].base == TextureState::MAX,
  "RES_AND_MAX_PAIR must index the Res and Max pair of STATE_PAIRS");
// The pair view sorted by the state, the first view of the pair.
constexpr int pair_view(int pair) { return PAIR_FIRST_VIEW + pair * PAIR_VIEW_COUNT; }
constexpr int difference_view(int pair) { return DIFFERENCE_FIRST_VIEW + pair; }

uint32_t state_bit(TextureState state) { return 1u << *state; }

const char *state_name(TextureState state) { return STATE_TEXTS[*state].name; }

float state_memory_mb(const TextureTableItem &item, TextureState state)
{
  return float(item.data->getState(state).memBytes / BYTES_TO_MEGABYTES);
}

// A state above its base (a loaded level above the quality limit) lost nothing, as in the difference views.
float lost_memory_mb(const TextureTableItem &item, TextureState state, TextureState base)
{
  return eastl::max(state_memory_mb(item, base) - state_memory_mb(item, state), 0.0f);
}

void draw_right_aligned_text(const char *text)
{
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + eastl::max(0.0f, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(text).x));
  ImGui::TextUnformatted(text);
}

// shown_states: the state_bit() bits of the states that the cell shows; their rows are highlighted.
void draw_texture_states_tooltip(const TextureTableItem &item, uint32_t shown_states)
{
  const TextureData &data = *item.data;
  constexpr int COLUMN_COUNT = 5;
  if (!ImGui::BeginTable("##TextureStates", COLUMN_COUNT, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
    return;
  for (const char *header : {"State", "W x H", "Mips", "Mem", "Lost"})
    ImGui::TableSetupColumn(header);
  ImGui::TableHeadersRow();
  char text[LP_CELL_TEXT_SIZE];
  const int stateCount = data.isStreamed ? *TextureState::COUNT : 1;
  for (int index = 0; index < stateCount; ++index)
  {
    const TextureState state = static_cast<TextureState>(index);
    const TextureStateSize &size = data.getState(state);
    ImGui::TableNextRow();
    if (data.isStreamed && (shown_states & state_bit(state)))
      ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Header));
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(data.isStreamed ? state_name(state) : "All");
    ImGui::TableNextColumn();
    if (size.mips > 0)
    {
      snprintf(text, sizeof(text), "%d x %d", size.w, size.h);
      draw_right_aligned_text(text);
      ImGui::TableNextColumn();
      snprintf(text, sizeof(text), "%d", size.mips);
      draw_right_aligned_text(text);
    }
    else
    {
      ImGui::TextDisabled(STUB_TEXT);
      ImGui::TableNextColumn();
    }
    ImGui::TableNextColumn();
    if (size.mips > 0)
    {
      lp_format_memory_mb(text, sizeof(text), state_memory_mb(item, state));
      draw_right_aligned_text(text);
    }
    else
      ImGui::TextDisabled(STUB_TEXT);
    ImGui::TableNextColumn();
    if (index == 0)
      continue;
    const float lostMb = lost_memory_mb(item, state, static_cast<TextureState>(index - 1));
    if (lostMb > 0.0f)
    {
      text[0] = '-';
      lp_format_memory_mb(text + 1, sizeof(text) - 1, lostMb);
      draw_right_aligned_text(text);
    }
    else
      ImGui::TextDisabled("0");
  }
  ImGui::EndTable();
  if (!data.isStreamed)
    ImGui::TextDisabled("Not streamed by the texture manager: all states are the resource as it is now");
  if (data.quality.isLoweredByList())
    ImGui::TextDisabled("The %s list of the level sets %s for this texture", explicit_list_name(data.quality.explicitList),
      tex_quality_name(data.quality.selectedQuality));
  if (data.isStreamed && data.getState(TextureState::RES).mips == 0)
    ImGui::TextDisabled("stub: not in memory at Collect; the game shows a placeholder until streaming loads it.");
}

LpCellTooltip<TextureTableItem> states_tooltip(uint32_t shown_states)
{
  return [shown_states](const TextureTableItem &item) { draw_texture_states_tooltip(item, shown_states); };
}

LpValueGetter<TextureTableItem, int> state_size_getter(int TextureStateSize::*field, TextureState state)
{
  return [field, state](const TextureTableItem &item, int &value) {
    const TextureStateSize &size = item.data->getState(state);
    value = size.*field;
    return size.mips > 0;
  };
}

LpValueGetter<TextureTableItem, float> state_memory_getter(TextureState state)
{
  return [state](const TextureTableItem &item, float &value) {
    value = state_memory_mb(item, state);
    return item.data->getState(state).mips > 0;
  };
}

LpValueGetter<TextureTableItem, float> memory_share_getter(TextureState state, TextureState base)
{
  return [state, base](const TextureTableItem &item, float &value) {
    const uint64_t baseBytes = item.data->getState(base).memBytes;
    if (baseBytes == 0)
      return false;
    value = float(100.0 * item.data->getState(state).memBytes / baseBytes);
    return true;
  };
}

template <typename T>
LpValueGetter<TextureTableItem, T> state_difference_getter(LpValueGetter<TextureTableItem, T> state_getter,
  LpValueGetter<TextureTableItem, T> base_getter)
{
  return [state_getter, base_getter](const TextureTableItem &item, T &value) {
    T stateValue{}, baseValue{};
    if (!base_getter(item, baseValue))
      baseValue = T{};
    if (!state_getter(item, stateValue))
      stateValue = T{};
    value = eastl::max(baseValue - stateValue, T{});
    return true;
  };
}

// The size filter key of a state without texture data.
constexpr int NO_SIZE_KEY = 0;

void size_key_label(const int &key, char *label, size_t size)
{
  if (key == NO_SIZE_KEY)
    snprintf(label, size, "%s", STUB_TEXT);
  else
    MultiSelectLabelFuncs::IntLabelWithOther(key, label, size);
}

eastl::unique_ptr<ILpColumnFilter> size_class_filter(const LpValueColumnDesc<TextureTableItem, int> &desc)
{
  auto keyGetter = [getter = desc.getter](const TextureTableItem &item) {
    int value = 0;
    return getter(item, value) ? TextureModule::getSizeFilterKey(value) : NO_SIZE_KEY;
  };
  // Larger sizes first, so "Other" (-1) comes last.
  return eastl::make_unique<LpCheckColumnFilter<TextureTableItem, int, eastl::greater<int>>>(keyGetter, size_key_label, desc.name);
}

template <typename T>
struct SizeColumnDesc
{
  const char *name;
  float pairWidth;
  LpValueFormat<T> format;
  eastl::function<LpValueGetter<TextureTableItem, T>(TextureState)> getter;
  eastl::function<eastl::unique_ptr<ILpColumnFilter>(const LpValueColumnDesc<TextureTableItem, T> &)> filter;
  // Mem: the right part of a pair is the share of the base state, "(/Full)"; else the value of the base state, "(vs Full)".
  bool pairShowsShare = false;
  // A difference view for each pair: what the state lost, in mips or memory. W and H have none: a difference of two sizes in
  // pixels means nothing, the mips tell it.
  bool hasDifference = false;
};

template <typename T>
LpColumnSetup size_states_column(const SizeColumnDesc<T> &desc)
{
  ProfilerString stateNames[*TextureState::COUNT];
  int stateFilterIndices[*TextureState::COUNT];
  eastl::vector<int> stateViews;
  LpViewColumnBuilder builder;
  for (int index = 0; index < *TextureState::COUNT; ++index)
  {
    const TextureState state = static_cast<TextureState>(index);
    const StateText &stateText = STATE_TEXTS[index];
    stateNames[index].sprintf("%s %s", stateText.name, desc.name);
    const LpCellTooltip<TextureTableItem> tooltip = states_tooltip(state_bit(state));
    LpValueColumnDesc<TextureTableItem, T> valueDesc{
      stateNames[index].c_str(), desc.pairWidth, desc.getter(state), desc.format, tooltip, tooltip, STUB_TEXT};
    stateFilterIndices[index] = builder.addValueFilter(desc.filter(valueDesc));
    stateViews.push_back(builder.addView({stateText.glyph, stateText.name, stateText.title, lp_value_column(valueDesc),
      {stateFilterIndices[index]}, stateText.description}));
    G_ASSERT(stateViews.back() == index);
  }
  builder.setExportViews(stateViews);

  for (int pairIndex = 0; pairIndex < STATE_PAIR_COUNT; ++pairIndex)
  {
    const StatePair &pair = STATE_PAIRS[pairIndex];
    const char *stateName = state_name(pair.state);
    const char *baseName = state_name(pair.base);
    ProfilerString header, shareName, byState, byBase, description;
    header.sprintf(desc.pairShowsShare ? "%s %s (/%s)" : "%s %s (vs %s)", stateName, desc.name, baseName);
    shareName.sprintf("%s/%s %s", stateName, baseName, desc.name);
    byState.sprintf("%s, sorted by %s", pair.title, stateName);
    byBase.sprintf("%s, sorted by %s", pair.title, desc.pairShowsShare ? shareName.c_str() : baseName);
    description.sprintf("%s\n%s", STATE_TEXTS[*pair.state].description, STATE_TEXTS[*pair.base].description);

    const LpValueGetter<TextureTableItem, T> stateGetter = desc.getter(pair.state);
    const LpValueGetter<TextureTableItem, T> baseGetter = desc.getter(pair.base);
    int rightFilterIndex = stateFilterIndices[*pair.base];
    LpValuePartDesc<TextureTableItem, T> right{stateNames[*pair.base].c_str(), baseGetter, desc.format};
    if (desc.pairShowsShare)
    {
      if constexpr (eastl::is_same_v<T, float>)
      {
        right = {shareName.c_str(), memory_share_getter(pair.state, pair.base), LP_PERCENT_FORMAT};
        rightFilterIndex = builder.addValueFilter(lp_range_filter(
          LpValueColumnDesc<TextureTableItem, float>{shareName.c_str(), desc.pairWidth, right.getter, LP_PERCENT_FORMAT},
          UIConstants::SHARE_FILTER_SPEED));
      }
    }
    const LpCellTooltip<TextureTableItem> tooltip = states_tooltip(state_bit(pair.state) | state_bit(pair.base));
    LpCompositeColumnDesc<TextureTableItem, T, T> pairDesc{
      header.c_str(), desc.pairWidth, {stateNames[*pair.state].c_str(), stateGetter, desc.format}, right, tooltip, tooltip, STUB_TEXT};
    auto pairViews =
      lp_composite_views(pairDesc, pair.glyph, byState.c_str(), byBase.c_str(), stateFilterIndices[*pair.state], rightFilterIndex);
    pairViews.first.description = pairViews.second.description = description;
    const eastl::pair<int, int> views = builder.addViews(eastl::move(pairViews));
    G_ASSERT(views.first == pair_view(pairIndex));
    G_UNUSED(views);
  }

  if (desc.hasDifference)
    for (int pairIndex = 0; pairIndex < STATE_PAIR_COUNT; ++pairIndex)
    {
      const StatePair &pair = STATE_PAIRS[pairIndex];
      ProfilerString name, description;
      name.sprintf("%s-%s %s", state_name(pair.base), state_name(pair.state), desc.name);
      description.sprintf("%s\n%s", STATE_TEXTS[*pair.base].description, STATE_TEXTS[*pair.state].description);
      const LpCellTooltip<TextureTableItem> tooltip = states_tooltip(state_bit(pair.state) | state_bit(pair.base));
      LpValueColumnDesc<TextureTableItem, T> differenceDesc{name.c_str(), desc.pairWidth,
        state_difference_getter<T>(desc.getter(pair.state), desc.getter(pair.base)), desc.format, tooltip, tooltip};
      const int filterIndex = builder.addValueFilter(desc.filter(differenceDesc));
      const int view = builder.addView({pair.differenceGlyph, pair.differenceShortName, pair.differenceTitle,
        lp_value_column(differenceDesc), {filterIndex}, description});
      G_ASSERT(view == difference_view(pairIndex));
      G_UNUSED(view);
    }
  return builder.build();
}
} // namespace

// --- LpTextureTable implementation ---

LpTextureTable::LpTextureTable(TextureProfilerUI *profiler_ui_ptr) :
  LpTypedTable("Textures", *TextureColumn::BASE_COUNT, texture_name), profilerUI(profiler_ui_ptr)
{
  addColumn(*TextureColumn::NAME, nameColumnSetup(eastl::make_unique<LpTextureNameColumn>(), "Search..."));
  addColumn(*TextureColumn::FORMAT,
    lp_check_column<ProfilerString>(
      LpValueColumnDesc<TextureTableItem, ProfilerString>{
        "Format", LP_NARROW_COLUMN_WIDTH, lp_member_getter(&TextureTableItem::formatName), LP_TEXT_FORMAT},
      [](const ProfilerString &format_name) { return format_name; }, MultiSelectLabelFuncs::ProfilerStringLabel));
  addColumn(*TextureColumn::QUALITY,
    {eastl::make_unique<LpTextureQualityColumn>(),
      eastl::make_unique<LpCheckColumnFilter<TextureTableItem, ProfilerString>>(
        [](const TextureTableItem &item) { return item.qualityReason; }, MultiSelectLabelFuncs::ProfilerStringLabel)});
  const auto sizeGetter = [](int TextureStateSize::*field) {
    return [field](TextureState state) { return state_size_getter(field, state); };
  };
  addColumn(*TextureColumn::WIDTH, size_states_column(SizeColumnDesc<int>{.name = "W",
                                     .pairWidth = UIConstants::COL_SIZE_PAIR_WIDTH,
                                     .format = LP_INT_FORMAT,
                                     .getter = sizeGetter(&TextureStateSize::w),
                                     .filter = size_class_filter}));
  addColumn(*TextureColumn::HEIGHT, size_states_column(SizeColumnDesc<int>{.name = "H",
                                      .pairWidth = UIConstants::COL_SIZE_PAIR_WIDTH,
                                      .format = LP_INT_FORMAT,
                                      .getter = sizeGetter(&TextureStateSize::h),
                                      .filter = size_class_filter}));
  addColumn(*TextureColumn::MIPS,
    size_states_column(SizeColumnDesc<int>{.name = "Mips",
      .pairWidth = UIConstants::COL_SIZE_PAIR_WIDTH,
      .format = LP_INT_FORMAT,
      .getter = sizeGetter(&TextureStateSize::mips),
      .filter =
        [](const LpValueColumnDesc<TextureTableItem, int> &desc) { return lp_range_filter(desc, UIConstants::MIPS_FILTER_SPEED); },
      .hasDifference = true}));
  addColumn(*TextureColumn::MEM_SIZE, size_states_column(SizeColumnDesc<float>{.name = "Mem",
                                        .pairWidth = UIConstants::COL_MEM_PAIR_WIDTH,
                                        .format = LP_MEMORY_MB_FORMAT,
                                        .getter = state_memory_getter,
                                        .filter =
                                          [](const LpValueColumnDesc<TextureTableItem, float> &desc) {
                                            return lp_range_filter(desc, UIConstants::MEM_SIZE_FILTER_SPEED);
                                          },
                                        .pairShowsShare = true,
                                        .hasDifference = true}));
  addColumn(*TextureColumn::USAGE, {eastl::make_unique<LpTextureUsageColumn>(*profilerUI->getRIModule()),
                                     eastl::make_unique<LpTextureUsageFilter>(*profilerUI->getRIModule())});
  const int sizeGroup = addColumnGroup("size columns", *TextureColumn::WIDTH, *TextureColumn::MEM_SIZE - *TextureColumn::WIDTH + 1);

  const auto addStatePreset = [&](const char *name, const char *tooltip, eastl::vector<LpColumnViewRef> views,
                                eastl::vector<ColumnIndex> extra_columns) {
    eastl::vector<ColumnIndex> columns = {*TextureColumn::FORMAT, *TextureColumn::USAGE};
    columns.insert(columns.end(), extra_columns.begin(), extra_columns.end());
    addPreset({.name = name,
      .tooltip = tooltip,
      .columns = eastl::move(columns),
      .groups = {sizeGroup},
      .views = eastl::move(views),
      .sortColumn = *TextureColumn::MEM_SIZE,
      .sortDirection = ImGuiSortDirection_Descending});
  };
  const auto pairViews = [sizeGroup](int pair) -> eastl::vector<LpColumnViewRef> {
    // The difference view goes only to the size columns that have it (Mips, Mem), over the pair view.
    return {{.group = sizeGroup, .view = pair_view(pair)}, {.group = sizeGroup, .view = difference_view(pair)}};
  };
  addStatePreset("Budget",
    "What takes the VRAM at the quality settings of the player? Which textures are still 4096 at medium quality?",
    {{.group = sizeGroup, .view = *TextureState::MAX}}, {*TextureColumn::QUALITY});
  addStatePreset("Streaming", "What did the streaming not load at this point of the map? Why is a texture blurry?",
    pairViews(RES_AND_MAX_PAIR), {});
  addStatePreset("Source", "What does the asset cost as it is? A texture without mips, not a power of 2, an extra 4096?",
    {{.group = sizeGroup, .view = *TextureState::FULL}}, {});
  addStatePreset("Quality", "How much do the quality settings cut? Does the texture have an HQ part?", pairViews(MAX_AND_FULL_PAIR),
    {*TextureColumn::QUALITY});

  setCopyMenuTopColumns({*TextureColumn::NAME});
  addCopyMenuItem("Usage details",
    [this](const TextureTableItem &item) { return getColumn(*TextureColumn::USAGE)->getFullCellText(&item); });
  setSelectedCallback([this](const TextureTableItem &item) { profilerUI->selectTexture(item.name); });
}

bool LpTextureTable::pollItemsChanged()
{
  const RIModule *riModule = profilerUI->getRIModule();
  // The Usage filter keeps its result of the instance counts in the rows; the Full Row copy and the export read them live.
  if (unsigned counts = riModule->getRiCountsGeneration(); counts != cachedRiCountsGeneration)
  {
    cachedRiCountsGeneration = counts;
    if (isFilterActive(TextureColumn::USAGE))
      markRowsDirty();
  }

  unsigned texGeneration = profilerUI->getTextureModule()->getDataGeneration();
  unsigned riGeneration = riModule->getRiDataGeneration();
  if (texGeneration == cachedTextureDataGeneration && riGeneration == cachedRiDataGeneration)
    return false;
  cachedTextureDataGeneration = texGeneration;
  cachedRiDataGeneration = riGeneration;
  return true;
}

const char *LpTextureTable::getNoRowsText() const { return "No textures match current filters"; }

void LpTextureTable::rebuildItems()
{
  items.clear();
  TextureModule *texModule = profilerUI->getTextureModule();
  RIModule *riModule = profilerUI->getRIModule();

  const auto &allTextures = texModule->getTextures();
  static const eastl::unordered_map<ProfilerString, TextureUsage> EMPTY_USAGE_MAP;
  static const eastl::unordered_map<ProfilerString, eastl::vector<ProfilerString>> EMPTY_ASSETS_MAP;
  const auto &usageMap = riModule ? riModule->getTextureUsage() : EMPTY_USAGE_MAP;
  const auto &texToAssets = riModule ? riModule->getTextureToAssetsMap() : EMPTY_ASSETS_MAP;

  items.reserve(allTextures.size());

  for (const auto &[name, td] : allTextures)
  {
    const char *fmt = TextureModule::getFormatName(td.info.cflg);
    if (!fmt)
      continue;
    int uniqueCount = 0;
    if (riModule)
    {
      auto uit = usageMap.find(name);
      if (uit != usageMap.end())
        uniqueCount = uit->second.unique;
    }
    TextureTableItem item(name, td);
    item.formatName = fmt;
    item.qualityReason = quality_reason(td);
    item.usageUniqueCount = uniqueCount;
    if (uniqueCount == 0)
      item.usageLabel = "Non-RI";
    else if (uniqueCount == 1)
      item.usageLabel = "Unique";
    else
      item.usageLabel = "Shared(" + eastl::to_string(uniqueCount) + ")";
    if (riModule)
    {
      auto ait = texToAssets.find(name);
      if (ait != texToAssets.end() && !ait->second.empty())
        item.assetNames = &ait->second;
    }
    items.push_back(eastl::move(item));
  }
}

LpTextureMemorySums LpTextureTable::sumMemory()
{
  refreshRows();
  LpTextureMemorySums sums;
  const int view = getColumn(*TextureColumn::MEM_SIZE)->getView();
  const StatePair *pair = view >= DIFFERENCE_FIRST_VIEW ? &STATE_PAIRS[view - DIFFERENCE_FIRST_VIEW]
                          : view >= PAIR_FIRST_VIEW     ? &STATE_PAIRS[(view - PAIR_FIRST_VIEW) / PAIR_VIEW_COUNT]
                                                        : nullptr;
  const TextureState state = pair ? pair->state : static_cast<TextureState>(view);
  sums.label.sprintf("%s Mem", state_name(state));
  if (pair)
    sums.differenceLabel = pair->differenceSumLabel;
  const auto add = [&](const TextureTableItem &item, bool visible) {
    const float memoryMb = state_memory_mb(item, state);
    (visible ? sums.memory.visible : sums.memory.total) += memoryMb;
    if (pair)
      (visible ? sums.difference.visible : sums.difference.total) += lost_memory_mb(item, state, pair->base);
  };
  for (int itemIndex : getDisplayOrder())
    add(items[itemIndex], true);
  for (const TextureTableItem &item : items)
    add(item, false);
  return sums;
}

} // namespace levelprofiler
