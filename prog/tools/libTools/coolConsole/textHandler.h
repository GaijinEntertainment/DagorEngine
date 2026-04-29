// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <propPanel/colors.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

// Returns font glyphs and their coordinates for a given text.
// This is based on ImFont::RenderText().
class ImguiTextGlyphIterator
{
public:
  ImguiTextGlyphIterator(float line_start_x, float wrap_width, float in_x, float in_y, const char *text_begin, const char *text_end) :
    textBegin(text_begin),
    textEnd(text_end),
    font(ImGui::GetFont()),
    fontBaked(ImGui::GetFontBaked()),
    fontSize(ImGui::GetFontSize()),
    start_x(IM_TRUNC(line_start_x)), // align to be pixel perfect
    fontScale(ImGui::GetFontSize() / ImGui::GetFontBaked()->Size),
    lineHeight(ImGui::GetTextLineHeightWithSpacing()),
    wrapWidth(wrap_width),
    wordWrapEnabled(wrap_width > 0.0f),
    text(text_begin),
    x(IM_TRUNC(in_x)), // align to be pixel perfect
    y(IM_TRUNC(in_y))  // align to be pixel perfect
  {}

  // Returns false when there are no more characters.
  bool iterate()
  {
    if (text >= textEnd)
      return false;

    if (wordWrapEnabled)
    {
      // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for
      // what's essentially an uncommon feature.
      if (!wordWrapEol)
        wordWrapEol = font->CalcWordWrapPosition(fontSize, text, textEnd, wrapWidth - (x - start_x));

      if (text >= wordWrapEol)
      {
        x = start_x;
        y += lineHeight;
        wordWrapEol = nullptr;
        text = calcWordWrapNextLineStartA(text, textEnd); // Wrapping skips upcoming blanks

        glyph = nullptr;
        return true;
      }
    }

    // Decode and advance source
    unsigned int currentChar = (unsigned int)*text;
    if (currentChar < 0x80)
      text += 1;
    else
      text += ImTextCharFromUtf8(&currentChar, text, textEnd);

    if (currentChar < 32)
    {
      if (currentChar == '\n')
      {
        x = start_x;
        y += lineHeight;
        glyph = nullptr;
        return true;
      }
      if (currentChar == '\r')
      {
        glyph = nullptr;
        return true;
      }
    }

    glyph = fontBaked->FindGlyph((ImWchar)currentChar);
    if (glyph != nullptr)
      x += glyph->AdvanceX * fontScale;

    return true;
  }

  // After getNext() this returns the current glyph.
  const ImFontGlyph *getGlyph() const { return glyph; }

  // After getNext() this will return the start of the next character.
  float getX() const { return x; }
  float getY() const { return y; }

  // After getNext() this will return the start of the next text pointer.
  const char *getTextPos() { return text; }

private:
  // Taken from imgui_draw.cpp.
  static inline const char *calcWordWrapNextLineStartA(const char *text, const char *text_end)
  {
    while (text < text_end && ImCharIsBlankA(*text))
      text++;
    if (*text == '\n')
      text++;
    return text;
  }

  static constexpr unsigned WORD_WRAP_CHAR = 1;

  const char *const textBegin;
  const char *const textEnd;
  ImFont *const font;
  ImFontBaked *const fontBaked;
  const float fontSize;
  const float start_x;
  const float fontScale;
  const float lineHeight;
  const float wrapWidth;
  const bool wordWrapEnabled;

  const ImFontGlyph *glyph = nullptr;
  const char *wordWrapEol = nullptr;
  const char *text;
  float x;
  float y;
};

struct LineGlyph
{
  const ImFontGlyph *glyph;
  float x;
  float y;
  float advanceX;     // scaled with FontScale
  int characterIndex; // within the line
  int colorIndex;     // PropPanel::ColorOverride
};

