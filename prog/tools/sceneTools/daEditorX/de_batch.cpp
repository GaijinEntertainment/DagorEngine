#include "de_batch.h"

#include <coolConsole/coolConsole.h>

#include <libTools/util/strUtil.h>

#include <oldEditor/de_interface.h>

#include <osApiWrappers/dag_files.h>

#include <winGuiWrapper/wgw_dialogs.h>

#define TRY_WORD     "__try"
#define TRY_END_WORD "__tryend"

#define STR_BUF_LEN 1024 * 4


static char batchStrBuf[STR_BUF_LEN];


//==================================================================================================
void DeBatch::prepareToBatch()
{
  tryBlock = false;
  gotoTryEnd = false;
  multiLineComment = false;
}


//==================================================================================================
bool DeBatch::openAndRunBatch()
{

  String batchPath = wingw::file_open_dlg(NULL, "Select a batch file", "Dagor batch file (*.dcmd)|*.dcmd", "dcmd");

  if (batchPath.length())
    return runBatch(batchPath);

  return false;
}


//==================================================================================================
bool DeBatch::runBatch(const char *path)
{
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

    bool ret = runBatchStr(commands);

    ::df_close(file);
    return ret;
  }

  return false;
}


//==================================================================================================
bool DeBatch::runBatchStr(const Tab<String> &commands)
{
  String cmdBuf;
  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();
  con.startProgress();
  con.setTotal(commands.size());

  bool ret = true;

  for (int i = 0; i < commands.size(); ++i)
  {
    cmdBuf = commands[i];
    char *cmd = (char *)::trim(cmdBuf.data());
    cmd = checkMultilineComment(cmd);

    if (multiLineComment)
      continue;

    removeSingleLineComment(cmd);

    if (*cmd)
    {
      ProcessAction act = checkBatchSpecialWords(cmd);

      // just run statement
      if (act == PA_RUN_COMMAND)
      {
        // if it is an error block, skip all commands to __tryend statemend
        if (gotoTryEnd)
          continue;

        if (!con.runCommand(cmd))
        {
          ++numErrors;

          if (tryBlock)
          {
            con.addMessage(ILogWriter::FATAL, "Errors while executing statement \"%s\". Go to end of \"" TRY_WORD "\" block.", cmd);

            gotoTryEnd = true;
            continue;
          }
          else
          {
            con.addMessage(ILogWriter::FATAL, "Errors while executing statement \"%s\". Batch aborted.", cmd);

            ret = false;
            break;
          }
        }
      }
      // go to next line
      else if (act == PA_CONTINUE)
      {
        continue;
      }
      // terminate batch
      else if (act == PA_TERMINATE)
      {
        ret = false;
        break;
      }
    }

    con.incDone();
  }

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

  con.endProgress();
  con.showConsole();

  con.addMessage(ILogWriter::NOTE, "Batch finished");

  return ret;
}


//==================================================================================================
DeBatch::ProcessAction DeBatch::checkBatchSpecialWords(const char *str)
{
  CoolConsole &con = DAGORED2->getConsole();

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
void DeBatch::removeSingleLineComment(char *str) const
{
  for (char *c = str; *c; ++c)
    if (*c == '/' && *(c + 1) == '/')
    {
      *c = '\0';
      break;
    }
}


//==================================================================================================
char *DeBatch::checkMultilineComment(char *str)
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
