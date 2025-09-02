// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "formattedTextHandler.h"

#include <generic/dag_tab.h>
#include <coolConsole/coolConsole.h>
#include <generic/dag_sort.h>
#include <propPanel/colors.h>
#include <util/dag_console.h>
#include <util/dag_delayedAction.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_resetDevice.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <util/dag_globDef.h>
#include <workCycle/dag_workCycle.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_unicode.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>

#include <libTools/util/hash.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <limits.h>

#include <stdio.h>

#define ABOUT_MESSAGE "CoolConsole version 1.3"

#define DAGOR_IDLE_CYCLE_CALL_TIMEOUT 1000

// #define GET_CMD_HASH(cmd) ::get_hash(_strlwr(cmd), 0)

#define MAKE_CMD_HASH(cmd, hash) \
  String temp(cmd);              \
  hash = GET_CMD_HASH(temp.begin());


static FormattedTextHandler formatted_text_handler;
static bool destroyed = false;
static bool visible_console_exists = false;

static void call_idle_cycle_seldom(bool important = false)
{
  if (destroyed || is_restoring_3d_device())
    return;
  static int lastDagorIdleCall = 0;

  int msec = get_time_msec();
  if (msec - lastDagorIdleCall > (important ? 50 : DAGOR_IDLE_CYCLE_CALL_TIMEOUT))
  {
    lastDagorIdleCall = msec;
    ::dagor_idle_cycle();

    // We have to call rendering to make messages added during a long running operation visible to the user. But we
    // cannot call rendering if we are in an ImGui frame. (So long running operations should be started from delayed
    // messages of PropPanel. This is generally true as button clicks and other common events are sent with delayed
    // messages.)
    const ImGuiContext *context = ImGui::GetCurrentContext();
    if (visible_console_exists && (!context || !context->WithinFrameScope) && d3d::is_inited())
    {
      // We intentionally not use dagor_work_cycle() here because that calls act(), which we do not want, it could change
      // the behavior of the code compared to the non-ImGui-based CoolConsole.
      d3d::GpuAutoLock gpuLock;
      dagor_work_cycle_flush_pending_frame();
      dagor_draw_scene_and_gui(true, true);
      d3d::update_screen();
    }
  }
}

static char *strtokLastToken = NULL;

// strtok analog excluding token-delimited qouted strings from tokens search
// for example string 'command "spaced parameter1" parameter2' will be split to
//'command', 'spaced parameter1', 'parameter2'
//==============================================================================
static char *conSttrok(char *str, const char *delim)
{
  if (!str)
    str = strtokLastToken;

  if (str)
  {
    if (*str == '"')
    {
      char *c = strchr(str + 1, '"');

      if (c)
      {
        bool quotedStr = false;

        if (!*(c + 1))
          quotedStr = true;
        else
          for (const char *tok = delim; *tok; ++tok)
            if (*(c + 1) == *tok)
            {
              quotedStr = true;
              break;
            }

        if (quotedStr)
        {
          *c = 0;
          strtokLastToken = c + 1;

          return str + 1;
        }
      }
    }

    for (char *c = str; *c; ++c)
    {
      for (const char *tok = delim; *tok; ++tok)
        if (*c == *tok)
        {
          *c = 0;
          strtokLastToken = c + 1;
          return str;
        }
    }

    strtokLastToken = NULL;
    return str;
  }

  return NULL;
}

//==============================================================================
CoolConsole::CoolConsole() :
  commands(midmem),
  cmdNames(midmem),
  progressCb(NULL),
  cmdHistoryLastUnfinished(false),
  cmdHistoryPos(0),
  lastVisible(false),
  notesCnt(0),
  warningsCnt(0),
  errorsCnt(0),
  fatalsCnt(0),
  globWarnCnt(0),
  globErrCnt(0),
  globFatalCnt(0)
{
  flags = CC_CAN_CLOSE | CC_CAN_MINIMIZE;

  registerCommand("help", this);
  registerCommand("list", this);
  registerCommand("clear", this);
  console::set_visual_driver_raw(this);
}


//==============================================================================
CoolConsole::~CoolConsole()
{
  console::set_visual_driver_raw(nullptr);
  PropPanel::remove_delayed_callback(*this);
  destroyConsole();
}


