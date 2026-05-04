// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "textHandler.h"

#include <gui/dag_imgui.h>
#include <math/dag_e3dColor.h>
#include <util/dag_string.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

class FormattedTextHandler
{
public:
  void clear()
  {
    fullText.clear();
    textLines.clear();
    textLinesSizeValid = false;
    clearSelection();
  }

  void clearSelection() { characterSelectionStart = characterSelectionEnd = -1; }
  bool hasSelection() const { return characterSelectionStart >= 0; }

  void addText(const char *text, int color_index, bool bold, bool start_new_line)
  {
    // Split the text into lines to make selection handling easier.
    while (true)
    {
      if (start_new_line && !textLines.empty())
      {
        TextLine &textLine = textLines.back();
        if (textLine.textSpans.empty())
          textLines.back().addText(1, color_index, bold);
        else
          textLine.textSpans.back().length += 1;

        fullText += "\n";
        textLines.push_back();
      }

      const char *lineEnd;
      const char *nextLine = getLine(text, lineEnd);
      if (lineEnd == text && !nextLine) // empty text, no new line
        break;

      if (textLines.empty())
        textLines.push_back();

      TextLine &textLine = textLines.back();
      fullText.append(text, lineEnd - text);
      textLine.addText(lineEnd - text, color_index, bold);

      if (!nextLine)
        break;

      start_new_line = true;
      text = nextLine;
    }

    textLinesSizeValid = false;
  }

  const String &getAsString() const { return fullText; }

  String getSelectionAsString() const
  {
    if (!hasSelection())
      return String();

    int orderedCharacterSelectionStart, orderedCharacterSelectionEnd;
    getOrderedSelection(orderedCharacterSelectionStart, orderedCharacterSelectionEnd);
    if (orderedCharacterSelectionStart >= fullText.length())
      return String();
    const int length = min(orderedCharacterSelectionEnd - orderedCharacterSelectionStart, fullText.length());
    if (length <= 0)
      return String();

    return String(fullText.c_str() + orderedCharacterSelectionStart, length);
  }

  const ImVec2 &getTextLinesSize(float wrap_width)
  {
    updateTextCache(wrap_width);
    return textLinesSize;
  }

  void onHit(const char *hit_text, bool clicked)
  {
    const int textIndex = hit_text - fullText.c_str();

    if (clicked)
    {
      if (characterSelectionStart >= 0 && (ImGui::GetIO().KeyMods & ImGuiMod_Shift) != 0)
        characterSelectionEnd = textIndex;
      else
        characterSelectionStart = characterSelectionEnd = textIndex;
    }
    else if (characterSelectionStart >= 0)
    {
      characterSelectionEnd = textIndex;
    }
  }

  void updateImgui(float wrap_width, ImGuiID &item_id, bool &item_focused)
  {
    const char *selectionStart = nullptr;
    const char *selectionEnd = nullptr;
    int orderedCharacterSelectionStart, orderedCharacterSelectionEnd;
    getOrderedSelection(orderedCharacterSelectionStart, orderedCharacterSelectionEnd);
    if (orderedCharacterSelectionStart < fullText.length())
    {
      const int length = min(orderedCharacterSelectionEnd - orderedCharacterSelectionStart, fullText.length());
      if (length > 0)
      {
        selectionStart = fullText.c_str() + orderedCharacterSelectionStart;
        selectionEnd = fullText.c_str() + orderedCharacterSelectionEnd;
      }
    }

    updateTextCache(wrap_width);
    G_ASSERT(textLinesSizeValid);

    const ImVec2 startCursorPos = ImGui::GetCursorScreenPos();
    const ImVec2 regionAvail = ImGui::GetContentRegionAvail();
    // Allow using right click to focus the control.
    ImGui::InvisibleButton("OutputText", ImVec2(max(textLinesSize.x, regionAvail.x), max(textLinesSize.y, regionAvail.y)),
      ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    // But do not change selection with the right click.
    const bool leftClicked = ImGui::IsItemActivated() && !ImGui::IsMouseDown(ImGuiMouseButton_Right);
    const bool select = leftClicked || (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left));

    item_id = ImGui::GetItemID();
    item_focused = ImGui::IsItemFocused();

    const bool itemHovered = ImGui::IsItemHovered();
    if (itemHovered)
      ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);

