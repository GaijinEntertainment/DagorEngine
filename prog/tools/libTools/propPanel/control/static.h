// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class StaticPropertyControl : public PropertyControlBase
{
public:
  StaticPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], hdpi::Px h, bool use_text_width = false, bool word_wrap = false, bool monospace = false,
    int wrap_width_in_chars = 0) :
    PropertyControlBase(id, event_handler, parent, x, y, w, h),
    controlCaption(caption),
    useTextWidth(use_text_width),
    wordWrap(word_wrap),
    monospace(monospace),
    wrapWidthInChars(wrap_width_in_chars)
  {}

  unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION | CONTROL_DATA_TYPE_STRING; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_STRING; }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setTextValue(const char value[]) override { controlCaption = value; }

  int getTextValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlCaption, buffer, buflen);
  }

  void setBoolValue(bool value) override { bold = value; }

  const char *getImguiTypeName() const override { return "Static"; }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    // Use full width by default.
    const float itemWidth = mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : -FLT_MIN;
    ImGui::SetNextItemWidth(itemWidth);

    if (bold)
      ImGui::PushFont(imgui_get_bold_font(), 0.0f);
    if (monospace)
      ImGui::PushFont(imgui_get_mono_font(), 0.0f); // 0.0f keeps the current size

    if (wordWrap)
    {
      if (wrapWidthInChars > 0)
      {
        // Use the width of the "0" character as the average character width. This way the wrap follows the font and the
        // DPI scale.
        const float desiredWrapWidth = ImGui::GetFontBaked()->GetCharAdvance('0') * wrapWidthInChars;
        // Never wrap past the window's edge.
        const float wrapWidth = min(desiredWrapWidth, ImGui::GetContentRegionAvail().x);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrapWidth);
      }
      else
        ImGui::PushTextWrapPos(0.0f); // Wrap at the window's edge.

      ImGui::TextEx(controlCaption.begin(), controlCaption.end(),
        (useTextWidth ? ImGuiTextFlags_None : ImGuiTextFlags_NoWidthForLargeClippedText));

      tooltip_helper.setPreviousImguiControlTooltip(this, controlTooltip.begin(), controlTooltip.end());

      ImGui::PopTextWrapPos();
    }
    else
    {
      labelWithTooltip(controlCaption.begin(), controlCaption.end(), useTextWidth);
    }

    if (monospace)
      ImGui::PopFont();
    if (bold)
      ImGui::PopFont();
  }

  Point2 getPreferredSize(float max_width_px) const override
  {
    ImFont *font;
    if (monospace)
      font = imgui_get_mono_font();
    else if (bold)
      font = imgui_get_bold_font();
    else
      font = ImGui::GetFont();

    if (!font)
      return Point2(0.0f, 0.0f);

    // PushFont() is needed to get the proper scaled font size. GetFontSize() returns the unscaled base size outside
    // the frame scope. See ImGui::UpdateFontsEndFrame().
    ImGui::PushFont(font, 0.0f);
    const float fontSize = ImGui::GetFontSize();
    ImGui::PopFont();

    float wrapWidth = 0.0f;
    if (wordWrap)
    {
      if (wrapWidthInChars > 0)
        wrapWidth = min(font->GetFontBaked(fontSize)->GetCharAdvance('0') * wrapWidthInChars, max_width_px);
      else
        wrapWidth = max_width_px;
    }

    ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, wrapWidth, controlCaption.begin(), controlCaption.end());

    // When word wrapping is off the control uses ImguiHelper::labelOnly(), which adds its frame padding.
    if (!wordWrap)
      size.y += ImGui::GetStyle().FramePadding.y * 2.0f;

    // updateImgui() clips the drawn item to mW (ellipsizing longer captions) instead of growing to fit them.
    // With mW == 0 it fills the available width, and the full caption fits in the text width.
    if (!wordWrap && !useTextWidth && mW > 0)
      return Point2(min((float)mW, max_width_px), size.y);

    return Point2(size.x, size.y);
  }

private:
  String controlCaption;
  bool controlEnabled = true;
  bool bold = false;
  bool useTextWidth = false;
  bool wordWrap = false;
  bool monospace = false;
  int wrapWidthInChars = 0;
};

} // namespace PropPanel
