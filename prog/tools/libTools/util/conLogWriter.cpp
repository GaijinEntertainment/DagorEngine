#include <libTools/util/conLogWriter.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <stdio.h>

void ConsoleLogWriter::addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum)
{
  String con_fmt;
  switch (type)
  {
    case NOTE: con_fmt.printf(0, "\r*%s\n", fmt); break;
    case WARNING: con_fmt.printf(0, "\rWARNING: %s\n", fmt); break;
    case ERROR:
      con_fmt.printf(0, "\nERROR: %s\n", fmt);
      err = true;
      break;
    case FATAL:
      con_fmt.printf(0, "\n-FATAL-: %s\n", fmt);
      err = true;
      break;
    default: G_ASSERT(false); return;
  }

#if _TARGET_STATIC_LIB
  DagorSafeArg::fprint_fmt(stdout, con_fmt, arg, anum);
#else
  String msg;
  msg.vprintf(0, con_fmt, arg, anum);
  if (fwrite(msg.data(), 1, msg.length(), stdout))
    clear_and_shrink(msg);
#endif
}
