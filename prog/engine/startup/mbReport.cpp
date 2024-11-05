// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <supp/_platform.h>
#include <util/limBufWriter.h>
#include <util/dag_cropMultiLineStr.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_messageBox.h>

void messagebox_report_fatal_error(const char *title, const char *msg, const char *call_stack)
{
  if (!::dgs_execute_quiet)
  {
    static char text[8192];

    LimitedBufferWriter lbw(text, sizeof(text));
    if (const char *stk_end = get_multi_line_str_end(call_stack, 10 * 2 + 1, 4096))
      lbw.aprintf("%s\n%.*s%s", msg, int(stk_end - call_stack), call_stack, *stk_end ? "\n..." : "");
    else
      lbw.aprintf("%s", msg);
    lbw.done();

#if _TARGET_PC_WIN
    MessageBox(NULL, text, title, MB_OK | MB_ICONSTOP);
#elif _TARGET_TVOS | _TARGET_C3 | _TARGET_C1 | _TARGET_C2
    // nothing to do with tvOS
    // nothing should be reported on ps4/5
#else
    os_message_box(text, title, GUI_MB_OK);
#endif
  }
}
