// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

template <typename Derived>
class TextHandler
{
public:
  // This is ImFont::RenderText() with callbacks instead of drawing.
  ImVec2 processText(const ImVec2 &pos, const ImVec4 &clip_rect, const char *text_begin, const char *text_end, float wrap_width)
  {
    // Align to be pixel perfect
    float x = IM_TRUNC(pos.x);
    float y = IM_TRUNC(pos.y);
    if (y > clip_rect.w)
      return pos;

    ImFont *font = ImGui::GetFont();
    const float fontSize = ImGui::GetFontSize();
    const float start_x = x;
    const float scale = 1.0f;
    const float line_height = fontSize * scale;
    const bool word_wrap_enabled = (wrap_width > 0.0f);

    // Fast-forward to first visible line
    const char *s = text_begin;
    if (y + line_height < clip_rect.y)
      while (y + line_height < clip_rect.y && s < text_end)
      {
        const char *line_end = (const char *)memchr(s, '\n', text_end - s);
        if (word_wrap_enabled)
        {
          // FIXME-OPT: This is not optimal as do first do a search for \n before calling CalcWordWrapPositionA().
          // If the specs for CalcWordWrapPositionA() were reworked to optionally return on \n we could combine both.
          // However it is still better than nothing performing the fast-forward!
          s = font->CalcWordWrapPositionA(scale, s, line_end ? line_end : text_end, wrap_width);
          s = calcWordWrapNextLineStartA(s, text_end);
        }
        else
        {
          s = line_end ? line_end + 1 : text_end;
        }
        y += line_height;
      }

    // For large text, scan for the last visible line in order to avoid over-reserving in the call to PrimReserve()
    // Note that very large horizontal line will still be affected by the issue (e.g. a one megabyte string buffer without a newline
    // will likely crash atm)
    if (text_end - s > 10000 && !word_wrap_enabled)
    {
      const char *s_end = s;
      float y_end = y;
      while (y_end < clip_rect.w && s_end < text_end)
      {
        s_end = (const char *)memchr(s_end, '\n', text_end - s_end);
        s_end = s_end ? s_end + 1 : text_end;
        y_end += line_height;
      }
      text_end = s_end;
    }
    if (s == text_end)
      return ImVec2(x, y);

    const char *word_wrap_eol = nullptr;

    static_cast<Derived *>(this)->onStart(s, x, y);

    while (s < text_end)
    {
      if (word_wrap_enabled)
      {
        // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for
        // what's essentially an uncommon feature.
        if (!word_wrap_eol)
          word_wrap_eol = font->CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - start_x));

        if (s >= word_wrap_eol)
        {
          const ImFontGlyph *glyph = font->FindGlyph((ImWchar)' ');
          const float charWidth = glyph ? (glyph->AdvanceX * scale) : 0.0f;
          static_cast<Derived *>(this)->onChar(s, x, y, charWidth);
          static_cast<Derived *>(this)->onNewLine(s + 1, x, y, charWidth);

          x = start_x;
          y += line_height;
          if (y > clip_rect.w)
            break; // break out of main loop
          word_wrap_eol = nullptr;
          s = calcWordWrapNextLineStartA(s, text_end); // Wrapping skips upcoming blanks
          continue;
        }
      }

      // Decode and advance source
      const char *currentCharString = s;
      unsigned int c = (unsigned int)*s;
      if (c < 0x80)
        s += 1;
      else
        s += ImTextCharFromUtf8(&c, s, text_end);

      if (c < 32)
      {
        if (c == '\n')
        {
          const ImFontGlyph *glyph = font->FindGlyph((ImWchar)' ');
          const float charWidth = glyph ? (glyph->AdvanceX * scale) : 0.0f;
          static_cast<Derived *>(this)->onChar(currentCharString, x, y, charWidth);
          static_cast<Derived *>(this)->onNewLine(currentCharString, x, y, charWidth);

          x = start_x;
          y += line_height;
          if (y > clip_rect.w)
            break; // break out of main loop
          continue;
        }
        if (c == '\r')
          continue;
      }

