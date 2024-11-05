// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_visConsole.h>

#include <drv/hid/dag_hiKeybIds.h>
#include <osApiWrappers/dag_unicode.h>
#include <osApiWrappers/dag_clipboard.h>
#include <gui/dag_stdGuiRender.h>
#include <math/dag_bounds2.h>
#include <debug/dag_debug.h>
#include <utf8/utf8.h>

#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiKeyboard.h>

namespace console
{

static void copy_to_clipboard(const char *text) { clipboard::set_clipboard_utf8_text(text); }

static bool paste_from_clipboard(String &edit_text, int &edit_pos)
{
  char buf[1024];
  if (!clipboard::get_clipboard_utf8_text(buf, sizeof(buf)))
    return false;

  for (char *s = buf; s < buf + sizeof(buf) && *s; s++)
    if (*s == '\r' || *s == '\n')
    {
      *s = '\0';
      break;
    }
    else if (*s < ' ')
      *s = ' ';

  edit_text.insert(edit_pos, buf);

  edit_pos += i_strlen(buf);
  if (edit_pos > edit_text.length())
    edit_pos = edit_text.length();

  return true;
}

static bool is_mod_key_pressed(int shift_bits)
{
  if (global_cls_drv_kbd && global_cls_drv_kbd->getDevice(0))
    return global_cls_drv_kbd->getDevice(0)->getRawState().shifts & shift_bits;
  return false;
}

static bool is_separator_char(const char c) { return strchr("!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~", c) != NULL; }

static int get_next_pos(const String &text, int pos, bool is_increment, bool is_jump_word)
{
  int nextPos = pos + (is_increment ? 1 : -1);
  if (nextPos < 0 || nextPos > text.length())
    return pos;

  if (!is_jump_word)
  {
    const char *it = text.str() + pos;
    is_increment ? utf8::next(it) : utf8::prior(it);
    return it - text.str();
  }
  else
  {
    int len = text.length();
    bool isInChain = false;
    bool isSeparatorChain = false;
    int lastStopPos = 0;

    for (int p = 0; p < len; p++)
    {
      if (text[p] == ' ')
      {
        isInChain = false;
        continue;
      }
      bool isChainStart = !isInChain;
      isInChain = true;
      bool isSeparator = is_separator_char(text[p]);
      if (!isChainStart && !isSeparatorChain && isSeparator && !is_separator_char(text[p + 1]))
        isSeparatorChain = true; // to avoid stop before 1-char separator at word end
      if (isChainStart || isSeparator != isSeparatorChain)
      {
        if (p > pos || (!is_increment && p == pos))
          return is_increment ? p : lastStopPos;

        lastStopPos = p;
        isSeparatorChain = isSeparator;
      }
    }

    return is_increment ? len : lastStopPos;
  }
}


DefaultDagorVisualConsoleDriver::DefaultDagorVisualConsoleDriver() :
  handleInputButton(HumanInput::DKEY_GRAVE),
  isVisible(false),
  screenLines(20),
  bufferLines(10000),
  lineHeight((int)(max(10.0, 20.0f * StdGuiRender::screen_height() / 1080.0))),
  linesList(midmem),
  editPos(0),
  tipsScrollPos(0),
  maxTipsCount(0),
  tipsOutOfScreen(0),
  commandsDisplayed(0),
  commandIndex(0)
{
  puts("F1 - console usage help\n", CONSOLE_TRACE);
}

void DefaultDagorVisualConsoleDriver::printHelp()
{
  puts("~ - hide console\n"
       "Up/Down - recall console command\n"
       "Tab - complete command\n"
       "Ctrl + V - paste from clipboard\n"
       "Ctrl + C - copy command line to clipboard\n"
       "Ctrl + Shift + C - copy console text to clipboard\n"
       "Ctrl + U, Esc - clear command line\n"
       "Alt + 1,2,3,4,5 - go to errors\n"
       "Alt + Up/Down - prev/next error\n"
       "Alt + ~ - scroll to end\n"
       "PgUp/PgDown - scroll\n"
       "Ctrl + PgUp/PgDown - fast scroll\n"
       "Alt + PgUp/PgDown - change console size\n"
       "Shift + PgUp/PgDown - scroll tips\n"
       "Ctrl + F - pin last command\n"
       "Ctrl + <number> - execute pinned command\n"
       "Ctrl + Shift + <number> - unpin command\n",
    CONSOLE_TRACE);
}

DefaultDagorVisualConsoleDriver::~DefaultDagorVisualConsoleDriver() {}

void DefaultDagorVisualConsoleDriver::onCommandModified()
{
  modified = true;
  tipsScrollPos = 0;
}


bool DefaultDagorVisualConsoleDriver::processKey(int btn_id, int wchar, bool handle_tilde)
{
  int ctrlBits = HumanInput::KeyboardRawState::CTRL_BITS;
  int shiftBits = HumanInput::KeyboardRawState::SHIFT_BITS;
  int altBits = HumanInput::KeyboardRawState::ALT_BITS;

  controlPressed = is_mod_key_pressed(ctrlBits);
  shiftPressed = is_mod_key_pressed(shiftBits);
  altPressed = is_mod_key_pressed(altBits);

  bool modifierPressed = controlPressed || shiftPressed || altPressed;

  if (handle_tilde && btn_id == handleInputButton && !modifierPressed)
  {
    if (console::is_visible())
      console::hide();
    else
    {
      console::show();
      progressIndicators.clear();
    }
    return true;
  }

  if (!is_visible())
    return false;

  auto onDefKey = [&]() {
    char buf[5];
    char converted = *wchar_to_utf8(wchar, buf, 4);
    // kbd->convertToChar(btn_idx, true);
    if (converted)
    {
      onCommandModified();
      editText.insert(editPos, buf);
      editPos += i_strlen(buf);
      if (editPos > editText.length())
        editPos = editText.length();
    }
  };

  G_STATIC_ASSERT(HumanInput::DKEY_0 - HumanInput::DKEY_1 == 9);
  const bool isNumberPressed = btn_id >= HumanInput::DKEY_1 && btn_id <= HumanInput::DKEY_0;
  const unsigned pressedNumber = isNumberPressed ? (btn_id == HumanInput::DKEY_0 ? 0 : btn_id - HumanInput::DKEY_1 + 1) : 0;

  if (btn_id == HumanInput::DKEY_V && controlPressed)
  {
    if (paste_from_clipboard(editText, editPos))
      onCommandModified();
  }
  if (btn_id == HumanInput::DKEY_F && controlPressed)
  {
    // Pin command
    const char *cmd = top_history_command();
    if (cmd && *cmd)
    {
      console::add_pinned_command(cmd);
      console::print("Command pinned: %s", cmd);
    }
    else
      console::print("No command in history to pin");
  }
  if (isNumberPressed && controlPressed && shiftPressed)
  {
    // Unpin command
    const auto &pinnedCommands = console::get_pinned_command_list();
    if (pressedNumber < pinnedCommands.size() && !pinnedCommands[pressedNumber].empty())
      console::remove_pinned_command(pinnedCommands[pressedNumber]);
    else
      console::print("There is no pinned command with id: %d", pressedNumber);
  }
  if (isNumberPressed && controlPressed && !shiftPressed)
  {
    // Run pinned command
    const auto &pinnedCommands = console::get_pinned_command_list();
    if (pressedNumber < pinnedCommands.size() && !pinnedCommands[pressedNumber].empty())
    {
      console::print("Executing pinned command: %s", pinnedCommands[pressedNumber]);
      console::add_history_command(pinnedCommands[pressedNumber]);
      console::command(pinnedCommands[pressedNumber]);
    }
    else
      console::print("There is no pinned command with id: %d", pressedNumber);
  }
  else if (btn_id == HumanInput::DKEY_C && controlPressed)
  {
    if (shiftPressed)
    {
      String buf;
      for (const ConsoleLine &v : linesList)
      {
        buf += v.line.str();
        buf += "\n";
      }
      copy_to_clipboard(buf);
    }
    else
      copy_to_clipboard(editText);
  }
  else
    switch (btn_id)
    {
      case HumanInput::DKEY_LEFT:
        if (!altPressed)
          editPos = get_next_pos(editText, editPos, false, controlPressed);
        break;

      case HumanInput::DKEY_RIGHT:
        if (!altPressed)
          editPos = get_next_pos(editText, editPos, true, controlPressed);
        break;

      case HumanInput::DKEY_A:
        if (controlPressed)
          ; // fallthrough
        else
        {
          onDefKey();
          break;
        }
      case HumanInput::DKEY_HOME:
        if (!modifierPressed)
          editPos = 0;
        break;

      case HumanInput::DKEY_F1:
        if (!modifierPressed)
          printHelp();
        break;

      case HumanInput::DKEY_E:
        if (controlPressed)
          ; // fallthrough
        else
        {
          onDefKey();
          break;
        }
      case HumanInput::DKEY_END:
        if (!modifierPressed)
          editPos = editText.length();
        break;

      case HumanInput::DKEY_H:
        if (controlPressed)
          ; // fallthrough
        else
        {
          onDefKey();
          break;
        }
      case HumanInput::DKEY_BACK:
        if (editPos > 0)
        {
          int oldPos = editPos;
          editPos = get_next_pos(editText, editPos, false, controlPressed);
          erase_items(editText, editPos, oldPos - editPos);
          onCommandModified();
        }
        break;

      case HumanInput::DKEY_DELETE:
        if (editPos < editText.length())
        {
          int cutToPos = get_next_pos(editText, editPos, true, controlPressed);
          erase_items(editText, editPos, cutToPos - editPos);
          onCommandModified();
        }
        break;

      case HumanInput::DKEY_U:
      {
        if (controlPressed)
        {
          erase_items(editText, 0, editPos);
          editPos = 0;
          onCommandModified();
        }
        else
          onDefKey();
      }
      break;

      case HumanInput::DKEY_UP:
        if (!modifierPressed)
        {
          onCommandModified();
          editText = console::get_prev_command();
          editPos = editText.length();
        }
        else if (altPressed)
        {
          int newTopLinePos = scrollPos + screenLines - 1;
          for (int i = importantLines.size() - 1; i >= 0; i--)
            if (importantLines[i] > newTopLinePos)
            {
              newTopLinePos = importantLines[i];
              break;
            }

          scrollPos = newTopLinePos - screenLines + 1;
        }
        break;

      case HumanInput::DKEY_DOWN:
        if (!modifierPressed)
        {
          onCommandModified();
          editText = console::get_next_command();
          editPos = editText.length();
        }
        else if (altPressed)
        {
          int newTopLinePos = scrollPos + screenLines - 1;
          for (int i = 0; i < importantLines.size(); i++)
            if (importantLines[i] < newTopLinePos)
            {
              newTopLinePos = importantLines[i];
              break;
            }

          scrollPos = max(0, newTopLinePos - screenLines + 1);
        }
        break;

      case HumanInput::DKEY_J:
        if (controlPressed)
          ; // fallthrough
        else
        {
          onDefKey();
          break;
        }
      case HumanInput::DKEY_RETURN:
      case HumanInput::DKEY_NUMPADENTER:
      {
        scrollPos = 0;
        console::trace(editText);
        console::add_history_command(editText.str());
        onCommandModified();
        editPos = 0;
        // clear 'editText' before calling command as it might call 'processKey' again
        String tmpEditText;
        tmpEditText.swap(editText);
        console::command(tmpEditText.str());
        editText.swap(tmpEditText); // swap it back to preserve allocated memory
        editText = "";
        editTextBeforeModify = "";
      }
      break;

      case HumanInput::DKEY_ESCAPE:
        editText = "";
        editPos = 0;
        onCommandModified();
        break;

      case HumanInput::DKEY_1:
      case HumanInput::DKEY_2:
      case HumanInput::DKEY_3:
      case HumanInput::DKEY_4:
      case HumanInput::DKEY_5:
      {
        if (altPressed)
        {
          int idx = min(btn_id - HumanInput::DKEY_1, int(importantLines.size()) - 1);
          if (idx >= 0)
            scrollPos = max(importantLines[idx] - screenLines + 1, 0);
        }
        else
          onDefKey();
      }
      break;

      case HumanInput::DKEY_GRAVE:
        if (altPressed)
          scrollPos = 0;
        else
          onDefKey();
        break;

      case HumanInput::DKEY_PRIOR:
      case HumanInput::DKEY_NUMPAD9:
        if (!wchar)
        {
          if (shiftPressed)
          {
            tipsScrollPos = max(tipsScrollPos - 2, 0);
          }
          else if (!altPressed)
          {
            int step = (controlPressed && screenLines > 2) ? screenLines - 1 : 1;
            int limit = max<int>(linesList.size() - screenLines + 1, 0);
            scrollPos = min(scrollPos + step, limit);
          }
          else // if (altPressed)
          {
            screenLines = max(screenLines - 2, 2);
          }
        }
        else
          onDefKey();
        break;

      case HumanInput::DKEY_NEXT:
      case HumanInput::DKEY_NUMPAD3:
        if (!wchar)
        {
          if (shiftPressed)
          {
            tipsScrollPos = max(tipsScrollPos + 2, 0);
          }
          else if (!altPressed)
          {
            int step = (controlPressed && screenLines > 2) ? screenLines - 1 : 1;
            scrollPos = max(scrollPos - step, 0);
          }
          else // if (altPressed)
          {
            screenLines = min(screenLines + 2, int(StdGuiRender::screen_height() / lineHeight - 10));
          }
        }
        else
          onDefKey();
        break;

      case HumanInput::DKEY_INSERT:
        if (is_mod_key_pressed(HumanInput::KeyboardRawState::SHIFT_BITS))
        {
          if (paste_from_clipboard(editText, editPos))
            onCommandModified();
        }
        else if (is_mod_key_pressed(HumanInput::KeyboardRawState::CTRL_BITS))
          copy_to_clipboard(editText);
        break;

      case HumanInput::DKEY_TAB:
      {
        if (modified)
        {
          editTextBeforeModify = editText;
          modified = false;
          commandIndex = 0;
        }

        console::CommandList commands(midmem);
        getFilteredCommandList(commands, editTextBeforeModify);

        if (commands.size())
        {
          if (!controlPressed)
          {
            if (shiftPressed)
              commandIndex = (commandIndex + 2 * commands.size() - 2) % commands.size();

            editText = commands[commandIndex].str();
            editPos = editText.length();

            commandIndex = (commandIndex + 1) % commands.size();
          }


          if (controlPressed)
          {

            if (commands.size() < screenLines - 1)
            {
              commandsDisplayed = 0;
              console::print("");
            }

            if (prevTabText != editTextBeforeModify)
              commandsDisplayed = 0;

            int count = 0;
            for (int i = 0; i < commands.size(); i++)
              if (i >= commandsDisplayed)
              {
                String text(commands[i].str());
                text += "   ";

                if (!commands[i].varValue.empty())
                {
                  text += "(";
                  text += commands[i].varValue.str();
                  text += ") ";
                }
                else
                {
                  for (int k = 1; k < commands[i].minArgs; k++)
                    text += "x ";

                  if (commands[i].minArgs != commands[i].maxArgs)
                  {
                    text += "[";
                    for (int k = commands[i].minArgs; k < commands[i].maxArgs; k++)
                      if (k != commands[i].maxArgs - 1)
                        text += "x ";
                      else
                        text += "x";
                    text += "]";
                  }
                }


                puts(text);
                commandsDisplayed++;
                count++;

                if (count >= screenLines - 2)
                  break;
              }

            if (commandsDisplayed >= commands.size())
            {
              commandsDisplayed = 0;
              console::print("");
            }
          }
        }

        prevTabText = editTextBeforeModify;
      }
      break;

      default:
      {
        onDefKey();
      }
    }

  return true;
}

void DefaultDagorVisualConsoleDriver::reset_important_lines()
{
  WinAutoLock lock(consoleCritSec);
  importantLines.clear();
}

void DefaultDagorVisualConsoleDriver::puts(const char *str, console::LineType lineType)
{
  WinAutoLock lock(consoleCritSec);

  constexpr int maxImportantLinesCount = 50;
  constexpr int removeImportantLinesFrom = 15;
  if (lineType == CONSOLE_ERROR)
  {
    if (importantLines.size() < maxImportantLinesCount)
    {
      importantLines.push_back(0);
    }
    else
    {
      importantLines.erase(importantLines.begin() + removeImportantLinesFrom);
      importantLines.push_back(0);
    }
  }

  String line(str);
  line.replaceAll("\r", "");
  line.replaceAll("\t", "    ");
  int prevLinesCount = linesList.size();

  while (char *cr = (char *)line.find('\n'))
  {
    *cr = 0;
    if (cr != line.str())
      linesList.push_back({lineType, line});
    erase_items(line, 0, cr - line.str() + 1);
  }
  if (line.length() > 0)
    linesList.push_back({lineType, line});

  for (int &importantLine : importantLines)
    importantLine += linesList.size() - prevLinesCount;

  if (bufferLines > 0 && linesList.size() > bufferLines + 100)
  {
    erase_items(linesList, 0, linesList.size() - (bufferLines - 1));
    for (int i = importantLines.size() - 1; i >= 0; i--)
      if (importantLines[i] >= linesList.size())
        importantLines.erase(importantLines.begin() + i);
  }

  if (scrollPos > 0)
    scrollPos += linesList.size() - prevLinesCount;
}

void DefaultDagorVisualConsoleDriver::render()
{
  if (!is_visible())
    return;

  WinAutoLock lock(consoleCritSec);

  lineHeight = (int)(max(10.0, 20.0f * StdGuiRender::screen_height() / 1080.0));
  StdGuiRender::start_render();


  const GuiViewPort &vp = StdGuiRender::get_viewport();

  StdGuiRender::reset_textures();
  StdGuiRender::set_color(0, 32, 64, 128);
  StdGuiRender::render_rect(0, 0, vp.rightBottom.x, screenLines * lineHeight);

  StdGuiRender::set_font(0);
  StdGuiRender::set_color(255, 255, 255);


  for (int lineNo = 0; lineNo < screenLines; lineNo++)
  {
    int n = lineNo - scrollPos + (int)linesList.size() - screenLines;
    if (n >= 0 && n < linesList.size())
      for (int offset = 1; offset != -1; offset--)
      {
        StdGuiRender::goto_xy(0, (screenLines - linesList.size() - lineNo + scrollPos - 0.3f) * lineHeight);
        StdGuiRender::draw_str(linesList[n].line.str(), linesList[n].line.length());
      }
  }

  // edit box area
  StdGuiRender::set_color(100, 160, 200, 255);
  float lineY = (screenLines - 1) * lineHeight;
  StdGuiRender::draw_line(0, lineY, vp.rightBottom.x, lineY);
  lineY += lineHeight;
  StdGuiRender::draw_line(0, lineY, vp.rightBottom.x, lineY);

  // edit box text
  StdGuiRender::set_color(255, 255, 255);
  StdGuiRender::goto_xy(0, (screenLines - 0.3f) * lineHeight);
  StdGuiRender::draw_str(editText.str(), editText.length());

  int cursorX = 1;

  if (editText.length() && editPos)
  {
    BBox2 strBox = StdGuiRender::get_str_bbox(editText.str(), editPos);
    if (!strBox.isempty())
      cursorX = (int)strBox.width().x;
  }

  if (cursorX < 1)
    cursorX = 1;

  StdGuiRender::set_color(255, 255, 255);
  StdGuiRender::draw_line(cursorX, (screenLines - 1) * lineHeight + 2, cursorX, screenLines * lineHeight - 2);

  StdGuiRender::end_render();
}

void DefaultDagorVisualConsoleDriver::getCommandList(console::CommandList &out_list) { console::get_command_list(out_list); }

void DefaultDagorVisualConsoleDriver::getFilteredCommandList(console::CommandList &out_list, const String &prefix)
{
  console::get_filtered_command_list(out_list, prefix);
}

void DefaultDagorVisualConsoleDriver::set_progress_indicator(const char *id, const char *title)
{
  WinAutoLock lock(consoleCritSec);

  if (!linesList.empty() && strstr(linesList.back().line.str(), "console.progress_indicator ") == linesList.back().line.str())
    linesList.pop_back();

  for (int i = 0; i < progressIndicators.size(); i++)
    if (!strcmp(id, progressIndicators[i].indicatorId.str()))
    {
      if (!title || !title[0])
      {
        String buf(0, "%s - Done\n", progressIndicators[i].title.str());
        puts(buf.str());
        erase_items(progressIndicators, i, 1);
        return;
      }
      progressIndicators[i].title.setStr(title);
      return;
    }

  if (title && title[0])
    progressIndicators.push_back(ConsoleProgressIndicator{String(id), String(title)});
}


} // namespace console

#if !(_TARGET_C1 | _TARGET_C2)
console::IVisualConsoleDriver *console::create_def_console_driver(bool /*allow_tty*/) { return new DefaultDagorVisualConsoleDriver; }
#endif
