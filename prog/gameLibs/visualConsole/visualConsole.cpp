// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <visualConsole/dag_visualConsole.h>
#include <util/dag_localization.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <gui/dag_stdGuiRender.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_bounds2.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <EASTL/unique_ptr.h>
#include <stdio.h>
#include <EASTL/array.h>

static eastl::unique_ptr<console::IVisualConsoleDriver> visual_console_driver;
console::IVisualConsoleDriver *setup_visual_console_driver()
{
  console::set_visual_driver(NULL, NULL);
  console::IVisualConsoleDriver *drv;
#if _TARGET_C1 | _TARGET_C2




#else
  drv = new VisualConsoleDriver;
#endif
  visual_console_driver.reset(drv);
  console::set_visual_driver(visual_console_driver.get(), NULL);
  return visual_console_driver.get();
}

void close_visual_console_driver()
{
  console::set_visual_driver(NULL, NULL);
  visual_console_driver.reset();
}

VisualConsoleDriver::VisualConsoleDriver() :
  linesLeft(-1), timeSinceLastSwitch(0), refTime(0), timeSpeedHotKeys(false), prevCursorX(-1)
{
  screenLines = ::dgs_get_settings()->getBlockByNameEx("debug")->getInt("consoleLines", 20);
}

void VisualConsoleDriver::getCommandList(console::CommandList &out_list) { DefaultDagorVisualConsoleDriver::getCommandList(out_list); }

void VisualConsoleDriver::set_cur_height(float ht) { screenLines = (StdGuiRender::screen_height() * ht / lineHeight); }

void VisualConsoleDriver::renderPinnedItems()
{
  const auto &pinnedCommands = console::get_pinned_command_list();
  if (pinnedCommands.empty())
    return;
  StdGuiRender::set_color(E3DCOLOR(0xFF808080));
  StdGuiRender::set_color(E3DCOLOR(console::pinned_commands_background_color));
  StdGuiRender::render_rect(0.0f, screenLines * lineHeight, StdGuiRender::screen_width(), lineHeight * (screenLines + 1));
  StdGuiRender::set_color(E3DCOLOR(0xFF808080));
  StdGuiRender::draw_line(Point2(0, (screenLines)*lineHeight), Point2(StdGuiRender::screen_width(), (screenLines)*lineHeight), 1.0f);
  StdGuiRender::draw_line(Point2(0, (screenLines + 1) * lineHeight),
    Point2(StdGuiRender::screen_width(), (screenLines + 1) * lineHeight), 1.0f);
  StdGuiRender::set_color(E3DCOLOR(0xFFFFFFFF));
  int xOffset = 8;
  for (int i = 0; i < pinnedCommands.size(); ++i)
  {
    if (pinnedCommands[i].empty())
      continue;
    auto msg = String(0, "%d: %s", i, pinnedCommands[i].c_str());
    for (int offset = 1; offset != -1; offset--)
    {
      StdGuiRender::goto_xy(offset + xOffset, int((screenLines + 1 - 0.2f) * lineHeight) + offset);
      StdGuiRender::set_color(offset == 1 ? E3DCOLOR(0, 0, 0) : E3DCOLOR(255, 255, 255));
      StdGuiRender::draw_str(msg);
    }
    for (const char &c : msg)
      xOffset += StdGuiRender::get_char_width_u(c);
    xOffset += 16;
  }
}