//==============================================================================
int CoolConsole::compareCmd(const String *s1, const String *s2) { return stricmp((const char *)*s1, (const char *)*s2); }


String CoolConsole::onCmdArrowKey(bool up, const char *current_command)
{
  if (up && cmdHistoryPos > 0)
  {
    if (cmdHistoryPos == cmdHistory.size() && !cmdHistoryLastUnfinished && current_command && *current_command)
    {
      cmdHistory.push_back() = current_command;
      cmdHistoryLastUnfinished = true;
    }
    cmdHistoryPos--;
    return cmdHistory[cmdHistoryPos];
  }
  else if (!up && cmdHistoryPos < cmdHistory.size())
  {
    cmdHistoryPos++;

    String command;
    if (cmdHistoryPos < cmdHistory.size())
      command = cmdHistory[cmdHistoryPos];

    if (cmdHistoryLastUnfinished && cmdHistoryPos + 1 == cmdHistory.size())
    {
      cmdHistoryLastUnfinished = false;
      cmdHistory.pop_back();
      cmdHistoryPos = cmdHistory.size();
    }

    return command;
  }

  return String();
}
void CoolConsole::saveCmdHistory(DataBlock &blk) const
{
  blk.removeBlock("cmdHistory");
  DataBlock &b = *blk.addBlock("cmdHistory");
  for (int i = 0; i < cmdHistory.size(); i++)
    b.addStr("cmd", cmdHistory[i]);
}
void CoolConsole::loadCmdHistory(const DataBlock &blk)
{
  const DataBlock &b = *blk.getBlockByNameEx("cmdHistory");
  cmdHistory.clear();
  cmdHistoryLastUnfinished = false;

  int nid = blk.getNameId("cmd");
  for (int i = 0; i < b.paramCount(); i++)
    if (b.getParamNameId(i) == nid && b.getParamType(i) == b.TYPE_STRING)
      cmdHistory.push_back() = b.getStr(i);
  cmdHistoryPos = cmdHistory.size();
}


//==============================================================================
bool CoolConsole::isVisible() const { return visible; }


//==============================================================================
void CoolConsole::addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!fmt || !*fmt)
    return;

  // Prevent logmessage_fmt to be redirected here.
  static bool entranceGuard = false;
  if (entranceGuard)
    return;
  entranceGuard = true;

  unsigned color = PropPanel::ColorOverride::CONSOLE_LOG_TEXT;
  bool bold = false;
  String con_fmt(0, "CON: %s", fmt);

  switch (type)
  {
    case REMARK:
      color = PropPanel::ColorOverride::CONSOLE_LOG_REMARK_TEXT;
      if (isCountMessages())
        ++notesCnt;
      break;

    case WARNING:
      ++globWarnCnt;
      color = PropPanel::ColorOverride::CONSOLE_LOG_WARNING_TEXT;
      if (isCountMessages())
        ++warningsCnt;
      logmessage_fmt(LOGLEVEL_WARN, con_fmt, arg, anum);
      break;

    case ERROR:
      ++globErrCnt;
      color = PropPanel::ColorOverride::CONSOLE_LOG_ERROR_TEXT;
      if (isCountMessages())
        ++errorsCnt;
      logmessage_fmt(LOGLEVEL_ERR, con_fmt, arg, anum);
      break;

    case FATAL:
      ++globFatalCnt;
      color = PropPanel::ColorOverride::CONSOLE_LOG_FATAL_TEXT;
      bold = true;
      if (isCountMessages())
        ++fatalsCnt;
      logmessage_fmt(LOGLEVEL_ERR, con_fmt, arg, anum);
      break;

    default:
      logmessage_fmt(LOGLEVEL_DEBUG, con_fmt, arg, anum);
      if (isCountMessages())
        ++notesCnt;
  }

  entranceGuard = false;
  String msg;

  if (!is_main_thread())
  {
    msg.vprintf(0, String::mk_str_cat("*", fmt), arg, anum);
    delayed_call([this, msg = eastl::move(msg), color, bold]() { addTextToLog(msg, color, bold); });
    return;
  }

  msg.vprintf(0, fmt, arg, anum);
  addTextToLog(msg, color, bold);

  call_idle_cycle_seldom();
}
void CoolConsole::puts(const char *str, console::LineType type)
{
  if (limitOutputLinesLeft == 0)
    return;

  unsigned color = PropPanel::ColorOverride::CONSOLE_LOG_TEXT;
  switch (type)
  {
    case console::CONSOLE_INFO: color = PropPanel::ColorOverride::CONSOLE_LOG_REMARK_TEXT; break;
    case console::CONSOLE_WARNING: color = PropPanel::ColorOverride::CONSOLE_LOG_WARNING_TEXT; break;
    case console::CONSOLE_ERROR: color = PropPanel::ColorOverride::CONSOLE_LOG_ERROR_TEXT; break;

    // to prevent the unhandled switch case error
    case console::CONSOLE_DEBUG:
    case console::CONSOLE_TRACE:
    case console::CONSOLE_DEFAULT: break;
  }

  addTextToLog(str, color);
  if (--limitOutputLinesLeft == 0)
    addTextToLog("... too many lines, output omitted (see debug for full log)...\n",
      PropPanel::ColorOverride::CONSOLE_LOG_WARNING_TEXT);
}

