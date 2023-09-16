//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <imgui/imgui.h>
#include <EASTL/string.h>
#include <dag/dag_vector.h>

class DataBlock;

namespace ImGuiDagor
{
bool InputText(const char *label, eastl::string *str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr,
  void *user_data = nullptr);
bool InputTextMultiline(const char *label, eastl::string *str, const ImVec2 &size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0,
  ImGuiInputTextCallback callback = nullptr, void *user_data = nullptr);
bool InputTextWithHint(const char *label, const char *hint, eastl::string *str, ImGuiInputTextFlags flags = 0,
  ImGuiInputTextCallback callback = nullptr, void *user_data = nullptr);

void Blk(const DataBlock *blk, bool default_open = false, const char *block_name_override = nullptr);
void BlkEdit(const DataBlock *blk, DataBlock *changes, bool default_open = false, const char *block_name_override = nullptr);

void HelpMarker(const char *desc, float text_wrap_pos_in_pixels = 0.0);

bool ComboWithFilter(const char *label, const dag::Vector<eastl::string_view> &data, int &current_idx, eastl::string &input,
  bool enumerate = false, bool return_on_arrows = false, const char *hint = "", ImGuiComboFlags flags = 0);
} // namespace ImGuiDagor
