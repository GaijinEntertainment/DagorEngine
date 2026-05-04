// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_delayedAction.h>
#include <debug/dag_debug.h>
#import <UIKit/UIKit.h>

int os_message_box(const char *utf8_text, const char *utf8_caption, int flags)
{
  // there is no reliable way to do blocking message box on ios, previous attempts failed on some os versions
  // so it has to be completely rewritten so mainloop would be aware of the modal message box
  debug("%s(%s, %s, %d)", __FUNCTION__, utf8_text, utf8_caption, flags);

  return GUI_MB_FAIL;
}