//==============================================================================
bool CoolConsole::initConsole()
{
  G_ASSERT(!destroyed);
  addMessage(REMARK, ABOUT_MESSAGE);
  return true;
}


//==============================================================================
void CoolConsole::destroyConsole() { destroyed = true; }


//==============================================================================
void CoolConsole::showConsole(bool activate)
{
  activateOnShow = activate;
  scrollToBottomOnShow = true;
  visible = visible_console_exists = true;
  justMadeVisible = true;
}


//==============================================================================
void CoolConsole::hideConsole() { visible = visible_console_exists = false; }


//==============================================================================
void CoolConsole::clearConsole() { formatted_text_handler.clear(); }

void CoolConsole::saveToFile(const char *file_name) const
{
  FILE *fp = fopen(file_name, "wb");
  if (fp)
  {
    const String &text = formatted_text_handler.getAsString();
    fwrite(text.c_str(), text.length(), 1, fp);
    fclose(fp);
  }
  else
  {
    wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Save log error", "Could not save log to file \"%s\".", file_name);
  }
}


//==============================================================================
bool CoolConsole::runCommand(const char *cmd)
{
  String command(cmd, tmpmem);
  if (command.empty())
    return false;

  if (cmdHistoryLastUnfinished)
    cmdHistory.pop_back();
  for (int i = cmdHistory.size() - 1; i >= 0; i--)
    if (strcmp(cmdHistory[i], command) == 0)
      erase_items(cmdHistory, i, 1);
  cmdHistory.push_back() = command;
  cmdHistoryLastUnfinished = false;
  cmdHistoryPos = cmdHistory.size();

  if (strcmp(command, "clrhist") == 0)
  {
    cmdHistory.clear();
    cmdHistoryLastUnfinished = false;
    cmdHistoryPos = cmdHistory.size();
    addMessage(NOTE, "command history cleared!");
    return true;
  }
  Tab<const char *> params(tmpmem);
  const char *cmdName = NULL;

  const char *token = ::conSttrok(command, " \t");

  while (token)
  {
    if (*token)
    {
      if (cmdName)
        params.push_back(token);
      else
        cmdName = token;
    }

    token = ::conSttrok(NULL, " \t");
  }

  if (!cmdName)
    return false;

  addTextToLog(cmdName, PropPanel::ColorOverride::CONSOLE_LOG_COMMAND_TEXT, true);

  for (int i = 0; i < params.size(); ++i)
  {
    addTextToLog(" ", PropPanel::ColorOverride::CONSOLE_LOG_PARAMETER_TEXT, true, false);
    addTextToLog(params[i], PropPanel::ColorOverride::CONSOLE_LOG_PARAMETER_TEXT, true, false);
  }

  IConsoleCmd *handler = NULL;
  commands.get(String(cmdName), handler);

  limitOutputLinesLeft = 1024;
  if (handler && handler->onConsoleCommand(cmdName, params))
  {
    limitOutputLinesLeft = -1;
    return true;
  }
  for (auto &p : params)
    command.aprintf(0, " %s", p);
  if (console::command(command.c_str()))
  {
    limitOutputLinesLeft = -1;
    return true;
  }
  addTextToLog(String(32, "Unknown command \"%s\"", command.c_str()), PropPanel::ColorOverride::CONSOLE_LOG_ERROR_TEXT);
  limitOutputLinesLeft = -1;
  return false;
}


