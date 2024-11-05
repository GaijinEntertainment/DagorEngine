// Copyright (C) Gaijin Games KFT.  All rights reserved.

extern int mac_message_box_internal(const char *utf8_text, const char *utf8_caption, int flags);
int os_message_box(const char *utf8_text, const char *utf8_caption, int flags)
{
  return mac_message_box_internal(utf8_text, utf8_caption, flags);
}

#define EXPORT_PULL dll_pull_osapiwrappers_messageBox
#include <supp/exportPull.h>