bool VisualConsoleDriver::renderTips()
{
  int line_offset = console::get_pinned_command_count() == 0 ? 0 : 1;
  int displayedLines = screenLines + line_offset;
  if (linesLeft < 0)
    linesLeft = (StdGuiRender::screen_height() / lineHeight) - displayedLines;

  if (linesLeft <= 1 || editText.empty())
    return false;

  console::CommandList mlist(framemem_ptr());
  mlist.reserve(linesLeft);
  console::get_filtered_command_list(mlist, editText); // this kinda slow

  maxTipsCount = max<int>(maxTipsCount, mlist.size());
  tipsOutOfScreen = max<int>(tipsOutOfScreen, int(mlist.size()) - tipsScrollPos - console::max_tips);


  String cmdText;
  cmdText.reserve(256);

  if (mlist.size())
    for (int i = tipsScrollPos; i < mlist.size(); ++i)
    {
      if (i - tipsScrollPos >= console::max_tips)
        break;

      console::CommandStruct &cmd = mlist[i];
      cmdText.printf(256, "%s  ", cmd.command.str());

      if (!cmd.argsDescription.empty())
      {
        cmdText += cmd.argsDescription;
      }
      else if (!cmd.varValue.empty())
      {
        cmdText += "(=";
        cmdText += cmd.varValue;
        cmdText += ") ";
      }
      else
      {
        int args = 0;
        for (int k = 1; k < cmd.minArgs; k++)
        {
          cmdText += "<x> ";
          args++;
        }

        for (int k = cmd.minArgs; k < cmd.maxArgs; k++)
        {
          cmdText += "[x] ";
          args++;
          if (args > 12)
          {
            cmdText += "...";
            break;
          }
        }
      }

      if (!cmd.description.empty())
      {
        cmdText += " -- ";
        cmdText += cmd.description;
      }

      for (int offset = 1; offset != -1; offset--)
      {
        StdGuiRender::goto_xy(offset, int((displayedLines + 1 + i - tipsScrollPos - 0.3f) * lineHeight) + offset);
        StdGuiRender::set_color(offset == 1 ? E3DCOLOR(0, 0, 0) : E3DCOLOR(255, 255, 255));
        StdGuiRender::draw_str(cmdText.str());
      }
    }
  return true;
}

void VisualConsoleDriver::renderCursor()
{
  if (!refTime)
    refTime = get_time_msec();

  int cursorX = 1;
  if (editText.str() && strlen(editText.str()) && editPos)
    cursorX = max((int)StdGuiRender::get_str_bbox(editText.str(), editPos).width().x, 1);

  if (cursorX != prevCursorX) // force show cursor when it's position changed
  {
    refTime = 0;
    prevCursorX = cursorX;
  }

  if (uint32_t(get_time_msec() - refTime) % 1024 > 650)
    return; // hide phase

  StdGuiRender::set_color(E3DCOLOR(0xFFFFFFFF));
  StdGuiRender::draw_line(Point2(cursorX, (screenLines - 1) * lineHeight + 2), Point2(cursorX, screenLines * lineHeight - 2), 1);
}

static E3DCOLOR line_type_to_color(console::LineType line_type)
{
  switch (line_type)
  {
    case console::CONSOLE_ERROR: return 0xFFFFA070;
    case console::CONSOLE_WARNING: return 0xFFFFFF50;
    case console::CONSOLE_INFO: return 0xFF60FF20;
    case console::CONSOLE_DEBUG: return 0xFFFFFFFF;
    case console::CONSOLE_TRACE: return 0xFFA8C8FF;
    default: return 0xFFFFFFFF;
  };
}

