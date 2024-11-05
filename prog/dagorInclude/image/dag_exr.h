//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

bool save_exr(const char *filename, uint8_t **pixels, int width, int height, int planes, int stride, const char **plane_names,
  const char *comments);