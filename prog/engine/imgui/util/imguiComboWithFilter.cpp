// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/fixed_string.h>
#include <gui/dag_imguiUtil.h>
#include <imgui/imgui_internal.h>

// Most of the code for ImGuiDagor::ComboWithFilter() was based on the following ImGui thread:
// https://github.com/ocornut/imgui/issues/1658


static eastl::string to_lower_case(const eastl::string_view &str)
{
  eastl::string res(str);
  for (auto &ch : res)
    ch = tolower(ch);
  return res;
}


static int string_vector_search(const eastl::string &needle, const dag::Vector<eastl::string_view> &data)
{
  // ImGuiDagor::ComboWithFilter() is deliberately made to not support empty strings as combo options:
  if (needle.empty())
    return -1;

  // Start with calculating first occurences:
  dag::Vector<size_t> occurences;
  occurences.reserve(data.size());
  for (const auto &haystack : data)
    occurences.push_back(to_lower_case(haystack).find(needle));

  // Then search if 'needle' is in the beginning of the 'haystack':
  int commonPrefix = -1;
  for (size_t i = 0; i < occurences.size(); i++)
  {
    if (occurences[i] == 0)
    {
      // Exact match is preferred
      if (needle == data[i])
        return i;
      // Else return first string where 'needle' is in the beginning
      if (commonPrefix == -1)
        commonPrefix = i;
    }
  }

  if (commonPrefix > -1)
    return commonPrefix;

  // And only then search for other occurences.
  // This is done in order to avoid undesirable search results due to one option name being entirely contained
  // within another option name, e.g. 'post_fx_node' being a part of 'prepare_post_fx_node'.
  for (size_t i = 0; i < occurences.size(); i++)
    if (occurences[i] != eastl::string::npos)
      return i;

  return -1;
}


static eastl::string safe_string_getter(const dag::Vector<eastl::string_view> &data, int n)
{
  if (n >= 0 && n < (int)data.size())
    return eastl::string(data[n]);
  return "";
}


static float get_field_widget_width(const dag::Vector<eastl::string_view> &data)
{
  auto it = eastl::max_element(data.begin(), data.end(), [](const auto &a, const auto &b) {
    return ImGui::CalcTextSize(eastl::string(a).c_str()).x < ImGui::CalcTextSize(eastl::string(b).c_str()).x;
  });
  return ImGui::CalcTextSize(eastl::string(*it).c_str()).x;
}


static float get_extra_width_for_enumeration(size_t itemCount)
{
  float maxWidth = 0.f;
  for (size_t i = 0; i < itemCount; ++i)
  {
    const eastl::fixed_string<char, 16> enumerationString(eastl::fixed_string<char, 16>::CtorSprintf{}, "%u.", i + 1);
    const float width = ImGui::CalcTextSize(enumerationString.c_str()).x;
    if (maxWidth < width)
      maxWidth = width;
  }
  return maxWidth;
}


static void arrow_scroll(const dag::Vector<eastl::string_view> &data, int &current_idx, eastl::string &input,
  ImGuiDagor::ComboInfo &info)
{
  if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
  {
    if (current_idx > 0)
    {
      current_idx -= 1;
      info.arrowScroll = true;
      ImGui::SetWindowFocus();
    }
  }
  if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
  {
    if (current_idx >= -1 && current_idx + 1 < data.size())
    {
      current_idx += 1;
      info.arrowScroll = true;
      ImGui::SetWindowFocus();
    }
  }

  // select the first match
  if (ImGui::IsKeyPressed(ImGuiKey_Enter))
  {
    info.arrowScroll = true;
    current_idx = string_vector_search(input, data);
    if (current_idx < 0)
      input.clear();
    ImGui::CloseCurrentPopup();
  }

  if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
  {
    current_idx = string_vector_search(input, data);
    info.selectionChanged = true;
  }
}

