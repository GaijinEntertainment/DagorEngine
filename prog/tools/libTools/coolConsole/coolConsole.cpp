// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "autocompletePopup.h"
#include "formattedTextHandler.h"

#include <generic/dag_tab.h>
#include <coolConsole/coolConsole.h>
#include <coolConsole/conBatch.h>
#include <generic/dag_sort.h>
#include <propPanel/colors.h>
#include <propPanel/focusHelper.h>
#include <propPanel/imguiHelper.h>
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

#include <EASTL/unique_ptr.h>
#include <EASTL/vector_set.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <limits.h>

#include <stdio.h>

#define ABOUT_MESSAGE "CoolConsole version 1.3"

#define DAGOR_IDLE_CYCLE_CALL_TIMEOUT 1000

class CoolConsoleCommandProcessor : public console::ICommandProcessor
{
public:
  // Use a higher priority to allow overriding built-in commands like "con.cmdlist".
  explicit CoolConsoleCommandProcessor(CoolConsole &instance) :
    ICommandProcessor(console::CONSOLE_CON_PROC_PRIORITY + 1), coolConsole(instance)
  {}

  void destroy() override {}

  bool processCommand(const char *argv[], int argc) override
  {
    if (argc < 1)
      return false;

    const dag::Span<const char *> params = make_span(&argv[1], argc - 1);
    int found = 0;

    CONSOLE_CHECK_NAME_EX("", "help", 1, 2,
      "Type 'help <command name>' to take command help\nType 'list' to take available commands list", "")
    {
      coolConsole.runHelp(params.size() ? params[0] : "help");
    }
    CONSOLE_CHECK_NAME_EX("", "list", 1, 2, "List all known commands. The list can be optionally filtered.", "<search text>")
    {
      coolConsole.runCmdList(params.size() ? params[0] : "");
    }
    // Override "con.cmdlist" with "list" for colored output.
    CONSOLE_CHECK_NAME_EX("con", "cmdlist", 1, 2, "List all known commands. The list can be optionally filtered.", "<search text>")
    {
      coolConsole.runCmdList(params.size() ? params[0] : "");
    }
    CONSOLE_CHECK_NAME_EX("", "clear", 1, 1, "Type 'clear' to completely clear console", "") { coolConsole.clearConsole(); }

    return found != 0;
  }

private:
  CoolConsole &coolConsole;
};

static CoolConsoleCommandProcessor *command_processor = nullptr;
static eastl::unique_ptr<AutocompletePopup> autocomplete_popup;
static FormattedTextHandler formatted_text_handler;
static bool destroyed = false;

static void call_idle_cycle_seldom(bool important, bool allow_redraw)
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
    if (allow_redraw && (!context || !context->WithinFrameScope) && d3d::is_inited())
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

// strtok analog excluding token-delimited qouted strings from tokens search
// for example string 'command "spaced parameter1" parameter2' will be split to
//'command', 'spaced parameter1', 'parameter2'
//==============================================================================
static char *conStrtok(char *str, const char *delim, char *&strtokLastToken)
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

static int compare_command_struct(const console::CommandStruct *a, const console::CommandStruct *b)
{
  return dd_stricmp(a->str(), b->str());
}

static void get_sorted_command_list_wihout_duplicates(console::CommandList &list)
{
  struct ConstCharPtrLess
  {
    bool operator()(const char *a, const char *b) const { return dd_stricmp(a, b) < 0; }
  };

  // The commands are ordered from higher priority command processors to lower.
  console::get_command_list(list);

  // Remove duplicates. They can exist because a higher priority processor can override a command from a lower priority processor.
  eastl::vector_set<const char *, ConstCharPtrLess> added;
  auto newEnd = eastl::remove_if(list.begin(), list.end(),
    [&added](const console::CommandStruct &cs) { return !added.insert(cs.command).second; });
  list.erase(newEnd, list.end());

  sort(list, compare_command_struct);
}

//==============================================================================
CoolConsole::CoolConsole() :
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

  command_processor = new CoolConsoleCommandProcessor(*this);
  add_con_proc(command_processor);

  console::set_visual_driver_raw(this);
}


//==============================================================================
CoolConsole::~CoolConsole()
{
  del_con_proc(command_processor);
  del_it(command_processor);

  stop_cmd_queue_idle_cycle();
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

  callIdleCycleSeldom();
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
  start_cmd_queue_idle_cycle();
  register_cmd_queue_console_handler();
  addMessage(REMARK, ABOUT_MESSAGE);
  return true;
}