//==============================================================================
void CoolConsole::runHelp(const char *command)
{
  IConsoleCmd *handler = NULL;

  if (commands.get(String(command), handler))
  {
    const char *help = handler->onConsoleCommandHelp(command);
    if (help)
    {
      addTextToLog(help, PropPanel::ColorOverride::CONSOLE_LOG_TEXT);
      return;
    }
  }

  addTextToLog(String(32, "Unknown command '%s'", command), PropPanel::ColorOverride::CONSOLE_LOG_ERROR_TEXT);
}


//==============================================================================
void CoolConsole::runCmdList()
{
  addTextToLog("Registered commands:", PropPanel::ColorOverride::CONSOLE_LOG_TEXT);

  for (int i = 0; i < cmdNames.size(); ++i)
    addTextToLog(cmdNames[i], PropPanel::ColorOverride::CONSOLE_LOG_TEXT);

  console::CommandList mlist;
  console::get_command_list(mlist);
  char cmdText[384];
  debug("mlist.size() = %d", mlist.size());
  for (int i = 0; i < mlist.size(); ++i)
  {
    const console::CommandStruct &cmd = mlist[i];
    _snprintf(cmdText, sizeof(cmdText), "%s  ", cmd.command.str());
    cmdText[sizeof(cmdText) - 1] = 0;

    if (!cmd.argsDescription.empty())
    {
      int argDescLen = cmd.argsDescription.length();
      int leftFreeSpace = sizeof(cmdText) - strlen(cmdText) - 1;
      strncat(cmdText, cmd.argsDescription, min(argDescLen, leftFreeSpace));
    }
    else
    {
      for (int k = 1; k < cmd.minArgs; k++)
        strncat(cmdText, "<x> ", min(4, int(sizeof(cmdText) - strlen(cmdText) - 1)));
      for (int k = cmd.minArgs; k < cmd.maxArgs; k++)
        strncat(cmdText, "[x] ", min(4, int(sizeof(cmdText) - strlen(cmdText) - 1)));
    }
    if (cmd.description.length())
    {
      const char *separator = " -- ";
      int sepLen = strlen(separator);
      const char *clipEnd = "...";
      int clipLen = strlen(clipEnd);

      int descLen = cmd.description.length();
      int textLen = strlen(cmdText);
      int freeCells = sizeof(cmdText) - textLen - 1;

      if (freeCells >= sepLen + descLen)
      {
        strncat(cmdText, separator, sepLen);
        strncat(cmdText, cmd.description, descLen);
      }
      else if (freeCells > sepLen + clipLen)
      {
        strncat(cmdText, separator, sepLen);
        int clippedLen = freeCells - sepLen - clipLen;
        strncat(cmdText, cmd.description.c_str(), clippedLen);
        strncat(cmdText, clipEnd, clipLen);
      }
    }

    addTextToLog(cmdText, PropPanel::ColorOverride::CONSOLE_LOG_TEXT);
  }
}


//==============================================================================
void CoolConsole::addTextToLog(const char *text, int color_index, bool bold, bool start_new_line)
{
  ImFont *font = bold ? imgui_get_bold_font() : nullptr;
  formatted_text_handler.addText(text, color_index, font, start_new_line);
}


//==============================================================================
void CoolConsole::startProgress(IProgressCB *progress_cb)
{
  progressCb = progress_cb;

  if (!isProgress())
  {
    lastVisible = isVisible();
    flags |= CC_PROGRESS;
  }

  showConsole(/*activate = */ false);
  call_idle_cycle_seldom(true);
}


//==============================================================================
void CoolConsole::endProgress()
{
  flags &= ~CC_PROGRESS;

  progressCb = nullptr;
  progressPosition = 0;
  progressMaxPosition = 0;

  if (!lastVisible)
    hideConsole();

  call_idle_cycle_seldom(true);
}


//==============================================================================
bool CoolConsole::registerCommand(const char *cmd, IConsoleCmd *handler)
{
  if (!cmd || !*cmd || !handler)
    return false;

  String command(cmd);
  dd_strlwr((char *)command);

  const bool ret = commands.add(command, handler);

  if (ret)
  {
    cmdNames.push_back(command);
    sort(cmdNames, &compareCmd);
  }
  else
    debug("[CoolConsole] Couldn't register command \"%s\" due to hash collision.", cmd);

  return ret;
}


