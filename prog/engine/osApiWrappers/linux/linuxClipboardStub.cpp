// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_clipboard.h>

namespace clipboard
{
bool get_clipboard_ansi_text(char *, int) { return false; }
bool set_clipboard_ansi_text(const char *) { return false; }
bool get_clipboard_utf8_text(char *, int) { return false; }
bool set_clipboard_utf8_text(const char *) { return false; }
bool set_clipboard_bmp_image(TexPixel32 * /*im*/, int /*wd*/, int /*ht*/, int /*stride*/) { return false; }
bool set_clipboard_file(const char * /*filename*/) { return false; }

} // namespace clipboard

#define EXPORT_PULL dll_pull_osapiwrappers_clipboard
#include <supp/exportPull.h>
