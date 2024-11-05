//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>

class DataBlock;
class Point2;

namespace PropPanel
{

struct ColorCorrectionInfo
{
  Tab<Point2> rPoints, gPoints, bPoints, rgbPoints;

  ColorCorrectionInfo();

  void load(const DataBlock &blk);
  void save(DataBlock &blk);

private:
  void loadChannel(const DataBlock &blk, Tab<Point2> &points);
  void saveChannel(DataBlock &blk, Tab<Point2> &points);
};

} // namespace PropPanel