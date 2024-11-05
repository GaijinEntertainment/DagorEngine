// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de3_gui_dialogs.h"
#include <util/dag_string.h>

#if !_TARGET_PC_WIN
String de3_imgui_file_open_dlg(const char *, const char *, const char *, const char *, const char *) { return String(""); }
String de3_imgui_file_save_dlg(const char *, const char *, const char *, const char *, const char *) { return String(""); }
#endif
