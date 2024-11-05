// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <image/dag_texPixel.h>
#include <memory/dag_mem.h>


class IGenLoad;


struct HDRImageInfo
{
  real exposure;
  real gamma;
};


TexImageF *load_rgbe(const char *fn, IMemAlloc *mem, HDRImageInfo *ii);

TexImageF *load_rgbe(IGenLoad &crd, int datalen, IMemAlloc *mem, HDRImageInfo *ii);