//==============================================================================
bool CoolConsole::unregisterCommand(const char *cmd, IConsoleCmd *handler)
{
  if (!cmd || !*cmd)
    return false;

  IConsoleCmd *regHandler = NULL;
  String s_cmd(cmd);

  if (commands.get(s_cmd, regHandler))
  {
    if (regHandler == handler)
    {
      commands.erase(s_cmd);
      for (int i = 0; i < cmdNames.size(); ++i)
        if (!stricmp(cmdNames[i], cmd))
        {
          erase_items(cmdNames, i, 1);
          break;
        }
    }
    else
    {
      debug("[CoolConsole] Couldn't unregister command \"%s\" due to handlers collision.", cmd);
      return false;
    }
  }

  return true;
}


//==============================================================================
bool CoolConsole::onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params)
{
  if (!stricmp("help", cmd))
  {
    runHelp(params.size() ? params[0] : "help");
    return true;
  }
  else if (!stricmp("list", cmd))
  {
    if (params.empty())
    {
      runCmdList();
      return true;
    }
  }
  else if (!stricmp("clear", cmd))
  {
    clearConsole();
    return true;
  }

  return false;
}


//==============================================================================
const char *CoolConsole::onConsoleCommandHelp(const char *cmd)
{
  if (!stricmp("help", cmd))
    return "Type 'help <command name>' to take command help\n"
           "Type 'list' to take available commands list";
  else if (!stricmp("list", cmd))
    return "Type 'list' to take list of all known commands";
  else if (!stricmp("clear", cmd))
    return "Type 'clear' to completely clear console";

  return NULL;
}


//==============================================================================
void CoolConsole::getCmds(const char *prefix, Tab<String> &cmds) const
{
  if (prefix)
  {
    const int prefLen = i_strlen(prefix);
    const bool startSegment = false;
    for (int i = 0; i < cmdNames.size(); ++i)
    {
      if (cmdNames[i].length() >= prefLen)
      {
        if (!memcmp(prefix, cmdNames[i].begin(), prefLen))
          cmds.push_back(cmdNames[i]);
        else if (startSegment)
          return;
      }
      else if (startSegment)
        return;
    }
  }
  else
    cmds = cmdNames;
}


//==============================================================================
void CoolConsole::setActionDescFmt(const char *desc, const DagorSafeArg *arg, int anum)
{
  addMessageFmt(NOTE, desc, arg, anum);
  call_idle_cycle_seldom();
}


//==============================================================================
void CoolConsole::setTotal(int total_cnt)
{
  progressMaxPosition = total_cnt;
  progressPosition = clamp(progressPosition, 0, progressMaxPosition);
  call_idle_cycle_seldom(true);
}


//==============================================================================
void CoolConsole::setDone(int done_cnt)
{
  progressPosition = clamp(done_cnt, 0, progressMaxPosition);
  call_idle_cycle_seldom(done_cnt == 0);
}


//==============================================================================
void CoolConsole::incDone(int inc)
{
  progressPosition = clamp(progressPosition + inc, 0, progressMaxPosition);
  call_idle_cycle_seldom();
}


void CoolConsole::renderLogTextUI(float height)
{
  const char *consoleOutputLabel = "##consoleOutput";
  ImGui::PushStyleColor(ImGuiCol_ChildBg, PropPanel::getOverriddenColor(PropPanel::ColorOverride::CONSOLE_LOG_BACKGROUND));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
    ImVec2(ImGui::GetStyle().WindowPadding.x * 0.5f, ImGui::GetStyle().WindowPadding.y * 0.5f));
  ImGui::BeginChild(consoleOutputLabel, ImVec2(0.0f, height), ImGuiChildFlags_AlwaysUseWindowPadding);
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();

  const bool scrollbarAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
  formatted_text_handler.updateImgui();
  if (scrollToBottomOnShow || scrollbarAtBottom)
  {
    // Repeat it in the next frame too if the window just appeared. Otherwise it does not work in docked windows...
    // Here we are repeating instead of just delaying scrolling to prevent possible flicker in non-docked mode.
    if (!justMadeVisible)
      scrollToBottomOnShow = false;
    ImGui::SetScrollHereY(1.0f);
  }

  if (formatted_text_handler.hasSelection() && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C))
    ImGui::SetClipboardText(formatted_text_handler.getSelectionAsString());

  if (ImGui::BeginPopupContextWindow(consoleOutputLabel))
  {
    if (formatted_text_handler.hasSelection() && ImGui::MenuItem("Copy", "Ctrl+C"))
      ImGui::SetClipboardText(formatted_text_handler.getSelectionAsString());
    if (ImGui::MenuItem("Copy all"))
      ImGui::SetClipboardText(formatted_text_handler.getAsString());
    ImGui::Separator();
    if (ImGui::MenuItem("Clear"))
      clearConsole();
    ImGui::EndPopup();
  }

  ImGui::EndChild();
}

