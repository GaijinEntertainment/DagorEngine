//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

struct TexPixel32;

namespace clipboard
{
KRNLIMP bool get_clipboard_ansi_text(char *dest, int bufSize);
KRNLIMP bool set_clipboard_ansi_text(const char *buf);
KRNLIMP bool get_clipboard_utf8_text(char *dest, int bufSize);
KRNLIMP bool set_clipboard_utf8_text(const char *buf);
KRNLIMP bool set_clipboard_bmp_image(TexPixel32 *im, int wd, int ht, int stride);
} // namespace clipboard

#include <supp/dag_undef_KRNLIMP.h>
