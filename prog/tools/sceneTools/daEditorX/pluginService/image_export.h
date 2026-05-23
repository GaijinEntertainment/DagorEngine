// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>

String export_any_tga(const char *file_name, void *data, int width, int height, int stride, int in_channels, int in_bits_per_channel,
  bool in_float, int out_channels, int out_bits_per_channel, bool out_float, bool swap_rb);

String export_any_raw(const char *file_name, void *data, int width, int height, int stride, int in_channels, int in_bits_per_channel,
  bool in_float, int out_channels, int out_bits_per_channel, bool out_float, bool swap_rb);

String export_any_tiff(const char *file_name, void *data, int width, int height, int stride, int in_channels, int in_bits_per_channel,
  bool in_float, int out_channels, int out_bits_per_channel, bool out_float, bool swap_rb);