//==============================================================================
void CoolConsole::destroyConsole()
{
  stop_cmd_queue_idle_cycle();
  destroyed = true;
}


//==============================================================================
void CoolConsole::showConsole(bool activate)
{
  activateOnShow = activate;
  scrollToBottomOnShow = true;
  visible = true;
  justMadeVisible = true;
}


//==============================================================================
void CoolConsole::hideConsole()
{
  autocomplete_popup.reset();
  visible = false;
}


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
bool CoolConsole::runCommand(const char *cmd, bool silent)
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
    if (!silent)
      addMessage(NOTE, "command history cleared!");
    return true;
  }
  Tab<const char *> params(tmpmem);
  const char *cmdName = NULL;

  char *brkt = nullptr;
  const char *token = ::conStrtok(command, " \t", brkt);

  while (token)
  {
    if (*token)
    {
      if (cmdName)
        params.push_back(token);
      else
        cmdName = token;
    }

    token = ::conStrtok(NULL, " \t", brkt);
  }

  if (!cmdName)
    return false;

  if (!silent)
  {
    addTextToLog(cmdName, PropPanel::ColorOverride::CONSOLE_LOG_COMMAND_TEXT, true);

    for (int i = 0; i < params.size(); ++i)
    {
      addTextToLog(" ", PropPanel::ColorOverride::CONSOLE_LOG_PARAMETER_TEXT, true, false);
      addTextToLog(params[i], PropPanel::ColorOverride::CONSOLE_LOG_PARAMETER_TEXT, true, false);
    }
  }

  limitOutputLinesLeft = 1024;
  command.updateSz(); // after `conStrtok`
  for (auto &p : params)
    command.aprintf(0, " %s", p);
  if (console::command(command.c_str()))
  {
    limitOutputLinesLeft = -1;
    return true;
  }
  if (!silent)
    addTextToLog(String(32, "Unknown command \"%s\"", command.c_str()), PropPanel::ColorOverride::CONSOLE_LOG_ERROR_TEXT);
  limitOutputLinesLeft = -1;
  return false;
}


//==============================================================================
void CoolConsole::runHelp(const char *command)
{
  console::CommandStruct info;
  if (command && *command && console::find_console_command(command, info))
  {
    console::command(String::mk_str_cat("con.help ", command));
    return;
  }

  addTextToLog(String(32, "Unknown command '%s'", command), PropPanel::ColorOverride::CONSOLE_LOG_ERROR_TEXT);
}


