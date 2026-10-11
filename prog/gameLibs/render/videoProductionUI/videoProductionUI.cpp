// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/videoProductionUI.h"
#include <EASTL/unique_ptr.h>
#include <generic/dag_enumerate.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>
#include <stdlib.h>

// A typed value applies once, on enter, and not on every keystroke that parses.
static constexpr ImGuiInputTextFlags VALUE_INPUT_FLAGS = ImGuiInputTextFlags_EnterReturnsTrue;

dag::Vector<eastl::unique_ptr<VPValue>> values;

struct VPPresetValue
{
  eastl::string name;
  double value;
};

struct VPPreset
{
  int id;
  eastl::string name;
  dag::Vector<VPPresetValue> values;
};

static const char *presetsFileName = "VideoProductionPresets.blk";

static dag::Vector<VPPreset> presets;
static int nextPresetId = 1;
static int selectedPresetId = -1;
static int editedPresetId = -1;
static bool focusEditedPreset = false;
static bool presetsLoaded = false;

static eastl::string make_preset_name(int id)
{
  eastl::string name;
  name.sprintf("Preset %d", id);
  return name;
}

static int find_preset_index(int id)
{
  for (auto [i, preset] : enumerate(presets))
    if (preset.id == id)
      return (int)i;
  return -1;
}

static const char *get_slider_format(double max_value)
{
  if (max_value <= 1)
    return "%.4f";
  return max_value <= 1000 ? "%.2f" : "%.1f";
}

static void save_presets()
{
  DataBlock blk;
  for (const VPPreset &preset : presets)
  {
    DataBlock *presetBlk = blk.addNewBlock("preset");
    presetBlk->setInt("id", preset.id);
    presetBlk->setStr("name", preset.name.c_str());

    for (const VPPresetValue &value : preset.values)
    {
      DataBlock *valueBlk = presetBlk->addNewBlock("value");
      valueBlk->setStr("name", value.name.c_str());
      valueBlk->setReal("value", value.value);
    }
  }

  if (!blk.saveToTextFile(presetsFileName))
    logerr("video production: can not save %s", presetsFileName);
}

static void load_presets()
{
  presets.clear();
  nextPresetId = 1;
  selectedPresetId = -1;
  editedPresetId = -1;
  focusEditedPreset = false;

  DataBlock blk;
  if (!dd_file_exist(presetsFileName) || !blk.load(presetsFileName))
    return;

  dblk::iterate_child_blocks_by_name(blk, "preset", [](const DataBlock &presetBlk) {
    VPPreset &preset = presets.push_back();
    preset.id = presetBlk.getInt("id", nextPresetId);
    preset.name = presetBlk.getStr("name", "");
    if (preset.name.empty())
      preset.name = make_preset_name(preset.id);
    if (preset.id >= nextPresetId)
      nextPresetId = preset.id + 1;

    dblk::iterate_child_blocks_by_name(presetBlk, "value", [&preset](const DataBlock &valueBlk) {
      VPPresetValue &value = preset.values.push_back();
      value.name = valueBlk.getStr("name", "");
      value.value = double(valueBlk.getReal("value", 0));
    });
  });
}

static void capture_preset(VPPreset &preset)
{
  preset.values.clear();
  for (const eastl::unique_ptr<VPValue> &entry : values)
    if (entry->selected)
    {
      VPPresetValue &value = preset.values.push_back();
      value.name = entry->name;
      value.value = entry->getValue();
    }
}

static void select_all_values(bool selected)
{
  for (const eastl::unique_ptr<VPValue> &entry : values)
    if (entry->widget != VPValue::Widget::Separator)
      entry->selected = selected;
}

static const VPPresetValue *find_preset_value(const VPPreset &preset, const eastl::string &name)
{
  for (const VPPresetValue &value : preset.values)
    if (value.name == name)
      return &value;
  return nullptr;
}

static void select_preset_values(const VPPreset &preset)
{
  for (const eastl::unique_ptr<VPValue> &entry : values)
    entry->selected = find_preset_value(preset, entry->name) != nullptr;
}

static void apply_preset(const VPPreset &preset)
{
  for (const eastl::unique_ptr<VPValue> &entry : values)
  {
    if (!entry->selected)
      continue;
    if (const VPPresetValue *value = find_preset_value(preset, entry->name))
      entry->setValue(value->value);
  }
}

void add_video_production_value(VPValue *value) { values.push_back().reset(value); }