void VisualConsoleDriver::render()
{
  if (!is_visible())
    return;

  WinAutoLock lock(consoleCritSec);

  int frameBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  {
    StdGuiRender::ScopeStarterOptional strt;

    StdGuiRender::set_font(fontId);

    lineHeight = int(StdGuiRender::get_font_cell_size().y * 1.15) + 1;

    StdGuiRender::set_color(255, 0, 255, 255);
    StdGuiRender::set_ablend(true);
    StdGuiRender::set_draw_str_attr(FFT_NONE, 0, 0, E3DCOLOR(255, 255, 255), 32);
    StdGuiRender::set_color(E3DCOLOR(console::main_background_color));
    StdGuiRender::reset_textures();
    StdGuiRender::set_alpha_blend(NONPREMULTIPLIED);
    StdGuiRender::render_rect(Point2(0.f, 0.f), Point2(StdGuiRender::screen_width(), screenLines * lineHeight));

    if (maxTipsCount > 0)
    {
      int line_offset = console::get_pinned_command_count() == 0 ? 0 : 1;
      int renderTipsCount = min(maxTipsCount - tipsScrollPos, console::max_tips + 1);
      StdGuiRender::set_color(E3DCOLOR(console::tips_background_color));
      StdGuiRender::render_rect(0.0f, (screenLines + line_offset) * lineHeight, StdGuiRender::screen_width(),
        min(float(StdGuiRender::screen_height()), lineHeight * (renderTipsCount + (screenLines + line_offset) + 0.5f)));

      if (tipsOutOfScreen > 0)
      {
        for (int offset = 1; offset != -1; offset--)
        {
          StdGuiRender::goto_xy(offset, float(lineHeight * (renderTipsCount + screenLines)) + offset);
          StdGuiRender::set_color(offset == 1 ? E3DCOLOR(0, 0, 0) : E3DCOLOR(0xFFFFFFFF));
          StdGuiRender::draw_str(String(0, "... %d lines", tipsOutOfScreen).str());
        }
      }
    }

    StdGuiRender::flush_data();

    // render history
    for (int lineNo = 0; lineNo < screenLines; lineNo++)
    {
      int n = lineNo - scrollPos + (int)linesList.size() - screenLines;
      if (n >= 0 && n < int(linesList.size()))
        for (int offset = 1; offset != -1; offset--)
        {
          StdGuiRender::goto_xy(offset, int((lineNo - int(progressIndicators.size()) - 0.3f) * lineHeight) + offset);
          StdGuiRender::set_color(offset == 1 ? E3DCOLOR(0, 0, 0) : line_type_to_color(linesList[n].line_type));
          StdGuiRender::draw_str(linesList[n].line.str());
        }
    }

    // render progress indicators
    for (int i = 0; i < progressIndicators.size(); i++)
    {
      char progress[] = "========";
      progress[abs(get_time_msec()) / 64 % 8] = ' ';
      String titleAndProgress(0, "%s %s", progressIndicators[i].title.str(), progress);
      int n = screenLines - i - 1;
      for (int offset = 1; offset != -1; offset--)
      {
        StdGuiRender::goto_xy(offset, int((n - 0.3f) * lineHeight) + offset);
        StdGuiRender::set_color(offset == 1 ? E3DCOLOR(0, 0, 0) : E3DCOLOR(255, 255, 255));
        StdGuiRender::draw_str(titleAndProgress.str());
      }
    }

    // render double line for edit text
    StdGuiRender::set_color(E3DCOLOR(0xFF808080));
    StdGuiRender::draw_line(Point2(0.f, (screenLines - 1) * lineHeight),
      Point2(StdGuiRender::screen_width(), (screenLines - 1) * lineHeight), 1.0f);
    StdGuiRender::draw_line(Point2(0.f, screenLines * lineHeight), Point2(StdGuiRender::screen_width(), screenLines * lineHeight),
      1.0f);

    // edit text
    for (int offset = 1; offset != -1; offset--)
    {
      StdGuiRender::goto_xy(offset, int((screenLines - 0.3f) * lineHeight) + offset);
      StdGuiRender::set_color(offset == 1 ? E3DCOLOR(0, 0, 0) : E3DCOLOR(255, 255, 255));
      StdGuiRender::draw_str(editText.str());
    }

    constexpr int scrollBarScreenEdgeYOffset = 2;
    const int scrollBarMaxY = (screenLines - 2) * lineHeight - scrollBarScreenEdgeYOffset * 2;

    // important lines
    for (int &line : importantLines)
    {
      constexpr int errorHeight = 1;
      const int errorY = float(scrollBarMaxY) * (1.0f - float(line) / (int(linesList.size()) + 1)) + scrollBarScreenEdgeYOffset;

      StdGuiRender::set_color(235, 140, 82);
      StdGuiRender::reset_textures();
      StdGuiRender::render_rect(StdGuiRender::screen_width() - 4, errorY, StdGuiRender::screen_width() - 2, errorY + errorHeight);
    }

    // scroll bar
    if (linesList.size() > screenLines)
    {
      const int scrollBarHeight = max(3u, (screenLines - 2) * scrollBarMaxY / (linesList.size() + 1));
      const float scrollBarY =
        float(scrollBarMaxY - scrollBarHeight) * (1.0f - float(scrollPos) / (int(linesList.size()) - screenLines + 1)) +
        scrollBarScreenEdgeYOffset;

      StdGuiRender::set_color(90, 90, 90, 180);
      StdGuiRender::reset_textures();
      StdGuiRender::render_rect(StdGuiRender::screen_width() - 6, scrollBarY, StdGuiRender::screen_width() - 2,
        scrollBarY + scrollBarHeight);
    }


    renderCursor();

    maxTipsCount = 0;
    tipsOutOfScreen = 0;

    renderPinnedItems();
    renderTips();
    if (tipsScrollPos > maxTipsCount - 1)
      tipsScrollPos = max(maxTipsCount - 1, 0);

    StdGuiRender::flush_data();
  }
  ShaderGlobal::setBlock(frameBlockId, ShaderGlobal::LAYER_FRAME);
}


void VisualConsoleDriver::init(const char *)
{
  commandsDisplayed = 0;
  commandIndex = 0;
  tipsScrollPos = 0;
  editTextBeforeModify = "";
}