//==============================================================================
void CoolConsole::runCmdList(const char *filter)
{
  addTextToLog("Registered commands:", PropPanel::ColorOverride::CONSOLE_LOG_TEXT);

  console::CommandList mlist;
  get_sorted_command_list_wihout_duplicates(mlist);
  debug("mlist.size() = %d", mlist.size());
  for (int i = 0; i < mlist.size(); ++i)
  {
    const console::CommandStruct &cmd = mlist[i];
    if (!dd_stristr(cmd.command, filter))
      continue;

    addTextToLog(cmd.command, PropPanel::ColorOverride::CONSOLE_LOG_TEXT);

    if (!cmd.argsDescription.empty())
    {
      addTextToLog(" ", PropPanel::ColorOverride::CONSOLE_LOG_REMARK_TEXT, false, false);
      addTextToLog(cmd.argsDescription, PropPanel::ColorOverride::CONSOLE_LOG_REMARK_TEXT, false, false);
    }
    else
    {
      for (int k = 1; k < cmd.minArgs; k++)
        addTextToLog(" <x>", PropPanel::ColorOverride::CONSOLE_LOG_PARAMETER_TEXT, false, false);
      for (int k = cmd.minArgs; k < cmd.maxArgs; k++)
        addTextToLog(" [x]", PropPanel::ColorOverride::CONSOLE_LOG_PARAMETER_TEXT, false, false);
    }
    if (cmd.description.length())
    {
      addTextToLog(" -- ", PropPanel::ColorOverride::CONSOLE_LOG_REMARK_TEXT, false, false);
      addTextToLog(cmd.description, PropPanel::ColorOverride::CONSOLE_LOG_REMARK_TEXT, false, false);
    }
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
  callIdleCycleSeldom(true);
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

  callIdleCycleSeldom(true);
}


//==============================================================================
void CoolConsole::setActionDescFmt(const char *desc, const DagorSafeArg *arg, int anum)
{
  addMessageFmt(NOTE, desc, arg, anum);
  callIdleCycleSeldom();
}


//==============================================================================
void CoolConsole::setTotal(int total_cnt)
{
  progressMaxPosition = total_cnt;
  progressPosition = clamp(progressPosition, 0, progressMaxPosition);
  callIdleCycleSeldom(true);
}


//==============================================================================
void CoolConsole::setDone(int done_cnt)
{
  progressPosition = clamp(done_cnt, 0, progressMaxPosition);
  callIdleCycleSeldom(done_cnt == 0);
}


//==============================================================================
void CoolConsole::incDone(int inc)
{
  progressPosition = clamp(progressPosition + inc, 0, progressMaxPosition);
  callIdleCycleSeldom();
}


//==============================================================================
void CoolConsole::yield() { callIdleCycleSeldom(true); }

void CoolConsole::callIdleCycleSeldom(bool important)
{
  const bool allowRedraw = visible && (!isRedrawAllowedCallback || isRedrawAllowedCallback(*this));
  call_idle_cycle_seldom(important, allowRedraw);
}

void CoolConsole::fillAutocompletePopup(bool show_on_empty_input)
{
  if (!autocomplete_popup)
    autocomplete_popup.reset(new AutocompletePopup());

  autocomplete_popup->beginAddingItems();

  if (show_on_empty_input || !commandInputText.empty())
  {
    console::CommandList commandList;
    get_sorted_command_list_wihout_duplicates(commandList);
    for (const console::CommandStruct &cmd : commandList)
      if (dd_stristr(cmd.command, commandInputText))
        autocomplete_popup->addItem(cmd.command, cmd.description);
  }

  autocomplete_popup->endAddingItems();
}

void CoolConsole::renderLogTextUI(float height, ImRect &log_window_rect)
{
  const char *consoleOutputLabel = "##consoleOutput";

  // Get the final child window name. Based on the code from ImGui::BeginChildEx().
  ImGuiWindow *currentWindow = ImGui::GetCurrentWindow();
  const ImGuiID outputWindowId = currentWindow->GetID(consoleOutputLabel);
  const char *outputWindowName;
  ImFormatStringToTempBuffer(&outputWindowName, nullptr, "%s/%s_%08X", currentWindow->Name, consoleOutputLabel, outputWindowId);

  const ImGuiWindow *outputWindow = ImGui::FindWindowByName(outputWindowName);
  const float textWrapWidth = outputWindow ? outputWindow->WorkRect.GetWidth() : ImGui::GetContentRegionAvail().x;
  // ScrollTarget.y == FLT_MAX when there is no scroll request. Its value changes for example when the user turns the mouse wheel.
  const bool scrollbarAtBottom =
    !outputWindow || (outputWindow->Scroll.y >= outputWindow->ScrollMax.y && outputWindow->ScrollTarget.y == FLT_MAX);
  const float textLinesHeight = formatted_text_handler.getTextLinesSize(textWrapWidth).y;

  if (scrollbarAtBottom || scrollToBottomOnShow)
  {
    scrollToBottomOnShow = false;
    ImGui::SetNextWindowScroll(ImVec2(-1.0f, textLinesHeight));
  }

  ImGui::SetNextWindowContentSize(ImVec2(0.0f, textLinesHeight));

  ImGui::PushStyleColor(ImGuiCol_ChildBg, PropPanel::getOverriddenColor(PropPanel::ColorOverride::CONSOLE_LOG_BACKGROUND));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
    ImVec2(ImGui::GetStyle().WindowPadding.x * 0.5f, ImGui::GetStyle().WindowPadding.y * 0.5f));
  ImGui::BeginChild(consoleOutputLabel, ImVec2(0.0f, height), ImGuiChildFlags_AlwaysUseWindowPadding);
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();

  ImGuiID outputTextItemId;
  bool outputTextItemFocused;
  formatted_text_handler.updateImgui(textWrapWidth, outputTextItemId, outputTextItemFocused);

  if (outputTextItemFocused && formatted_text_handler.hasSelection() &&
      (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C, ImGuiInputFlags_None, outputTextItemId) ||
        ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Insert, ImGuiInputFlags_None, outputTextItemId)))
  {
    ImGui::SetClipboardText(formatted_text_handler.getSelectionAsString());
  }

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

  log_window_rect = ImGui::GetCurrentWindow()->InnerRect;
  ImGui::EndChild();
}

