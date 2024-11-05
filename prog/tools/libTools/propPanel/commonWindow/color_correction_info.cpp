// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/commonWindow/color_correction_info.h>

#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>

namespace PropPanel
{

ColorCorrectionInfo::ColorCorrectionInfo() : rPoints(midmem), gPoints(midmem), bPoints(midmem), rgbPoints(midmem)
{
  Point2 p0(0, 0), p1(255, 255);
  rgbPoints.push_back(p0);
  rgbPoints.push_back(p1);
  rPoints.push_back(p0);
  rPoints.push_back(p1);
  gPoints.push_back(p0);
  gPoints.push_back(p1);
  bPoints.push_back(p0);
  bPoints.push_back(p1);
}


void ColorCorrectionInfo::load(const DataBlock &blk)
{
  const DataBlock *ch_blk = blk.getBlockByName("rgb_channel");
  if (ch_blk)
    loadChannel(*ch_blk, rgbPoints);

  ch_blk = blk.getBlockByName("r_channel");
  if (ch_blk)
    loadChannel(*ch_blk, rPoints);

  ch_blk = blk.getBlockByName("g_channel");
  if (ch_blk)
    loadChannel(*ch_blk, gPoints);

  ch_blk = blk.getBlockByName("b_channel");
  if (ch_blk)
    loadChannel(*ch_blk, bPoints);
}


void ColorCorrectionInfo::loadChannel(const DataBlock &blk, Tab<Point2> &points)
{
  clear_and_shrink(points);
  int name_id = blk.getNameId("point");
  if (name_id == -1)
    return;

  for (int i = 0; i < blk.paramCount(); ++i)
    if (blk.getParamNameId(i))
      points.push_back(blk.getPoint2(i));
}


void ColorCorrectionInfo::save(DataBlock &blk)
{
  DataBlock *ch_blk = blk.addBlock("rgb_channel");
  saveChannel(*ch_blk, rgbPoints);

  ch_blk = blk.addBlock("r_channel");
  saveChannel(*ch_blk, rPoints);

  ch_blk = blk.addBlock("g_channel");
  saveChannel(*ch_blk, gPoints);

  ch_blk = blk.addBlock("b_channel");
  saveChannel(*ch_blk, bPoints);
}


void ColorCorrectionInfo::saveChannel(DataBlock &blk, Tab<Point2> &points)
{
  blk.clearData();
  for (int i = 0; i < points.size(); ++i)
    blk.addPoint2("point", points[i]);
}

} // namespace PropPanel