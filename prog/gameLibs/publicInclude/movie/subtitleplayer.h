//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gui/dag_stdGuiRender.h>
#include <util/dag_localization.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_bounds2.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_files.h>
#include <debug/dag_debug.h>
#include <stdio.h>

class MovieSubPlayer
{
public:
  MovieSubPlayer() : items(midmem), autoLines(false) {}

  void initSrt(const char *filename)
  {
    autoLines = true;
    length = 0;
    clear_and_shrink(items);
    const char *fontName = "subtitles"; // game can always alias it to any font
    fontId = StdGuiRender::get_font_id(fontName);
    color = E3DCOLOR(255, 255, 255);

    char *buf = NULL;
    SubItem item;
    int state = 0;
    char *begin, *end;

    file_ptr_t fp = ::df_open(filename, DF_READ);
    if (!fp)
      return;
    int flen = ::df_length(fp);
    if (flen <= 0)
      goto finish;
    buf = new char[flen + 1];
    ::df_read(fp, buf, flen);
    buf[flen] = 0;

    begin = buf;
    if ((unsigned char)(*begin) == 0xEF)
      begin += 3;
    while ((begin < buf + flen) && ((*begin == '\n') || (*begin == '\r')))
      begin++;
    if (begin >= buf + flen)
      goto finish;

    while (begin < buf + flen)
    {
      end = begin;
      while ((end < buf + flen) && *end && (*end != '\n') && (*end != '\r'))
        end++;
      // if (end > begin)
      {
        *end = 0;
        // parse line
        switch (state)
        {
          case 0:
          {
            int idx = 0;
            if (sscanf(begin, "%d", &idx) != 1)
              goto finish;
            if (idx != items.size() + 1) // index is one-based
              goto finish;
            item.from = 0;
            item.to = 0;
            item.text = "";
            state = 1;
          }
          break;
          case 1:
          {
            int from_hr, from_min, from_sec, from_msec;
            int to_hr, to_min, to_sec, to_msec;
            if (sscanf(begin, "%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d", &from_hr, &from_min, &from_sec, &from_msec, &to_hr,
                  &to_min, &to_sec, &to_msec) != 8)
              goto finish;
            item.from = from_msec + (from_sec + (from_hr * 60 + from_min) * 60) * 1000;
            item.to = to_msec + (to_sec + (to_hr * 60 + to_min) * 60) * 1000;
            state = 2;
          }
          break;
          case 2:
          {
            if (end > begin + 1)
            {
              item.text += String(begin);
              item.text += String("\r\n");
            }
            else
            {
              items.push_back(item);
              state = 0;
            }
          }
          break;
        };
      }
      begin = end;
      while ((begin < end + 2) && (begin < buf + flen) && ((*begin == 0) || (*begin == '\n') || (*begin == '\r')))
        begin++;
    };
    if ((state == 2) && item.text.length() > 0)
      items.push_back(item);

  finish:
    if (buf)
      delete[] buf;
    if (fp)
      ::df_close(fp);
  }
  void init(const char *filename, const char *sublangtiming)
  {
    autoLines = false;
    length = 0;
    clear_and_shrink(items);
    DataBlock blk(filename);

    const char *fontName = blk.getStr("font", NULL);
    fontId = fontName ? StdGuiRender::get_font_id(fontName) : -1;

    G_ASSERTF(fontId >= 0, "Font for subtitles not found: %s", fontName);

    color = blk.getE3dcolor("color", E3DCOLOR(255, 255, 255));
    int itemId = blk.getNameId("line");
    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == itemId)
      {
        DataBlock *cb = blk.getBlock(i);

        SubItem item;
        if (cb->paramExists("loc"))
          item.text = get_localized_text(cb->getStr("loc", ""));
        else
          item.text = cb->getStr("text", "");

        if (sublangtiming && cb->getBlockByName(sublangtiming))
          cb = cb->getBlockByName(sublangtiming);
        item.from = cb->getInt("from", 0);
        item.to = cb->getInt("to", -1);
        items.push_back(item);
        if (item.to > length)
          length = item.to;
      }
  }

  void render(int pos_ms)
  {
    const char *text = getText(pos_ms);
    int max_lines = 4;

    StdGuiRender::set_draw_str_attr(FFT_GLOW, 0, 0, E3DCOLOR(0, 0, 0, 255), 56);
    StdGuiRender::set_color(color);
    StdGuiRender::set_font(fontId);
    renderMultilineTex(text, 0.95, autoLines ? -1 : max_lines);
  }

  const char *getText(unsigned ms)
  {
    for (int i = 0; i < items.size(); i++)
      if ((items[i].from <= ms) && (items[i].to > ms))
        return items[i].text;
    return NULL;
  }

  int fontId;
  unsigned color;
  int length;