    ImGui::SetCursorScreenPos(startCursorPos);

    const ImRect &clipRect = ImGui::GetCurrentWindowRead()->ClipRect;
    const float mousePosY = ImGui::GetMousePos().y;

    updateFirstVisibleLine(startCursorPos, clipRect.Min.y);
    G_ASSERT(firstVisibleLineIndexValid);
    if (firstVisibleLineIndex < 0)
      return;

    const char *text = fullText.c_str() + firstVisibleLineFullTextOffset;
    float nextLineStartY = clipRect.Max.y; // Give it an invalid value but it will be always set by the loop.
    bool foundHit = false;
    for (int lineIndex = firstVisibleLineIndex; lineIndex < textLines.size(); ++lineIndex)
    {
      const TextLine &line = textLines[lineIndex];
      G_ASSERT(line.boundsValid);

      const ImVec2 lineStartCursorPos = ImVec2(startCursorPos.x, startCursorPos.y + line.relativeBounds.Min.y);
      nextLineStartY = lineStartCursorPos.y + line.relativeBounds.GetHeight();
      const char *const lineStartText = text;
      text += line.getLength();

      lineGlyphs.clear();
      getLineGlyphs(line, lineStartCursorPos, lineStartText, lineGlyphs, wrap_width);

      if (select && !foundHit && mousePosY < lineStartCursorPos.y)
      {
        foundHit = true;
        onHit(lineStartText, leftClicked);
      }

      if (selectionStart && selectionStart <= text && selectionEnd >= lineStartText)
        LineGlyphHelper::drawSelectionRectangle(lineGlyphs, lineStartText, selectionStart, selectionEnd, item_focused);

      LineGlyphHelper::drawText(lineGlyphs);

      if (select && !foundHit && mousePosY >= lineStartCursorPos.y && mousePosY < nextLineStartY)
        if (const char *hitText = LineGlyphHelper::getHitTextByMousePosition(lineGlyphs, lineStartCursorPos, lineStartText))
        {
          foundHit = true;
          onHit(hitText, leftClicked);
        }

      if (nextLineStartY > clipRect.Max.y) // next line is no longer visible
        break;
    }

    // Select the entire last drawn line if the click is below it.
    if (select && !foundHit && mousePosY > nextLineStartY && nextLineStartY < clipRect.Max.y)
    {
      foundHit = true;
      onHit(text, leftClicked);
    }

    if (itemHovered)
    {
      const int clickCount = ImGui::GetMouseClickedCount(ImGuiMouseButton_Left);
      if (clickCount == 2)
        extendSelectionToWordOrLine(false);
      else if (clickCount == 3)
        extendSelectionToWordOrLine(true);
    }
  }

