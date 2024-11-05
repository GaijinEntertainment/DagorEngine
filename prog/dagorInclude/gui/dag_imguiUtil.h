//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <imgui/imgui.h>
#include <EASTL/string.h>
#include <dag/dag_vector.h>
#include <EASTL/optional.h>
#include <EASTL/variant.h>

class DataBlock;
class BaseTexture;
class D3DRESID;
class String;
typedef BaseTexture Texture;
typedef D3DRESID TEXTUREID;

namespace ImGuiDagor
{
// service struct for Combo
// initialized in BeginCombo, modified in custom draw function and read in EndCombo
struct ComboInfo
{
  bool arrowScroll{};
  bool selectionChanged{};
  bool inputTextChanged{};
};

bool InputText(const char *label, eastl::string *str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr,
  void *user_data = nullptr);
bool InputText(const char *label, String *str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr,
  void *user_data = nullptr);
bool InputTextMultiline(const char *label, eastl::string *str, const ImVec2 &size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0,
  ImGuiInputTextCallback callback = nullptr, void *user_data = nullptr);
bool InputTextMultiline(const char *label, String *str, const ImVec2 &size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0,
  ImGuiInputTextCallback callback = nullptr, void *user_data = nullptr);
bool InputTextWithHint(const char *label, const char *hint, eastl::string *str, ImGuiInputTextFlags flags = 0,
  ImGuiInputTextCallback callback = nullptr, void *user_data = nullptr);
bool InputTextWithHint(const char *label, const char *hint, String *str, ImGuiInputTextFlags flags = 0,
  ImGuiInputTextCallback callback = nullptr, void *user_data = nullptr);

void Blk(const DataBlock *blk, bool default_open = false, const char *block_name_override = nullptr);
void BlkEdit(const DataBlock *blk, DataBlock *changes, bool default_open = false, const char *block_name_override = nullptr);

void HelpMarker(const char *desc, float text_wrap_pos_in_pixels = 0.0);

bool ComboWithFilter(const char *label, const dag::Vector<eastl::string_view> &data, int &current_idx, eastl::string &input,
  bool enumerate = false, bool return_on_arrows = false, const char *hint = "", ImGuiComboFlags flags = 0);

// return empty if Combo is not open and ComboInfo otherwise
eastl::optional<ComboInfo> BeginComboWithFilter(const char *label, const dag::Vector<eastl::string_view> &data, int &current_idx,
  eastl::string &input, float width, const char *hint = "", ImGuiComboFlags flags = 0);

// return true if we have new selected string
bool EndComboWithFilter(const char *label, const dag::Vector<eastl::string_view> &data, const int &current_idx, eastl::string &input,
  const ComboInfo &info, bool selectedByMouse = false);

template <typename T>
inline void EnumCombo(const char *name, T first, T last, T &value, const char *(*conv)(T),
  ImGuiComboFlags flags = ImGuiChildFlags_None)
{
  if (ImGui::BeginCombo(name, conv(value), flags))
  {
    for (int i = (int)first; i <= (int)last; ++i)
      if (ImGui::Selectable(conv((T)i), value == (T)i))
        value = (T)i;
    ImGui::EndCombo();
  }
}

void Image(const TEXTUREID &id, Texture *texture, const ImVec2 &uv0 = ImVec2(0, 0), const ImVec2 &uv1 = ImVec2(1, 1));
void Image(const TEXTUREID &id, float aspect, const ImVec2 &uv0 = ImVec2(0, 0), const ImVec2 &uv1 = ImVec2(1, 1));

} // namespace ImGuiDagor