void CoolConsole::renderCommandInputUI(const ImRect &log_window_rect)
{
  ImGui::SetNextItemWidth(-FLT_MIN);
  PropPanel::focus_helper.setFocusToNextImGuiControlIfRequested(&commandInputText);

  bool enterPressed;
  if (autocomplete_popup && autocomplete_popup->isPopupOpen())
  {
    // Hacky but there is no saner way to do this because ImGui::Input() uses both IsKeyPressed() and Shortcut().
    PropPanel::ImguiHelper::KeyDownHider keyDownHider(ImGuiKey_Escape);
    enterPressed = ImGuiDagor::InputTextWithHint("##command", "Command", &commandInputText, ImGuiInputTextFlags_EnterReturnsTrue);
  }
  else
  {
    enterPressed = ImGuiDagor::InputTextWithHint("##command", "Command", &commandInputText, ImGuiInputTextFlags_EnterReturnsTrue);
  }

  const ImGuiID inputId = ImGui::GetItemID();
  const ImRect inputRect = ImGui::GetCurrentContext()->LastItemData.Rect;
  const bool inputEdited = ImGui::IsItemEdited();
  bool showOnEmptyInput = false;

  auto setInputText = [this, inputId](const char *text) {
    commandInputText = text;
    if (ImGuiInputTextState *inputState = ImGui::GetInputTextState(inputId))
      inputState->ReloadUserBufAndMoveToEnd();
  };

  // Delay initial focus for one frame as a workaround for focusing not working on just opened docked windows.
  // See: https://github.com/ocornut/imgui/issues/5289
  if (activateOnShow && !justMadeVisible)
  {
    activateOnShow = false;
    ImGui::ActivateItemByID(inputId);
    ImGui::FocusItem();
    ImGui::FocusWindow(ImGui::GetCurrentWindow());
  }

  if (enterPressed)
  {
    // Some commands might display PropPanel dialogs, so we can't execute the command immediately from the ImGui frame.
    commandToRun = commandInputText;
    PropPanel::request_delayed_callback(*this);

    setInputText("");
    autocomplete_popup.reset();

    // Keep the input focused.
    ImGui::SetKeyboardFocusHere(-1);
  }
  else if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && isCanClose() && !isProgress() &&
           ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_None, inputId))
  {
    hideConsole();
  }
  else if (!autocomplete_popup || !autocomplete_popup->isPopupOpen())
  {
    if (ImGui::Shortcut(ImGuiKey_UpArrow, ImGuiInputFlags_Repeat, inputId))
    {
      const String prevCmd = onCmdArrowKey(true, commandInputText);
      if (!prevCmd.empty())
        setInputText(prevCmd);
    }
    else if (ImGui::Shortcut(ImGuiKey_DownArrow, ImGuiInputFlags_Repeat, inputId))
    {
      setInputText(onCmdArrowKey(false, commandInputText));
    }
    else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Space, ImGuiInputFlags_None, inputId))
    {
      showOnEmptyInput = true;
    }
  }

  if (inputEdited || showOnEmptyInput)
    fillAutocompletePopup(showOnEmptyInput);

  if (autocomplete_popup)
  {
    bool selectionChanged, itemClicked;
    autocomplete_popup->updateImgui(log_window_rect, inputRect, selectionChanged, itemClicked);
    if (selectionChanged || itemClicked)
    {
      if (const char *itemText = autocomplete_popup->getSelectedItemName())
      {
        setInputText(itemText);
        if (itemClicked) // The mouse click takes away the focus, we have to restore it.
          PropPanel::focus_helper.requestFocus(&commandInputText);
      }
    }
  }
}

//==============================================================================
void CoolConsole::onImguiDelayedCallback(void *user_data)
{
  runCommand(commandToRun);
  commandToRun.clear();
}

//==============================================================================
void CoolConsole::updateImgui()
{
  const float separatorHeight = ImGui::GetStyle().ItemSpacing.y;
  const float inputHeight = ImGui::GetFrameHeightWithSpacing(); // the progress bar has the same height
  const bool showProgressBar = isProgressBarVisible();
  const float progressBarHeight = showProgressBar ? (separatorHeight + inputHeight) : 0.0f;
  const float height = progressBarHeight + separatorHeight + inputHeight;

  ImRect logWindowRect;
  renderLogTextUI(-height, logWindowRect);

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
  renderCommandInputUI(logWindowRect);

  justMadeVisible = false;
}