static void imguiWindow()
{
  if (!presetsLoaded)
  {
    load_presets();
    presetsLoaded = true;
  }

  if (ImGui::Button("Select all"))
    select_all_values(true);
  ImGui::SameLine();
  if (ImGui::Button("Deselect all"))
    select_all_values(false);

  ImGui::Separator();

  char id[8];
  for (auto [i, entry] : enumerate(values))
  {
    if (entry->widget == VPValue::Widget::Separator)
    {
      ImGui::Separator();
      continue;
    }

    snprintf(id, sizeof(id), "##%d", (int)i);
    ImGui::Checkbox(id, &entry->selected);

    ImGui::SameLine();

    double value = entry->getValue();
    const char *name = entry->name.c_str();
    switch (entry->widget)
    {
      case VPValue::Widget::Separator: break; // drawn before the switch
      case VPValue::Widget::Checkbox:
      {
        bool state = value != 0;
        if (ImGui::Checkbox(name, &state))
          entry->setValue(state ? 1 : 0);
        break;
      }
      case VPValue::Widget::FloatValue:
      {
        char state[32];
        snprintf(state, sizeof(state), "%g", value);
        if (ImGui::InputText(name, state, sizeof(state), VALUE_INPUT_FLAGS | ImGuiInputTextFlags_CharsScientific))
          entry->setValue(atof(state));
        break;
      }
      case VPValue::Widget::FloatSlider:
      case VPValue::Widget::FloatSliderLog:
      {
        ImGuiSliderFlags flags = ImGuiSliderFlags_NoRoundToFormat;
        if (entry->widget == VPValue::Widget::FloatSliderLog)
          flags |= ImGuiSliderFlags_Logarithmic;

        float state = clamp(value, entry->minValue, entry->maxValue);
        if (ImGui::SliderFloat(name, &state, entry->minValue, entry->maxValue, get_slider_format(entry->maxValue), flags))
          entry->setValue(double(state));
        break;
      }
      case VPValue::Widget::IntValue:
      {
        char state[32];
        snprintf(state, sizeof(state), "%d", (int)value);
        if (ImGui::InputText(name, state, sizeof(state), VALUE_INPUT_FLAGS | ImGuiInputTextFlags_CharsDecimal))
          entry->setValue(atoi(state));
        break;
      }
      case VPValue::Widget::Combo:
      {
        const int itemCount = (int)entry->items.size();
        const int state = (int)clamp(value, 0.0, (double)(itemCount - 1));
        if (itemCount > 0 && ImGui::BeginCombo(name, entry->items[state].c_str()))
        {
          for (auto [itemIndex, item] : enumerate(entry->items))
            if (ImGui::Selectable(item.c_str(), (int)itemIndex == state))
              entry->setValue((double)itemIndex);
          ImGui::EndCombo();
        }
        break;
      }
      case VPValue::Widget::IntSlider:
      case VPValue::Widget::IntSliderLog:
      {
        ImGuiSliderFlags flags = entry->widget == VPValue::Widget::IntSliderLog ? ImGuiSliderFlags_Logarithmic : 0;

        int state = (int)clamp(value, entry->minValue, entry->maxValue);
        if (ImGui::SliderInt(name, &state, (int)entry->minValue, (int)entry->maxValue, "%d", flags))
          entry->setValue(state);
        break;
      }
    }

    if (!entry->tooltip.empty() && ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", entry->tooltip.c_str());
  }

  ImGui::Separator();
  ImGui::Separator();

  if (ImGui::Button("New preset"))
  {
    VPPreset &preset = presets.push_back();
    preset.id = nextPresetId++;
    preset.name = make_preset_name(preset.id);
    selectedPresetId = preset.id;
    editedPresetId = preset.id;
    focusEditedPreset = true;
    save_presets();
  }
  ImGui::SameLine();

  ImGui::BeginDisabled(selectedPresetId < 0);

  if (ImGui::Button("Rename preset"))
  {
    editedPresetId = selectedPresetId;
    focusEditedPreset = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete preset"))
  {
    int index = find_preset_index(selectedPresetId);
    presets.erase(presets.begin() + index);
    editedPresetId = -1;
    focusEditedPreset = false;
    if (index >= (int)presets.size())
      index = (int)presets.size() - 1;
    selectedPresetId = index >= 0 ? presets[index].id : -1;
    save_presets();
  }
  ImGui::SameLine();
  if (ImGui::Button("Save preset"))
  {
    capture_preset(presets[find_preset_index(selectedPresetId)]);
    save_presets();
  }
  ImGui::SameLine();
  if (ImGui::Button("Load preset"))
    apply_preset(presets[find_preset_index(selectedPresetId)]);

  ImGui::EndDisabled();

  if (ImGui::BeginListBox("##presets", ImVec2(-FLT_MIN, 0)))
  {
    for (VPPreset &preset : presets)
    {
      ImGui::PushID(preset.id);
      if (preset.id == editedPresetId)
      {
        if (focusEditedPreset)
        {
          ImGui::SetKeyboardFocusHere();
          focusEditedPreset = false;
        }

        ImGuiDagor::InputText("##name", &preset.name, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        if (ImGui::IsItemDeactivated())
        {
          if (preset.name.empty())
            preset.name = make_preset_name(preset.id);
          editedPresetId = -1;
          save_presets();
        }
      }
      else if (ImGui::Selectable(preset.name.c_str(), preset.id == selectedPresetId))
      {
        selectedPresetId = preset.id;
        select_preset_values(preset);
      }
      ImGui::PopID();
    }
    ImGui::EndListBox();
  }
}

REGISTER_IMGUI_WINDOW("Render", "Video production", imguiWindow);