void CoolConsole::renderCommandInputUI()
{
  ImGui::SetNextItemWidth(-FLT_MIN);

  const ImGuiInputTextFlags textFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;
  const bool enterPressed = ImGuiDagor::InputTextWithHint(
    "##command", "Command", &commandInputText, textFlags,
    [](ImGuiInputTextCallbackData *data) {
      CoolConsole *instance = static_cast<CoolConsole *>(data->UserData);

      if (data->EventFlag & ImGuiInputTextFlags_CallbackHistory)
      {
        if (data->EventKey == ImGuiKey_UpArrow)
        {
          const String prevCmd = instance->onCmdArrowKey(true, instance->commandInputText);
          if (!prevCmd.empty())
          {
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, prevCmd);
          }
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
          data->DeleteChars(0, data->BufTextLen);
          const String nextCmd = instance->onCmdArrowKey(false, instance->commandInputText);
          data->InsertChars(0, nextCmd);
        }
      }
      return 0;
    },
    this);

  const ImGuiID inputId = ImGui::GetItemID();

  // Delay initial focus for one frame as a workaround for focusing not working on just opened docked windows.
  // See: https://github.com/ocornut/imgui/issues/5289
  if ((activateOnShow || enterPressed) && !justMadeVisible)
  {
    activateOnShow = false;
    ImGui::SetKeyboardFocusHere(-1);
  }

  if (enterPressed) // Some commands might display PropPanel dialogs, so we can't execute the command immediately from the ImGui frame.
    PropPanel::request_delayed_callback(*this);
  else if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && isCanClose() && !isProgress() &&
           ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_None, inputId))
    hideConsole();
}

//==============================================================================
void CoolConsole::onImguiDelayedCallback(void *user_data)
{
  runCommand(commandInputText);
  commandInputText.clear();
}

//==============================================================================
void CoolConsole::updateImgui()
{
  const float separatorHeight = ImGui::GetStyle().ItemSpacing.y;
  const float inputHeight = ImGui::GetFrameHeightWithSpacing(); // the progress bar has the same height
  const bool showProgressBar = isProgress() || progressMaxPosition > 0;
  const float progressBarHeight = showProgressBar ? (separatorHeight + inputHeight) : 0.0f;
  const float height = progressBarHeight + separatorHeight + inputHeight;

  renderLogTextUI(-height);

  if (showProgressBar)
  {
    float progress = 0.0f;
    if (progressMaxPosition > 0)
      progress = static_cast<float>(progressPosition) / progressMaxPosition;

    ImGui::Separator();

    // PropPanel::ImguiHelper::getButtonSize()
    const char *buttonLabel = "Cancel";
    const ImVec2 labelSize = ImGui::CalcTextSize(buttonLabel);
    const ImVec2 buttonSize = ImGui::CalcItemSize(ImVec2(0.0f, 0.0f), labelSize.x + ImGui::GetStyle().FramePadding.x * 2.0f,
      labelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, PropPanel::getOverriddenColor(PropPanel::ColorOverride::CONSOLE_PROGRESS_BAR_BACKGROUND));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, PropPanel::getOverriddenColor(PropPanel::ColorOverride::CONSOLE_PROGRESS_BAR));
    ImGui::ProgressBar(progress, ImVec2(progressCb ? -buttonSize.x : -FLT_MIN, 0), "");
    ImGui::PopStyleColor(2);

    if (progressCb)
    {
      ImGui::SameLine();
      if (ImGui::Button(buttonLabel))
        progressCb->onCancel();
    }
  }

  ImGui::Separator();
  renderCommandInputUI();

  justMadeVisible = false;
}
