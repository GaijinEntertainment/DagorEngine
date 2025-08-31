// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "brushMask.h"

#include <EditorCore/ec_IEditorCore.h>

#include <math/dag_bits.h>
#include <supp/dag_math.h>
#include <libTools/util/fileMask.h>

#include <float.h>

using editorcore_extapi::dagTools;

static inline unsigned get_power(int num)
{
  unsigned long power = 0;
  __bit_scan_reverse(power, num);
  return power;
}

float IBrushMask::Mask::getCenteredMask(float x1, float y1, float radius, float angle)
{
  if (!width || !height)
    return 1;

  float x, y;
  if (angle != 0)
  {
    float cosa = cosf(angle), sina = sinf(angle);
    x = cosa * x1 + sina * y1;
    y = -sina * x1 + cosa * y1;
  }
  else
  {
    x = x1;
    y = y1;
  }


  float mx = (x / 2.0f + 0.5f) * width;
  float my = (y / 2.0f + 0.5f) * height;
  if (mx < 0)
    mx = 0;
  if (mx >= width)
    mx = width - 1;
  if (my < 0)
    my = 0;
  if (my >= height)
    my = height - 1;

  float cellXf, cellYf;
  float fracX = modff(mx, &cellXf);
  float fracY = modff(my, &cellYf);
  int cellX = int(cellXf), cellY = int(cellYf);
  int cellX1 = cellX, cellY1 = cellY;
  if (cellX < width - 1)
    cellX1 = cellX + 1;
  if (cellY < height - 1)
    cellY1 = cellY + 1;

  int p11 = cellX + cellY * width;
  int p12 = cellX1 + cellY * width;
  int p21 = cellX + cellY1 * width;
  int p22 = cellX1 + cellY1 * width;

  if (p11 < 0 || p12 < 0 || p21 < 0 || p22 < 0 || p11 >= mask.size() || p12 >= mask.size() || p21 >= mask.size() || p22 >= mask.size())
    return 1.0;

  float maskResult =
    (mask[p11] * (1.0 - fracX) + mask[p12] * fracX) * (1.0f - fracY) + (mask[p21] * (1.0 - fracX) + mask[p22] * fracX) * fracY;

  if (maskResult > 1.0f)
    maskResult = 1.0f;

  return maskResult;
}

float IBrushMask::getCenteredMask(float x1, float y1, float radius, float angle)
{
  if (!maskChain.size())
    return 1;

  int maski;

  for (maski = 0; maski < maskChain.size(); ++maski)
  {
    if (maskChain[maski].width < radius)
      break;
  }

  if (maski == maskChain.size() || maski == 0)
    return maskChain[maski ? maski - 1 : maski].getCenteredMask(x1, y1, radius, angle);

  float maskResult1 = maskChain[maski].getCenteredMask(x1, y1, radius, angle);
  float maskResult2 = maskChain[maski - 1].getCenteredMask(x1, y1, radius, angle);
  float maskIweight = (maskChain[maski - 1].width - radius) / (maskChain[maski - 1].width - maskChain[maski].width);
  return maskResult2 * (1.0f - maskIweight) + maskResult2 * maskIweight;
}


bool IBrushMask::load(const char *file)
{
  maskChain.clear();

  SmallTab<float, TmpmemAlloc> mask;
  int width, height;
  bool ret = dagTools->loadmaskloadMaskFromFile(file, mask, width, height);

  if (!ret || !width || !height)
    return false;

  float min = FLT_MAX, max = -FLT_MAX;
  for (int i = 0; i < mask.size(); ++i)
    if (mask[i] < min)
      min = mask[i];
    else if (mask[i] > max)
      max = mask[i];

  if (max == min)
    min = 0;

  for (int i = 0; i < mask.size(); ++i)
    mask[i] = (mask[i] - min) / (max - min);

  int chains = width < height ? ::get_power(width) : ::get_power(height);
  maskChain.resize(chains);

  if (!maskChain.size())
    return true;

  maskChain[0].width = width;
  maskChain[0].height = height;
  maskChain[0].mask = mask;

  for (int i = 1; i < maskChain.size(); ++i)
  {
    maskChain[i].width = width >> i;
    maskChain[i].height = height >> i;
    clear_and_resize(maskChain[i].mask, maskChain[i].width * maskChain[i].height);

    int subW = width / maskChain[i].width;
    int subH = height / maskChain[i].height;
    for (int id = 0, y = 0; y < maskChain[i].height; ++y)
    {
      for (int x = 0; x < maskChain[i].width; ++x, ++id)
      {
        double val = 0;
        for (int sy = 0; sy < subH; ++sy)
        {
          for (int sx = 0; sx < subW; ++sx)
          {
            val += mask[(y * subH + sy) * width + (x * subW + sx)];
          }
        }
        maskChain[i].mask[id] = (val / (subW * subH));
      }
    }
  }
  return ret;
}

IBrushMask::IBrushMask() : maskChain(midmem) {}
