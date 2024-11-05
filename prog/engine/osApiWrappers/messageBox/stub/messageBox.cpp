// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_messageBox.h>

int os_message_box(const char *, const char *, int) { return GUI_MB_FAIL; }

#define EXPORT_PULL dll_pull_osapiwrappers_messageBox
#include <supp/exportPull.h>
