// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <coolConsole/conBatch.h>
#include <coolConsole/coolConsole.h>

#include <EditorCore/ec_interface.h>

#include <libTools/util/strUtil.h>

#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_files.h>

#include <perfMon/dag_cpuFreq.h>
#include <util/dag_delayedAction.h>

#include <winGuiWrapper/wgw_dialogs.h>


#define TRY_WORD     "__try"
#define TRY_END_WORD "__tryend"

#define STR_BUF_LEN (1024 * 4)


static char batchStrBuf[STR_BUF_LEN];


static ConBatch queuedBatch;

ConBatch *get_cmd_queue_batch() { return &queuedBatch; }

static Tab<String> cmdQueue;
static bool cmdQueueRerun = false;
static int cmdQueueRerunCount = 0;
static bool cmdQueueYield = false;
static int cmdQueueTimeout = -1;
static bool cmdQueueRet = true;

static void queuedBatchUpdate(void *) { ConBatch::updateQueue(); }

void start_cmd_queue_idle_cycle() { register_regular_action_to_idle_cycle(queuedBatchUpdate, nullptr); }

void stop_cmd_queue_idle_cycle() { unregister_regular_action_to_idle_cycle(queuedBatchUpdate, nullptr); }

/* static */
void ConBatch::updateQueue()
{
  if (cmdQueue.empty())
    return;

  if (!queuedBatch.prepared)
  {
    cmdQueueRet = true;
    queuedBatch.prepareToBatch();
    queuedBatch.startBatch(false, 0);
  }

  bool ret = true;
  while (!cmdQueue.empty())
  {
    String &cmdBuf = cmdQueue.front();
    ret = queuedBatch.runCommandStr(cmdBuf);
    if (ret)
    {
      if (!cmdQueueRerun)
      {
        cmdQueue.erase(cmdQueue.begin());
        cmdQueueRerunCount = 0;
      }
      else
      {
        cmdQueueRerun = false;
        cmdQueueRerunCount++;
      }

      if (cmdQueueYield)
      {
        cmdQueueYield = false;
        break;
      }
    }
    else
    {
      break;
    }
  }

  if (cmdQueue.empty())
  {
    cmdQueueRet = queuedBatch.stopBatch(false, ret);
  }
}


//==================================================================================================
void ConBatch::prepareToBatch()
{
  prepared = true;
  tryBlock = false;
  gotoTryEnd = false;
  multiLineComment = false;
}


//==================================================================================================
bool ConBatch::openAndRunBatch()
{
  String batchPath = wingw::file_open_dlg(NULL, "Select a batch file", "Dagor batch file (*.dcmd)|*.dcmd", "dcmd");

  if (batchPath.length())
    return runBatch(batchPath);

  return false;
}


//==================================================================================================
bool ConBatch::runBatch(const char *path, bool async)
{
  if (!async)
    prepareToBatch();

  file_ptr_t file = ::df_open(path, DF_READ);

  if (file)
  {
    String cmdBuf;
    Tab<String> commands(tmpmem);

    for (const char *str = ::df_gets(batchStrBuf, STR_BUF_LEN, file); str; str = ::df_gets(batchStrBuf, STR_BUF_LEN, file))
    {
      cmdBuf = str;
      ::trim(cmdBuf);

      if (cmdBuf.length())
        commands.push_back(cmdBuf);
    }

    bool ret = runBatchStr(commands, async);

    ::df_close(file);
    return ret;
  }

  return false;
}


//==================================================================================================
bool ConBatch::runBatchStr(const Tab<String> &commands, bool async)
{
  if (async)
  {
    cmdQueue.insert(cmdQueue.begin(), commands.begin(), commands.end());
    return true;
  }

  startBatch(true, commands.size());

  bool ret = true;

  int i = 0;
  while (i < commands.size())
  {
    String cmdBuf = commands[i];
    ret = runCommandStr(cmdBuf);
    if (!ret)
      break;

    // handle wait commands in sync run as well!
    if (!cmdQueueRerun)
    {
      i++;
      cmdQueueRerunCount = 0;
    }
    else
    {
      cmdQueueRerun = false;
      cmdQueueRerunCount++;
    }

    if (cmdQueueYield)
    {
      cmdQueueYield = false;
      EDITORCORE->getConsole().yield();
    }
  }

  return stopBatch(true, ret);
}

void ConBatch::startBatch(bool track_progress, int total_cnt)
{
  CoolConsole &con = EDITORCORE->getConsole();
  con.startLog();
  if (track_progress && !::dgs_execute_quiet)
  {
    con.startProgress();
    con.setTotal(total_cnt);
  }
}

//==============================================================================
bool ConBatch::runCommandStr(String &cmdBuf)
{
  CoolConsole &con = EDITORCORE->getConsole();

  char *cmd = (char *)::trim(cmdBuf.data());
  cmd = checkMultilineComment(cmd);

  if (multiLineComment)
  {
    con.incDone();
    return true;
  }

  removeSingleLineComment(cmd);

  if (*cmd)
  {
    ProcessAction act = checkBatchSpecialWords(cmd);

    // just run statement
    if (act == PA_RUN_COMMAND)
    {
      // if it is an error block, skip all commands to __tryend statemend
      if (gotoTryEnd)
      {
        con.incDone();
        return true;
      }

      const bool silent = cmdQueueRerunCount > 0;
      if (!con.runCommand(cmd, silent))
      {
        ++numErrors;

        if (tryBlock)
        {
          con.addMessage(ILogWriter::FATAL, "Errors while executing statement \"%s\". Go to end of \"" TRY_WORD "\" block.", cmd);

          gotoTryEnd = true;

          con.incDone();
          return true;
        }
        else
        {
          con.addMessage(ILogWriter::FATAL, "Errors while executing statement \"%s\". Batch aborted.", cmd);

          return false;
        }
      }
    }
    // go to next line
    else if (act == PA_CONTINUE)
    {
      con.incDone();
      return true;
    }
    // terminate batch
    else if (act == PA_TERMINATE)
    {
      return false;
    }
  }

  con.incDone();
  return true;
}