private:
  struct TextSpan
  {
    unsigned length;
    int colorIndex; // PropPanel::ColorOverride
    bool bold;
  };

  struct TextLine
  {
    void addText(unsigned text_length, int color_index, bool bold)
    {
      if (textSpans.empty() || textSpans.back().colorIndex != color_index || textSpans.back().bold != bold)
      {
        TextSpan &span = textSpans.push_back();
        span.length = text_length;
        span.colorIndex = color_index;
        span.bold = bold;
      }
      else
      {
        textSpans.back().length += text_length;
      }

      boundsValid = false;
    }

    unsigned getLength() const
    {
      unsigned length = 0;
      for (const TextSpan &span : textSpans)
        length += span.length;
      return length;
    }

    dag::Vector<TextSpan> textSpans;
    ImRect relativeBounds; // relative to the cursor's start position
    bool boundsValid = false;
  };

  struct TextCacheInfluencingSettings
  {
    void fill(float wrap_width)
    {
      wrapWidth = wrap_width;
      fontSize = ImGui::GetFontSize();
    }

    bool operator==(const TextCacheInfluencingSettings &) const = default;

    float wrapWidth = 0.0f;
    float fontSize = 0.0f;
  };

  void getOrderedSelection(int &ordered_start, int &ordered_end) const
  {
    ordered_start = characterSelectionStart;
    ordered_end = characterSelectionEnd;
    if (ordered_start >= 0) //-V1051 Consider checking for misprints. It's possible that the 'ordered_end' should be checked here.
    {
      if (ordered_end < 0)
        ordered_end = ordered_start;
      else if (ordered_end < ordered_start)
        eastl::swap(ordered_start, ordered_end);
    }
    else
    {
      ordered_end = -1;
    }
  }

  static const char *getLine(const char *text, const char *&line_end)
  {
    for (; *text != 0; ++text)
    {
      if (*text == '\n')
      {
        line_end = text;
        return text + 1;
      }
    }

    line_end = text;
    return nullptr;
  }

  static void getLineGlyphs(const TextLine &line, const ImVec2 &line_start_cursor_pos, const char *line_start_text,
    dag::Vector<LineGlyph> &glyphs, float wrap_width)
  {
    const float fontScale = ImGui::GetFontSize() / ImGui::GetFontBaked()->Size;
    ImVec2 handlerCursorPosition = line_start_cursor_pos;
    const char *handlerText = line_start_text;
    for (const TextSpan &span : line.textSpans)
    {
      if (span.length == 0)
        continue;

      if (span.bold)
        ImGui::PushFont(imgui_get_bold_font(), 0.0f);

      ImguiTextGlyphIterator glyphIterator(line_start_cursor_pos.x, wrap_width, handlerCursorPosition.x, handlerCursorPosition.y,
        handlerText, handlerText + span.length);
      while (true)
      {
        const char *textPos = glyphIterator.getTextPos();
        if (!glyphIterator.iterate())
          break;

        LineGlyph &glyph = glyphs.push_back();
        glyph.glyph = glyphIterator.getGlyph();
        glyph.advanceX = (glyph.glyph ? glyph.glyph->AdvanceX : ImGui::GetFontBaked()->GetCharAdvance((ImWchar)' ')) * fontScale;
        glyph.x = handlerCursorPosition.x;
        glyph.y = handlerCursorPosition.y;
        glyph.characterIndex = textPos - line_start_text;
        glyph.colorIndex = span.colorIndex;

        handlerCursorPosition.x = glyphIterator.getX();
        handlerCursorPosition.y = glyphIterator.getY();
      }

      if (span.bold)
        ImGui::PopFont();

      handlerText += span.length;
    }
  }

  void updateTextCache(float wrap_width)
  {
    TextCacheInfluencingSettings textCacheInfluencingSettings;
    textCacheInfluencingSettings.fill(wrap_width);
    const bool invalidateEverything = textCacheInfluencingSettings != lastTextCacheInfluencingSettings;
    if (textLinesSizeValid && !invalidateEverything)
      return;

    lastTextCacheInfluencingSettings = textCacheInfluencingSettings;

    float cursorPosY = 0.0f;
    float maxX = 0.0f;
    const char *text = fullText.c_str();
    for (TextLine &line : textLines)
    {
      if (!line.boundsValid || invalidateEverything)
      {
        lineGlyphs.clear();
        getLineGlyphs(line, ImVec2(0.0f, cursorPosY), text, lineGlyphs, wrap_width);

        const float nextCursorPosY = (lineGlyphs.empty() ? cursorPosY : lineGlyphs.back().y) + ImGui::GetTextLineHeightWithSpacing();

        line.relativeBounds.Min.x = 0.0f;
        line.relativeBounds.Min.y = cursorPosY;
        line.relativeBounds.Max.x = LineGlyphHelper::getMaxXFromLineGlyphs(lineGlyphs);
        line.relativeBounds.Max.y = nextCursorPosY;
        line.boundsValid = true;
      }

      cursorPosY += line.relativeBounds.GetHeight();
      maxX = max(maxX, line.relativeBounds.GetWidth());
      text += line.getLength();
    }

    textLinesSize = ImVec2(maxX, cursorPosY);
    textLinesSizeValid = true;
    firstVisibleLineIndexValid = false;
  }

  void updateFirstVisibleLine(const ImVec2 &cursor_start_pos, float clip_rect_min_y)
  {
    const float scrollY = ImGui::GetScrollY();
    if (firstVisibleLineIndexValid && scrollY == firstVisibleLineLastScrollPositionY)
      return;

    firstVisibleLineLastScrollPositionY = scrollY;
    firstVisibleLineIndex = -1;
    firstVisibleLineFullTextOffset = 0;

    for (int lineIndex = 0; lineIndex < textLines.size(); ++lineIndex)
    {
      const TextLine &line = textLines[lineIndex];
      G_ASSERT(line.boundsValid);

      const float nextLineStartY = cursor_start_pos.y + line.relativeBounds.Max.y;
      if (nextLineStartY >= clip_rect_min_y)
      {
        firstVisibleLineIndex = lineIndex;
        break;
      }

      firstVisibleLineFullTextOffset += line.getLength();
    }

    firstVisibleLineIndexValid = true;
  }

  void extendSelectionToLeft(int &index, bool use_line_separator)
  {
    // Start is inclusive.
    for (int i = index - 1; i >= 0 && !isSeparator(fullText[i], use_line_separator); --i)
      --index;
  }

  void extendSelectionToRight(int &index, bool use_line_separator)
  {
    // End is non-inclusive.
    while (index < fullText.length() && !isSeparator(fullText[index], use_line_separator))
      ++index;
  }

  void extendSelectionToWordOrLine(bool use_line_separator)
  {
    if (characterSelectionStart >= 0 && (ImGui::GetIO().KeyMods & ImGuiMod_Shift) != 0) // Extend selection?
    {
      if (characterSelectionEnd < 0 || characterSelectionEnd >= fullText.length())
        return;

      if (characterSelectionEnd >= characterSelectionStart)
        extendSelectionToRight(characterSelectionEnd, use_line_separator);
      else
        extendSelectionToLeft(characterSelectionEnd, use_line_separator);
    }
    else
    {
      if (characterSelectionStart < 0 || characterSelectionStart >= fullText.length())
        return;

      characterSelectionEnd = characterSelectionStart;
      extendSelectionToLeft(characterSelectionStart, use_line_separator);
      extendSelectionToRight(characterSelectionEnd, use_line_separator);
    }
  }

  // This a modified version of is_separator() from imgui_widgets.cpp.
  static bool isWordSeparator(unsigned c)
  {
    return ImCharIsBlankW(c) || c == ',' || c == ';' || c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' ||
           c == '<' || c == '>' || c == '|' || c == '\n' || c == '\r' || c == '.' || c == '!' || c == '\\' || c == '/';
  }

  static bool isLineSeparator(unsigned c) { return c == '\n' || c == '\r'; }

  static bool isSeparator(unsigned c, bool use_line_separator) { return use_line_separator ? isLineSeparator(c) : isWordSeparator(c); }

  String fullText;
  dag::Vector<TextLine> textLines;
  dag::Vector<LineGlyph> lineGlyphs; // temporary buffer
  int characterSelectionStart = -1;  // inclusive
  int characterSelectionEnd = -1;    // not inclusive

  // caching
  TextCacheInfluencingSettings lastTextCacheInfluencingSettings;
  ImVec2 textLinesSize;
  bool textLinesSizeValid = false;
  int firstVisibleLineIndex = -1;
  bool firstVisibleLineIndexValid = false;
  float firstVisibleLineLastScrollPositionY = 0.0f;
  int firstVisibleLineFullTextOffset = 0;
};
