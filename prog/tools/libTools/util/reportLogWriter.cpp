#include <libTools/util/reportLogWriter.h>

#include <util/dag_globDef.h>
#include <stdio.h>

#include <util/dag_string.h>
#include <propPanel2/comWnd/list_dialog.h>


void ReportLogWriter::addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum)
{
  String msg(" ");
  switch (type)
  {
    case NOTE: break;
    case WARNING:
      msg[0] = 'W';
      warnings = true;
      break;
    case ERROR:
      msg[0] = 'E';
      errors = true;
      break;
    case FATAL:
      msg[0] = 'F';
      errors = true;
      break;
    default: G_ASSERT(false); return;
  }

  msg.avprintf(0, fmt, arg, anum);
  list.push_back() = msg;
}

void ReportLogWriter::clearMessages() { clear_and_shrink(list); }


bool ReportLogWriter::showReport(const char *title, const char *error_msg, const char *ok_msg)
{
  int warn_num = 0, err_num = 0;

  Tab<String> strs(tmpmem);

  for (int i = 0; i < list.size(); i++)
    switch (list[i][0])
    {
      case ' ': strs.push_back().printf(260, "  %s", list[i].str() + 1); break;
      case 'W':
        strs.push_back().printf(260, "Warning: %s", list[i].str() + 1);
        warn_num++;
        break;
      case 'E':
        strs.push_back().printf(260, "Error: %s", list[i].str() + 1);
        err_num++;
        break;
      case 'F':
        strs.push_back().printf(260, "FATAL: %s", list[i].str() + 1);
        err_num++;
        break;
    }

  strs.push_back().printf(260, "   %d errors, %d warnings", err_num, warn_num);
  if (hasErrors())
    strs.push_back() = error_msg;
  else
    strs.push_back() = ok_msg;

  ListDialog dlg(0, title, strs, hdpi::_pxScaled(800), hdpi::_pxScaled(600));

  return dlg.showDialog() == DIALOG_ID_OK;
}
