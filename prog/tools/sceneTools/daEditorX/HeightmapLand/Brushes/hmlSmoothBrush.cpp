// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlSmoothBrush.h"
#include "../hmlPlugin.h"
#include "../hmlCm.h"
#include <ioSys/dag_dataBlock.h>


void HmapSmoothLandBrush::fillParams(PropPanel::ContainerPropertyControl &panel)
{
  HmapLandBrush::fillParams(panel);
  panel.createTrackInt(PID_BRUSH_SIGMA, "Sigma", sigma * 100, 1, 100, 1);
  panel.createTrackInt(PID_BRUSH_KERNEL_SIZE, "Filter Size", halfKernelSize, 1, 4, 1);
  // params.addReal(PID_BRUSH_KERNEL_SIZE, "KernelSize", limitValue);*/
}


void HmapSmoothLandBrush::updateToPanel(PropPanel::ContainerPropertyControl &panel)
{
  HmapLandBrush::updateToPanel(panel);
  panel.setInt(PID_BRUSH_SIGMA, sigma * 100);
  panel.setInt(PID_BRUSH_KERNEL_SIZE, halfKernelSize);
}


bool HmapSmoothLandBrush::updateFromPanelRef(PropPanel::ContainerPropertyControl &panel, int pid)
{
  switch (pid)
  {
    case PID_BRUSH_SIGMA:
    {
      sigma = panel.getInt(pid) / 100.0;
      makeKernel();
      return true;
    }

    case PID_BRUSH_KERNEL_SIZE:
      halfKernelSize = panel.getInt(pid);
      makeKernel();
      return true;

    default: return HmapLandBrush::updateFromPanelRef(panel, pid);
  }
  return true; // hack
}

void HmapSmoothLandBrush::makeKernel()
{
  where.setEmpty();
  float gaussSigma2 = sigma * sigma;
  int idx = 0;
  float totalKrnl = 0;
  clear_and_resize(gaussKernel, (halfKernelSize * 2 + 1) * (halfKernelSize * 2 + 1));
  for (int filterY = -halfKernelSize; filterY <= halfKernelSize; ++filterY)
    for (int filterX = -halfKernelSize; filterX <= halfKernelSize; ++filterX, ++idx)
    {
      gaussKernel[idx] = expf(-(filterX * filterX + filterY * filterY) / (2 * gaussSigma2 * halfKernelSize * halfKernelSize));
      totalKrnl += gaussKernel[idx];
    }
  for (int idx = 0; idx < gaussKernel.size(); ++idx)
  {
    gaussKernel[idx] /= totalKrnl;
  }
}

void HmapSmoothLandBrush::brushPaintApplyStart(const IBBox2 &where_apply)
{
  HmapLandBrush::brushPaintApplyStart(where_apply);
  if (where != where_apply)
  {
    where = where_apply;
    preFilteredSize = where[1] - where[0] + IPoint2(1 + 2 * halfKernelSize, 1 + 2 * halfKernelSize);
    clear_and_resize(preFiltered, preFilteredSize.x * preFilteredSize.y);
  }
  for (int ty = 0, y = where[0].y - halfKernelSize; y <= where[1].y + halfKernelSize; ++y, ++ty)
    for (int tx = 0, x = where[0].x - halfKernelSize; x <= where[1].x + halfKernelSize; ++x, ++tx)
      preFiltered[ty * preFilteredSize.x + tx] = heightMap.getBrushImageData(x, y, HmapLandPlugin::self->getEditedChannel());
}

bool HmapSmoothLandBrush::brushPaintApply(int x, int y, float inc, bool rb)
{
  if (!(where & IPoint2(x, y)))
    return false;
  if (inc > 1.0f)
    inc = 1.0f;
  float result = 0.0f;
  x -= where[0].x;
  y -= where[0].y;
  for (int filterY = 0; filterY < 2 * halfKernelSize + 1; ++filterY)
    for (int filterX = 0; filterX < 2 * halfKernelSize + 1; ++filterX)
    {
      float filterWeight = gaussKernel[filterX + (filterY) * (2 * halfKernelSize + 1)];
      if (rb)
      {
        if (filterY == halfKernelSize && filterX == halfKernelSize)
          filterWeight = 1 + (1 - filterWeight);
        else
          filterWeight = -filterWeight;
      }
      result += preFiltered[x + filterX + (y + filterY) * preFilteredSize.x] * filterWeight;
    }
  float prevHeight = preFiltered[x + halfKernelSize + (y + halfKernelSize) * preFilteredSize.x];

  heightMap.setBrushImageData(where[0].x + x, where[0].y + y, result * inc + prevHeight * (1.0f - inc),
    HmapLandPlugin::self->getEditedChannel());
  return true;
}

void HmapSmoothLandBrush::saveToBlk(DataBlock &blk) const
{
  HmapLandBrush::saveToBlk(blk);

  blk.addInt("halfKernelSize", halfKernelSize);
  blk.addReal("sigma", sigma);
}

void HmapSmoothLandBrush::loadFromBlk(const DataBlock &blk)
{
  HmapLandBrush::loadFromBlk(blk);

  halfKernelSize = blk.getInt("halfKernelSize", halfKernelSize);
  sigma = blk.getReal("sigma", sigma);
  makeKernel();
}

namespace heightmap_land
{
HmapLandBrush *getSmoothBrush(IBrushClient *client, IHmapBrushImage &height_map)
{
  return new HmapSmoothLandBrush(client, height_map);
}
}; // namespace heightmap_land