eastl::optional<ImGuiDagor::ComboInfo> ImGuiDagor::BeginComboWithFilter(const char *label, const dag::Vector<eastl::string_view> &data,
  int &current_idx, eastl::string &input, float width, const char *hint, ImGuiComboFlags flags)
{
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
    return {};

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;

  const ImGuiID popupId = window->GetID(label);
  bool popupIsAlreadyOpened = ImGui::IsPopupOpen(popupId, 0);
  eastl::string sActiveIdxValue = safe_string_getter(data, current_idx);
  bool popupNeedsToBeOpened = (input[0] != 0) && (!sActiveIdxValue.empty() && input != sActiveIdxValue);
  bool popupJustOpened = false;

  IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) !=
            (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

  const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : ImGui::GetFrameHeight();
  const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
  const float expected_w = width + arrow_size + style.ScrollbarSize + 3. * style.WindowPadding.x;
  const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : expected_w;
  const ImRect frame_bb(window->DC.CursorPos,
    ImVec2(window->DC.CursorPos.x + w, window->DC.CursorPos.y + label_size.y + style.FramePadding.y * 2.0f));
  const ImRect total_bb(frame_bb.Min, ImVec2(frame_bb.Max.x, frame_bb.Max.y));
  const float value_x2 = ImMax(frame_bb.Min.x, frame_bb.Max.x - arrow_size);
  ImGui::ItemSize(total_bb, style.FramePadding.y);
  if (!ImGui::ItemAdd(total_bb, popupId, &frame_bb))
    return {};

  bool hovered, held;
  bool pressed = ImGui::ButtonBehavior(frame_bb, popupId, &hovered, &held);

  if (!popupIsAlreadyOpened)
  {
    const ImU32 frame_col = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    ImGui::RenderNavHighlight(frame_bb, popupId);
    if (!(flags & ImGuiComboFlags_NoPreview))
      window->DrawList->AddRectFilled(frame_bb.Min, ImVec2(value_x2, frame_bb.Max.y), frame_col, style.FrameRounding,
        (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft);
  }
  if (!(flags & ImGuiComboFlags_NoArrowButton))
  {
    // Arrow in a box rendering
    ImU32 bg_col = ImGui::GetColorU32((popupIsAlreadyOpened || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
    window->DrawList->AddRectFilled(ImVec2(value_x2, frame_bb.Min.y), frame_bb.Max, bg_col, style.FrameRounding,
      (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
    if (value_x2 + arrow_size - style.FramePadding.x <= frame_bb.Max.x)
      ImGui::RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, frame_bb.Min.y + style.FramePadding.y), text_col,
        ImGuiDir_Down, 1.0f);
  }

  // Rendering 'minimized' searchbox
  if (!popupIsAlreadyOpened)
  {
    ImGui::RenderFrameBorder(frame_bb.Min, frame_bb.Max, style.FrameRounding);
    if (!(flags & ImGuiComboFlags_NoPreview))
    {
      const char *str = input.empty() ? label : input.c_str();
      ImGui::RenderTextClipped(ImVec2(frame_bb.Min.x + style.FramePadding.x, frame_bb.Min.y + style.FramePadding.y),
        ImVec2(value_x2, frame_bb.Max.y), str, NULL, NULL, ImVec2(0.0f, 0.0f));
    }

    if ((pressed || g.NavActivateId == popupId || popupNeedsToBeOpened))
    {
      if (window->DC.NavLayerCurrent == 0)
        window->NavLastIds[0] = popupId;
      ImGui::OpenPopupEx(popupId);
      popupIsAlreadyOpened = true;
      popupJustOpened = true;
    }
  }

  if (!popupIsAlreadyOpened)
    return {};

  const float totalWMinusArrow = w - arrow_size;
  const float popupHeight = label_size.y + 2.f * style.WindowPadding.y + (label_size.y + style.ItemInnerSpacing.y) * data.size();
  const float popupSizeData[2] = {totalWMinusArrow, ImMin(500.f, popupHeight)};

  struct ImGuiSizeCallbackWrapper
  {
    static void sizeCallback(ImGuiSizeCallbackData *data)
    {
      float(*popupSizeData)[2] = reinterpret_cast<float(*)[2]>(data->UserData);
      data->DesiredSize = ImVec2((*popupSizeData)[0], (*popupSizeData)[1]);
    }
  };
  ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(totalWMinusArrow, popupHeight), ImGuiSizeCallbackWrapper::sizeCallback,
    (void *)&popupSizeData);

  eastl::string name = "##Combo_";
  name += label;

  // Peek into expected window size so we can position it
  if (ImGuiWindow *popup_window = ImGui::FindWindowByName(name.data()))
  {
    if (popup_window->WasActive)
    {
      ImVec2 size_expected = ImGui::CalcWindowNextAutoFitSize(popup_window);
      if (flags & ImGuiComboFlags_PopupAlignLeft)
        popup_window->AutoPosLastDirection = ImGuiDir_Left;
      ImRect r_outer = ImGui::GetPopupAllowedExtentRect(popup_window);
      ImVec2 pos = ImGui::FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer,
        frame_bb, ImGuiPopupPositionPolicy_ComboBox);

      pos.y -= label_size.y + style.FramePadding.y * 2.0f;

      ImGui::SetNextWindowPos(pos);
    }
  }

  // Horizontally align ourselves with the framed text
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
  bool ret = ImGui::Begin(name.data(), NULL, window_flags);

  ImGui::PushItemWidth(ImGui::GetWindowWidth());
  ImGui::SetCursorPos(ImVec2(0.f, window->DC.CurrLineTextBaseOffset));
  if (popupJustOpened)
    ImGui::SetKeyboardFocusHere(0);

  ComboInfo info{};
  info.inputTextChanged = ImGuiDagor::InputTextWithHint("##inputText", hint, &input,
    ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
  ImGui::PopItemWidth();

  if (!ret)
  {
    ImGui::EndChild();
    ImGui::PopItemWidth();
    ImGui::EndPopup();
    IM_ASSERT(0); // This should never happen as we tested for IsPopupOpen() above
    return {};
  }

  if (!input.empty())
  {
    int new_idx = string_vector_search(input, data);
    int idx = new_idx >= 0 ? new_idx : current_idx;
    info.selectionChanged = current_idx != idx;
    current_idx = idx;
  }

  ImGuiWindowFlags window_flags2 = ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus;
  ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_None,
    window_flags2);

  arrow_scroll(data, current_idx, input, info);

  return info;
}


bool ImGuiDagor::EndComboWithFilter(const char *label, const dag::Vector<eastl::string_view> &data, const int &current_idx,
  eastl::string &input, const ComboInfo &info, bool selectedByMouse)
{
  eastl::string name = "##Combo_";
  name += label;

  if (info.arrowScroll && current_idx > -1)
  {
    input = safe_string_getter(data, current_idx);
    ImGuiWindow *wnd = ImGui::FindWindowByName(name.data());
    const ImGuiID id = wnd->GetID("##inputText");
    ImGuiInputTextState *state = ImGui::GetInputTextState(id);

    const char *buf_end = NULL;
    state->CurLenW = ImTextStrFromUtf8(state->TextW.Data, state->TextW.Size, input.c_str(), NULL, &buf_end);
    state->CurLenA = (int)(buf_end - input.c_str());
    state->CursorClamp();
  }

  ImGui::EndChild();
  ImGui::EndPopup();

  eastl::string sActiveIdxValue = safe_string_getter(data, current_idx);
  bool ret1 = (info.selectionChanged && (!sActiveIdxValue.empty() && sActiveIdxValue == input));

  return info.inputTextChanged || info.arrowScroll || selectedByMouse || ret1;
}


bool ImGuiDagor::ComboWithFilter(const char *label, const dag::Vector<eastl::string_view> &data, int &current_idx,
  eastl::string &input, bool enumerate, bool return_on_arrows, const char *hint, ImGuiComboFlags flags)
{
  int items_count = static_cast<int>(data.size());
  if (items_count == 0)
    return false;

  const float extra_width_for_enumeration =
    enumerate ? get_extra_width_for_enumeration(items_count) + GImGui->Style.WindowPadding.x : 0.f;
  const float width = get_field_widget_width(data) + extra_width_for_enumeration;

  auto res = ImGuiDagor::BeginComboWithFilter(label, data, current_idx, input, width, hint, flags);

  if (res.has_value())
  {
    auto info = res.value();

    if (info.inputTextChanged && !info.arrowScroll)
      ImGui::CloseCurrentPopup();

    float printOffset = ImGui::GetCursorPosX() + extra_width_for_enumeration;
    bool selectedByMouse = false;
    for (int n = 0; n < items_count; n++)
    {
      bool is_selected = n == current_idx;
      if (is_selected && (ImGui::IsWindowAppearing() || info.selectionChanged))
        ImGui::SetScrollHereY();

      if (is_selected && info.arrowScroll)
        ImGui::SetScrollHereY();

      eastl::string select_value = safe_string_getter(data, n);
      char item_id[128];
      if (extra_width_for_enumeration)
      {
        ImGui::Text("%u.", n + 1);
        ImGui::SameLine();
        ImGui::SetCursorPosX(printOffset);
      }
      ImFormatString(item_id, sizeof(item_id), "%s##item_%02d", select_value.c_str(), n);
      if (ImGui::Selectable(item_id, is_selected))
      {
        info.selectionChanged = current_idx != n;
        current_idx = n;
        input = select_value;
        ImGui::CloseCurrentPopup();
        selectedByMouse = true;
      }
    }

    info.arrowScroll = info.arrowScroll && return_on_arrows;

    return ImGuiDagor::EndComboWithFilter(label, data, current_idx, input, info, selectedByMouse);
  }

  return false;
}