public:
  static void renderMultilineTex(const char *text, float lower_scr_bound, int max_lines)
  {
    if (!text || strlen(text) >= 1024)
      return;

    char lines[1024];
    strncpy(lines, text, sizeof(lines) - 1);
    lines[sizeof(lines) - 1] = 0;
    real scrW = StdGuiRender::screen_width();
    real scrH = StdGuiRender::screen_height() * lower_scr_bound;

    char *ptr = lines, *str = lines;
    float yOffset = 0;
    int nLines = max_lines;
    int extra_lines = 20;
    if (max_lines < 0)
    {
      nLines = 0;
      const char *n_ptr = strchr(text, '\n');
      while (n_ptr)
      {
        nLines++;
        n_ptr = strchr(n_ptr + 1, '\n');
      }
    }

    int line_cnt = 0, scr_w = int(scrW * 2 / 3);
    // try default 2/3 screen width text area first
    while ((ptr = get_last_fit(str, scr_w, max_lines)) != NULL)
    {
      line_cnt++;
      if (line_cnt > nLines)
        break;
      str = ptr + 1;
      if (*ptr == 0)
        break;
    }

    if (line_cnt > nLines)
    {
      // for long text try maximum 85% screen width text area
      scr_w = int(scrW * 0.85);
      ptr = lines, str = lines;
      line_cnt = 0;
      while ((ptr = get_last_fit(str, scr_w, max_lines)) != NULL)
      {
        line_cnt++;
        if (line_cnt > nLines + extra_lines)
          break;
        str = ptr + 1;
        if (*ptr == 0)
          break;
      }
      // if even 85% is not enough, try to add upto extra_lines additional lines
      if (line_cnt > nLines)
        nLines = min(line_cnt, nLines + extra_lines);
    }

    // adjust to bottom
    if (line_cnt < nLines)
      nLines = min(line_cnt, extra_lines);
    ptr = lines, str = lines;
    line_cnt = 0;
    while ((ptr = get_last_fit(str, scr_w, max_lines)) != NULL)
    {
      bool needBreak = (*ptr == 0);
      if (line_cnt >= nLines)
      {
        // stop rendering text outside safearea to avoid violating TRC/TCR
        // Dront: changed log to assert, because skipping text is ALSO violation of TRC/TCR
        G_ASSERTF(0, "insufficient %d lines, rest text: <%s>", nLines, str);
        break;
      }
      *ptr = 0;

      BBox2 tb = StdGuiRender::get_str_bbox(str);
      Point2 textPos = Point2(scrW / 2 - tb.size()[0] / 2, scrH - tb.size()[1] * nLines * 1.5f);
      textPos[1] += yOffset;

      StdGuiRender::goto_xy(floor(textPos.x), floor(textPos.y));
      StdGuiRender::draw_str(str);

      str = ptr + 1;
      yOffset += tb.size()[1] * 1.5f;
      if (needBreak)
        break;
      line_cnt++;
    }
  }

private:
  struct SubItem
  {
    String text;
    unsigned from, to;
  };
  Tab<SubItem> items;
  bool autoLines;

  static char *get_last_fit(char *str, int width, int max_lines)
  {
    if (!str || !*str)
      return NULL;

    // most time, it's one line
    BBox2 tb = StdGuiRender::get_str_bbox(str);
    if ((tb.size()[0] <= width) && (max_lines >= 0))
      return str + strlen(str);

    char *result = NULL;
    for (char *ptr = str;; ptr++)
    {
      if (*ptr == '\n')
      {
        if (max_lines >= 0)
          *ptr = ' ';
        else
        {
          result = ptr;
          break;
        }
      }
      else if (*ptr == '\r')
        *ptr = ' ';
      if (*ptr == ' ' || *ptr == '\t')
      {
        tb = StdGuiRender::get_str_bbox(str, ptr - str);
        if (tb.size()[0] <= width)
          result = ptr;
        else
          break;
      }
      if (*ptr == 0)
      {
        tb = StdGuiRender::get_str_bbox(str);
        if (tb.size()[0] <= width)
          result = ptr;
        break;
      }
    }
    if (result)
      return result;
    else
      return str + strlen(str);
  }
};
