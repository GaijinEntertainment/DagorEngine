// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "textUtil.h"

#include <daRg/dag_element.h>

#include <gui/dag_stdGuiRender.h>
#include <osApiWrappers/dag_unicode.h>
#include "behaviors/bhvTextInput.h"
#include "behaviors/bhvTextAreaEdit.h"


namespace darg
{


int get_next_char_size(const char *start_str)
{
  if (!start_str)
    return 0;
  if ((start_str[0] & 0x80) == 0)
    return 1;

  int i = 1;
  for (; start_str[i] != '\0'; i++)
  {
    if ((start_str[i] & 0x80) == 0)
      return i;
    if ((start_str[i] & 0xC0) == 0xC0)
      return i;
  }

  return i;
}


int get_prev_char_size(const char *str, int len)
{
  int i = 1;
  while (len > 0)
  {
    if ((str[0] & 0x80) == 0)
      return i;
    if ((str[0] & 0xC0) == 0xC0)
      return i;
    str--;
    len--;
    i++;
  }
  return 1;
}


int utf_calc_bytes_for_symbols(const char *str, int utf_len, int n_symbols)
{
  int i = 0, n = 0;
  for (i = 0; i < utf_len && n < n_symbols;)
  {
    ++n;
    i += get_next_char_size(str + i);
  }
  return i;
}


int get_next_text_pos_u(const wchar_t *str, int len, int pos, bool is_increment, bool is_jump_word, bool ignore_spaces)
{
  int dir = is_increment ? 1 : -1;
  if (!is_jump_word)
    return clamp(pos + dir, 0, len);

  if (!ignore_spaces)
  {
    int idxShift = is_increment ? 0 : -1;
    bool isCapturing = false;
    for (int p = pos; p >= 0 && p <= len; p += dir)
    {
      wchar_t ch = str[max(0, p + idxShift)];
      bool isSeparator = ch == L' ' || ch == L'\n';
      if (is_increment == isSeparator)
        isCapturing = true;
      else if (isCapturing)
        return p;
    }
  }

  return is_increment ? len : 0;
}


int wchar_to_symbol(wchar_t wc, char *buffer, int buffer_sz)
{
  G_ASSERT(buffer_sz > 0);
  if (wc < 32)
    return 0;

  if (!wchar_to_utf8(wc, buffer, buffer_sz - 1))
    return 0;

  return buffer[0] ? i_strlen(buffer) : 0;
}


int font_mono_width_from_sq(int font_id, int font_ht, const Sqrat::Object &obj)
{
  if (obj.GetType() & SQOBJECT_NUMERIC)
    return obj.Cast<int>();

  if (obj.GetType() == OT_STRING)
  {
    auto s = obj.GetVar<const SQChar *>();
    StdGuiFontContext fctx;
    StdGuiRender::get_font_context(fctx, font_id, 0, 0, font_ht);
    return int(StdGuiRender::get_str_bbox(s.value, (int)s.valueLen, fctx).width().x);
  }

  return 0;
}


Point2 calc_text_size(const char *str, int len, int font_id, int spacing, int mono_width, int font_ht)
{
  StdGuiFontContext fctx;
  StdGuiRender::get_font_context(fctx, font_id, spacing, mono_width, font_ht);
  BBox2 bbox = StdGuiRender::get_str_bbox(str, len, fctx);

  int height = StdGuiRender::get_font_ascent(fctx) + StdGuiRender::get_font_descent(fctx);

  if (bbox.isempty())
    return Point2(0, height);

  return Point2(bbox.width().x, height);
}


Point2 calc_text_size_u(const wchar_t *str, int len, int font_id, int spacing, int mono_width, int font_ht)
{
  StdGuiFontContext fctx;
  StdGuiRender::get_font_context(fctx, font_id, spacing, mono_width, font_ht);
  BBox2 bbox = StdGuiRender::get_str_bbox_u(str, len, fctx);

  int height = StdGuiRender::get_font_ascent(fctx) + StdGuiRender::get_font_descent(fctx);

  if (bbox.isempty())
    return Point2(0, height);

  return Point2(bbox.width().x, height);
}


bool is_text_input(const Element *elem)
{
  for (const Behavior *bhv : elem->behaviors)
    if (bhv == &bhv_text_input || bhv == &bhv_text_area_edit)
      return true;
  return false;
}


} // namespace darg