//==============================================================================
bool ConBatch::stopBatch(bool track_progress, bool ret)
{
  CoolConsole &con = EDITORCORE->getConsole();

  if (tryBlock)
  {
    con.addMessage(ILogWriter::FATAL, "Syntax error: \"" TRY_WORD "\" without ending \"" TRY_END_WORD "\". ");
    ret = false;
  }

  if (multiLineComment)
  {
    con.addMessage(ILogWriter::FATAL, "Syntax error: \"/*\" comment without ending \"*/\" or both \"/*\" and \"*/\" on same line.");
    ret = false;
  }

  if (track_progress && !::dgs_execute_quiet)
  {
    con.endProgress();
    con.showConsole();
  }

  con.addMessage(ILogWriter::NOTE, "Batch finished");

  prepared = false;

  return ret;
}


//==================================================================================================
ConBatch::ProcessAction ConBatch::checkBatchSpecialWords(const char *str)
{
  CoolConsole &con = EDITORCORE->getConsole();

  if (!stricmp(str, TRY_WORD))
  {
    tryBlock = true;
    return PA_CONTINUE;
  }

  if (!stricmp(str, TRY_END_WORD))
  {
    ProcessAction ret = PA_CONTINUE;

    if (tryBlock)
    {
      tryBlock = false;
      gotoTryEnd = false;
    }
    else
    {
      con.addMessage(ILogWriter::FATAL, "Syntax error: \"" TRY_END_WORD "\" without opening \"" TRY_WORD "\". Batch aborted.");

      ret = PA_TERMINATE;
    }

    return ret;
  }

  return PA_RUN_COMMAND;
}


//==================================================================================================
void ConBatch::removeSingleLineComment(char *str) const
{
  for (char *c = str; *c; ++c)
    if (*c == '/' && *(c + 1) == '/')
    {
      *c = '\0';
      break;
    }
}


//==================================================================================================
char *ConBatch::checkMultilineComment(char *str)
{
  if (!multiLineComment)
  {
    for (char *c = str; *c; ++c)
      if (*c == '/' && *(c + 1) == '*')
      {
        *c = '\0';
        multiLineComment = true;
        return str;
      }
  }
  else
  {
    for (char *c = str; *c; ++c)
      if (*c == '*' && *(c + 1) == '/')
      {
        multiLineComment = false;
        return c + 2;
      }
  }

  return str;
}


class CmdQueueConsoleCmdHandler : public console::ICommandProcessor
{
public:
  void destroy() override { delete this; }

  bool processCommand(const char *argv[], int argc) override
  {
    if (argc < 1)
      return false;

    const dag::Span<const char *> params = make_span(&argv[1], argc - 1);
    int found = 0;

    CONSOLE_CHECK_NAME_EX("", "queue", 2, 2, "Type 'queue' <filename> to parse and queue console commands from a file", "") //-V547
    {
      if (params.size() >= 1)
      {
        ConBatch batch;
        batch.runBatch(params[0], true);
      }
    }
    CONSOLE_CHECK_NAME_EX("", "wait", 1, 1, "Shorthand for typing the 'wait.frame 0' command", "") { cmdQueueYield = true; }
    CONSOLE_CHECK_NAME_EX("wait", "frame", 2, 2,
      "Type 'wait.frame' <count> to yield console command queue execution for the specified number of frames", "")
    {
      cmdQueueYield = true;
      if (params.size() > 0)
      {
        if (cmdQueueRerunCount < console::to_int(params[0]))
          cmdQueueRerun = true;
      }
    }
    CONSOLE_CHECK_NAME_EX("wait", "time", 2, 2,
      "Type 'wait.time' <ms> to yield console command queue execution for the specified number of milliseconds", "")
    {
      if (params.size() >= 1)
      {
        const int msec = get_time_msec();
        if (cmdQueueRerunCount == 0)
        {
          cmdQueueTimeout = msec + console::to_int(params[0]);
        }

        if (cmdQueueTimeout > msec)
        {
          cmdQueueRerun = true;
          cmdQueueYield = true;
        }
      }
    }
    CONSOLE_CHECK_NAME_EX("wait", "progress", 1, 1,
      "Type 'wait.progress' to yield console command queue execution while progress is started", "")
    {
      // Don't check console progress when running from queue or in quiet mode
      if (!cmdQueue.empty() || ::dgs_execute_quiet)
      {
        if (EDITORCORE->getConsole().isProgress())
        {
          cmdQueueYield = true;
          cmdQueueRerun = true;
        }
      }
    }

    return found != 0;
  }
};

void register_cmd_queue_console_handler() { add_con_proc(new CmdQueueConsoleCmdHandler()); }
