// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sceneStatus.h"
#include <gui/dag_visualLog.h>
#include <gui/dag_stdGuiRender.h>
#include <perfMon/dag_cpuFreq.h>


namespace darg
{


void SceneStatus::reset()
{
  isShowingError = false;
  isShowingFullErrorDetails = false;
  errorCount = 0;
  errorBeginRenderTimeMsec = 0;
  errorMsg.clear();
  shortErrorString.clear();
}


void SceneStatus::appendErrorText(const char *msg)
{
  isShowingError = true;

  if (msg && errorCount < 50)
  {
    errorCount++;
    int msgLen = strlen(msg);
    const bool hasLineEnding = msgLen > 0 && msg[msgLen - 1] == '\n';
    errorMsg.appendText(msg);

    if (shortErrorString.empty())
    {
      const char *newLine = strchr(msg, '\n');
      if (!newLine)
        shortErrorString = msg;
      else
        shortErrorString = eastl::string(msg, newLine);
    }

    if (!hasLineEnding)
      errorMsg.appendText("\n");
  }


  if (errorRenderMode == SEI_STOP_AND_SHOW_FULL_INFO)
    isShowingFullErrorDetails = true;
}


void SceneStatus::dismissShownError() { isShowingError = false; }


void SceneStatus::toggleShowDetails()
{
  if (errorRenderMode != SEI_STOP_AND_SHOW_FULL_INFO)
  {
    visuallog::clear();
    isShowingFullErrorDetails = true;
    setSceneErrorRenderMode(SEI_STOP_AND_SHOW_FULL_INFO);
  }
  else
  {
    isShowingFullErrorDetails = false;
    setSceneErrorRenderMode(minimalErrorRenderMode);
  }
}


void SceneStatus::setSceneErrorRenderMode(SceneErrorRenderMode mode)
{
  errorRenderMode = mode;

  if (mode != SEI_STOP_AND_SHOW_FULL_INFO)
    minimalErrorRenderMode = mode;
}


void SceneStatus::renderError(StdGuiRender::GuiContext *guiContext)
{
  if (!errorBeginRenderTimeMsec)
    errorBeginRenderTimeMsec = get_time_msec();

  switch (errorRenderMode)
  {
    case SEI_WARNING_ICON:
    {
      guiContext->set_color(220, 0, 0, 255);
      guiContext->reset_textures();
      guiContext->render_rect(guiContext->screen_width() - 8, 0, guiContext->screen_width(), 8);

      int t = get_time_msec() - errorBeginRenderTimeMsec;
      E3DCOLOR frameColor(clamp(768 - t, 0, 255), clamp(512 - t, 0, 255), clamp(255 - t, 0, 255), clamp(768 - t, 0, 255));

      if (frameColor != 0)
      {
        guiContext->set_color(frameColor);
        guiContext->render_frame(0, 0, guiContext->screen_width(), guiContext->screen_height(), 6);
      }
    }
    break;

    case SEI_WARNING_SINGLE_STRING:
    {
      guiContext->set_font(0);
      int ascent = StdGuiRender::get_font_ascent(0);

      guiContext->set_color(80, 0, 0, 200);
      guiContext->reset_textures();
      guiContext->render_rect(0, 0, guiContext->screen_width(), ascent + 10);

      guiContext->set_color(255, 255, 255);
      guiContext->goto_xy(10, 1 + ascent);
      guiContext->draw_str(shortErrorString.c_str(), -1);
    }
    break;

    case SEI_WARNING_MESSAGE:
    {
      guiContext->set_font(0);
      int ascent = StdGuiRender::get_font_ascent(0);

      guiContext->set_color(80, 0, 0, 200);
      guiContext->reset_textures();
      guiContext->render_rect(0, 0, guiContext->screen_width(), 20 + ascent * 3);

      guiContext->set_color(255, 255, 255);
      guiContext->goto_xy(10, 5 + ascent * 2);
      guiContext->draw_str(shortErrorString.c_str(), -1);
      guiContext->goto_xy(10, 10 + ascent * 3);
      guiContext->draw_str("Press Ctrl+Shift+Backspace for detail, Ctrl+Alt+Backspace to close this message.");
    }
    break;

    case SEI_STOP_AND_SHOW_FULL_INFO:
    {
      guiContext->set_color(0, 20, 80, 80);
      guiContext->reset_textures();
      guiContext->render_rect(0, 0, guiContext->screen_width(), guiContext->screen_height());

      guiContext->set_color(255, 255, 200);

      textlayout::FormatParams fp;
      fp.maxWidth = guiContext->screen_width() - 100;
      errorMsg.format(fp);

      float lineSpacing = StdGuiRender::get_font_line_spacing(fp.defFontId);
      const int left = 20, top = 8 * lineSpacing; // skip place for logerrs

      for (int iLine = 0; iLine < errorMsg.lines.size(); ++iLine)
      {
        const textlayout::TextLine &line = errorMsg.lines[iLine];
        for (int iBlock = 0; iBlock < line.blocks.size(); ++iBlock)
        {
          const textlayout::TextBlock *block = line.blocks[iBlock];

          int ascent = StdGuiRender::get_font_ascent(block->fontId);
          guiContext->goto_xy(left + block->position.x, top + block->position.y + ascent);
          guiContext->draw_str(block->text.c_str(), block->text.length());
        }
      }
    }
    break;

    default: break;
  }
}


} // namespace darg
