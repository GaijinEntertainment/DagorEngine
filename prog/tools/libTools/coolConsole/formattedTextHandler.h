// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "textHandler.h"

#include <gui/dag_imgui.h>
#include <math/dag_e3dColor.h>
#include <propPanel/colors.h>
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

  void updateImgui()
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

    const ImVec2 startCursorPos = ImGui::GetCursorScreenPos();
    const ImVec2 contentSize = ImGui::GetCurrentWindowRead()->ContentSize;
    const ImVec2 regionAvail = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("OutputText", ImVec2(max(contentSize.x, regionAvail.x), max(contentSize.y, regionAvail.y)));

    if (ImGui::IsItemHovered())
      ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);

    ImGui::SetCursorScreenPos(startCursorPos);

    ImGui::PushTextWrapPos(0.0f);

    const ImRect &clipRect = ImGui::GetCurrentWindowRead()->ClipRect;
    const bool clicked = ImGui::IsItemActivated();
    const bool select = clicked || (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left));
    const float mousePosY = ImGui::GetMousePos().y;
    const float scrollY = ImGui::GetScrollY();
    const char *text = fullText.c_str();
    bool foundHit = false;

    SelectionHighlighterTextHandler selectionHighlighter(ImGui::GetTextLineHeightWithSpacing(), selectionStart, selectionEnd);

    for (const TextLine &line : textLines)
    {
      const ImVec2 lineStartCursorPos = ImGui::GetCursorScreenPos();
      const char *lineStartText = text;

      if (select && !foundHit && mousePosY < lineStartCursorPos.y &&
          (lineStartCursorPos.y + ImGui::GetTextLineHeightWithSpacing()) >= clipRect.Min.y)
      {
        foundHit = true;
        onHit(text, clicked);
      }

      for (const TextSpan &span : line.textSpans)
      {
        if (span.length == 0)
          continue;

        if (span.bold)
          ImGui::PushFont(imgui_get_bold_font());

        ImGui::PushStyleColor(ImGuiCol_Text, PropPanel::getOverriddenColor(span.colorIndex));
        ImGui::TextUnformatted(text, text + span.length);
        ImGui::PopStyleColor();

        if (span.bold)
          ImGui::PopFont();

        text += span.length;
      }

      if (select && !foundHit && mousePosY >= lineStartCursorPos.y && mousePosY < ImGui::GetCursorScreenPos().y)
        if (const char *hitText = getHitTextByMousePosition(line, lineStartCursorPos, lineStartText))
        {
          foundHit = true;
          onHit(hitText, clicked);
        }

      if (selectionStart && selectionStart <= text && selectionEnd >= lineStartText)
        drawSelectionRectangle(selectionHighlighter, line, lineStartCursorPos, lineStartText);

      if (ImGui::GetCursorScreenPos().y > clipRect.Max.y)
        break;
    }

    ImGui::PopTextWrapPos();

    // Select the entire last drawn line if the click is below it.
    if (select && !foundHit && mousePosY > ImGui::GetCursorScreenPos().y &&
        (ImGui::GetCursorScreenPos().y - ImGui::GetTextLineHeightWithSpacing()) < clipRect.Max.y)
    {
      foundHit = true;
      onHit(text, clicked);
    }

    if (ImGui::GetMouseClickedCount(ImGuiMouseButton_Left) == 2)
      extendSelectionToWordOrLine(false);
    else if (ImGui::GetMouseClickedCount(ImGuiMouseButton_Left) == 3)
      extendSelectionToWordOrLine(true);
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
    }

    unsigned getLength() const
    {
      unsigned length = 0;
      for (const TextSpan &span : textSpans)
        length += span.length;
      return length;
    }

    dag::Vector<TextSpan> textSpans;
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

  static const char *getHitTextByMousePositionInternal(ImVec2 &cursor_position, const char *text, unsigned text_length,
    const ImVec2 &mouse_position)
  {
    G_ASSERT(text_length > 0);

    const ImGuiWindow *window = ImGui::GetCurrentWindow();
    const float wrapPosX = window->DC.TextWrapPos;
    const bool wrapEnabled = wrapPosX >= 0.0f;
    const float wrapWidth = wrapEnabled ? ImGui::CalcWrapWidthForPos(cursor_position, wrapPosX) : 0.0f;
    const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
    const ImVec4 clipRect = window->DrawList->_CmdHeader.ClipRect;

    HitFinderTextHandler hitHandler(mouse_position, lineHeight);
    cursor_position = hitHandler.processText(cursor_position, clipRect, text, text + text_length, wrapWidth);
    return hitHandler.getHitCharacter();
  }

  const char *getHitTextByMousePosition(const TextLine &line, const ImVec2 &line_start_cursor_pos, const char *line_start_text)
  {
    const ImVec2 mousePosition = ImGui::GetMousePos();
    ImVec2 handlerCursorPosition = line_start_cursor_pos;
    const char *handlerText = line_start_text;
    for (const TextSpan &span : line.textSpans)
    {
      if (span.length == 0)
        continue;

      if (span.bold)
        ImGui::PushFont(imgui_get_bold_font());

      const char *hitText = getHitTextByMousePositionInternal(handlerCursorPosition, handlerText, span.length, mousePosition);

      if (span.bold)
        ImGui::PopFont();

      if (hitText)
        return hitText;

      handlerText += span.length;
    }

    return nullptr;
  }

  static void drawSelectionRectangleInternal(SelectionHighlighterTextHandler &handler, ImVec2 &cursor_position, const char *text,
    unsigned text_length)
  {
    G_ASSERT(text_length > 0);

    const ImGuiWindow *window = ImGui::GetCurrentWindow();
    const float wrapPosX = window->DC.TextWrapPos;
    const bool wrapEnabled = wrapPosX >= 0.0f;
    const float wrapWidth = wrapEnabled ? ImGui::CalcWrapWidthForPos(cursor_position, wrapPosX) : 0.0f;
    const ImVec4 clipRect = window->DrawList->_CmdHeader.ClipRect;

    cursor_position = handler.processText(cursor_position, clipRect, text, text + text_length, wrapWidth);
  }

  void drawSelectionRectangle(SelectionHighlighterTextHandler &handler, const TextLine &line, const ImVec2 &line_start_cursor_pos,
    const char *line_start_text)
  {
    ImVec2 handlerCursorPosition = line_start_cursor_pos;
    const char *handlerText = line_start_text;
    for (const TextSpan &span : line.textSpans)
    {
      if (span.length == 0)
        continue;

      if (span.bold)
        ImGui::PushFont(imgui_get_bold_font());

      drawSelectionRectangleInternal(handler, handlerCursorPosition, handlerText, span.length);

      if (span.bold)
        ImGui::PopFont();

      handlerText += span.length;
    }

    handler.onEnd(handlerCursorPosition.x, handlerCursorPosition.y);
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
           c == '|' || c == '\n' || c == '\r' || c == '.' || c == '!' || c == '\\' || c == '/';
  }

  static bool isLineSeparator(unsigned c) { return c == '\n' || c == '\r'; }

  static bool isSeparator(unsigned c, bool use_line_separator) { return use_line_separator ? isLineSeparator(c) : isWordSeparator(c); }

  String fullText;
  dag::Vector<TextLine> textLines;
  int characterSelectionStart = -1; // inclusive
  int characterSelectionEnd = -1;   // not inclusive
};