      const ImFontGlyph *glyph = font->FindGlyph((ImWchar)c);
      if (glyph == nullptr)
        continue;

      float char_width = glyph->AdvanceX * scale;
      static_cast<Derived *>(this)->onChar(currentCharString, x, y, char_width);
      x += char_width;
    }

    return ImVec2(x, y);
  }

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
};

class HitFinderTextHandler : public TextHandler<HitFinderTextHandler>
{
public:
  HitFinderTextHandler(const ImVec2 &mouse_position, float line_height) : mousePosition(mouse_position), lineHeight(line_height) {}

  void onStart(const char *text, float x, float y)
  {
    if (mousePosition.x < x && mousePosition.y >= y && mousePosition.y < (y + lineHeight) && isVisible(x, y, 1.0f))
      hitCharacter = text;
  }

  void onChar(const char *text, float x, float y, float char_width)
  {
    if (mousePosition.x >= x && mousePosition.x < (x + char_width) && mousePosition.y >= y && mousePosition.y < (y + lineHeight) &&
        isVisible(x, y, char_width))
    {
      hitCharacter = text;
      if (mousePosition.x >= (x + (char_width * 0.5f)))
        ++hitCharacter;
    }
  }

  void onNewLine(const char *text, float x, float y, float char_width)
  {
    if (mousePosition.x > x && mousePosition.y >= y && mousePosition.y < (y + lineHeight) && isVisible(x, y, char_width))
      hitCharacter = text;
  }

  const char *getHitCharacter() const { return hitCharacter; }

private:
  bool isVisible(float x, float y, float char_width) const
  {
    const ImRect bb(x, y, x + char_width, y + lineHeight);
    return bb.Overlaps(ImGui::GetCurrentWindowRead()->ClipRect);
  }

  const ImVec2 mousePosition;
  const float lineHeight;
  const char *hitCharacter = nullptr;
};

class SelectionHighlighterTextHandler : public TextHandler<SelectionHighlighterTextHandler>
{
public:
  SelectionHighlighterTextHandler(float line_height, const char *selection_start, const char *selection_end) :
    lineHeight(line_height), selectionStart(selection_start), selectionEnd(selection_end)
  {}

  void onStart(const char *text, float x, float y)
  {
    if (rectStartSet)
    {
      if (text >= selectionEnd)
        endRectangle(x, y);
    }
    else
    {
      if (text >= selectionStart && text < selectionEnd)
        startRectangle(x, y);
    }
  }

  void onChar(const char *text, float x, float y, float char_width)
  {
    if (rectStartSet)
    {
      if (text >= selectionEnd)
        endRectangle(x, y);
    }
    else
    {
      if (text >= selectionStart && text < selectionEnd)
        startRectangle(x, y);
    }
  }

  void onNewLine(const char *text, float x, float y, float char_width)
  {
    if (rectStartSet)
    {
      endRectangle(x + char_width, y);
    }
    else
    {
      if (text >= selectionStart && text < selectionEnd)
        startRectangle(x, y);
    }
  }

  void onEnd(float x, float y)
  {
    if (rectStartSet)
      endRectangle(x, y);
  }

private:
  void startRectangle(float x, float y)
  {
    rectStart = ImVec2(x, y);
    rectStartSet = true;
  }

  void endRectangle(float x, float y)
  {
    const ImU32 selectionBgColor = ImGui::GetColorU32(ImGuiCol_TextSelectedBg, 0.5f);
    ImGui::GetCurrentWindow()->DrawList->AddRectFilled(rectStart, ImVec2(x, y + lineHeight), selectionBgColor);
    rectStartSet = false;
  }

  const float lineHeight;
  const char *const selectionStart;
  const char *const selectionEnd;

  ImVec2 rectStart;
  bool rectStartSet = false;
};