class LineGlyphHelper
{
public:
  static void drawChar(const ImFontGlyph *glyph, float x, float y, float font_scale, ImU32 color, const ImVec4 &clip_rect)
  {
    if (!glyph || !glyph->Visible)
      return;

    const float x1 = x + glyph->X0 * font_scale;
    const float x2 = x + glyph->X1 * font_scale;
    if (x1 <= clip_rect.z && x2 >= clip_rect.x)
    {
      const float y1 = y + glyph->Y0 * font_scale;
      const float y2 = y + glyph->Y1 * font_scale;

      if (glyph->Colored)
        color |= ~IM_COL32_A_MASK;

      ImDrawList *drawList = ImGui::GetWindowDrawList();
      drawList->PrimReserve(6, 4);
      drawList->PrimRectUV(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V1), color);
    }
  }

  static void drawText(dag::ConstSpan<LineGlyph> line_glyphs)
  {
    const float fontScale = ImGui::GetFontSize() / ImGui::GetFontBaked()->Size;
    const ImGuiWindow *window = ImGui::GetCurrentWindow();
    const ImVec4 clipRect = window->DrawList->_CmdHeader.ClipRect;

    for (const LineGlyph &lineGlyph : line_glyphs)
    {
      const ImU32 textColor = PropPanel::getOverriddenColorU32(lineGlyph.colorIndex);
      drawChar(lineGlyph.glyph, lineGlyph.x, lineGlyph.y, fontScale, textColor, clipRect);
    }
  }

  static float getMaxXFromLineGlyphs(dag::ConstSpan<LineGlyph> line_glyphs)
  {
    float maxX = 0.0;
    for (const LineGlyph &lineGlyph : line_glyphs)
      if (lineGlyph.x > maxX)
        maxX = lineGlyph.x;
    return maxX;
  }

  static const char *getHitTextByMousePosition(const dag::ConstSpan<LineGlyph> line_glyphs, const ImVec2 &line_start_cursor_pos,
    const char *line_start_text)
  {
    if (line_glyphs.empty())
      return nullptr;

    const ImVec2 mousePosition = ImGui::GetMousePos();
    const float lineHeight = ImGui::GetTextLineHeightWithSpacing();

    if (mousePosition.y < line_glyphs.front().y)
      return nullptr;

    if (mousePosition.y >= (line_glyphs.back().y + lineHeight))
      return nullptr;

    // Checking for lineGlyph.glyph (!= nullptr) ensures that new line characters are not matched.

    // Find exact hit within the line's characters that are within the text area.
    const ImRect &clipRect = ImGui::GetCurrentWindowRead()->ClipRect;
    for (const LineGlyph &lineGlyph : line_glyphs)
    {
      if (mousePosition.x >= lineGlyph.x && mousePosition.x < (lineGlyph.x + lineGlyph.advanceX) && mousePosition.y >= lineGlyph.y &&
          mousePosition.y < (lineGlyph.y + lineHeight) && lineGlyph.glyph)
      {
        const ImRect bb(lineGlyph.x, lineGlyph.y, lineGlyph.x + lineGlyph.advanceX, lineGlyph.y + lineHeight);
        if (bb.Overlaps(clipRect))
        {
          const char *hitCharacter = line_start_text + lineGlyph.characterIndex;
          if (mousePosition.x >= (lineGlyph.x + (lineGlyph.advanceX * 0.5f)))
            ++hitCharacter;
          return hitCharacter;
        }
      }
    }

    // Find the leftmost hit that may be before the line horizontally (so no clip rect overlap testing is performed) but
    // is within the line vertically.
    // Find the rightmost hit that may be after the line horizontally (so no clip rect overlap testing is performed) but
    // is within the line vertically.
    const char *leftMostHit = nullptr;
    const char *rightMostHit = nullptr;
    for (const LineGlyph &lineGlyph : line_glyphs)
    {
      if (mousePosition.y >= lineGlyph.y && mousePosition.y < (lineGlyph.y + lineHeight) && lineGlyph.glyph)
      {
        if (mousePosition.x < lineGlyph.x && !leftMostHit)
          leftMostHit = line_start_text + lineGlyph.characterIndex;

        if (mousePosition.x >= (lineGlyph.x + lineGlyph.advanceX))
          rightMostHit = line_start_text + lineGlyph.characterIndex + 1;
      }
    }

    return leftMostHit ? leftMostHit : rightMostHit;
  }

  static void drawSelectionRectangle(const dag::ConstSpan<LineGlyph> line_glyphs, const char *line_start_text,
    const char *selection_start, const char *selection_end, bool focused)
  {
    if (line_glyphs.empty())
      return;

    const float lineHeight = ImGui::GetTextLineHeightWithSpacing();

    for (const LineGlyph &lineGlyph : line_glyphs)
    {
      const char *text = line_start_text + lineGlyph.characterIndex;
      if (text >= selection_start && text < selection_end)
      {
        const ImU32 selectionBgColor =
          PropPanel::getOverriddenColorU32(focused ? PropPanel::ColorOverride::CONSOLE_LOG_SELECTED_TEXT_BACKGROUND_FOCUSED
                                                   : PropPanel::ColorOverride::CONSOLE_LOG_SELECTED_TEXT_BACKGROUND);
        ImVec2 rectStart = ImVec2(lineGlyph.x, lineGlyph.y);
        ImGui::GetCurrentWindow()->DrawList->AddRectFilled(rectStart,
          ImVec2(rectStart.x + lineGlyph.advanceX, rectStart.y + lineHeight), selectionBgColor);
      }
    }
  }
};
