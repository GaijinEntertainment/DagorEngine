//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>

class DataBlock;
class Point2;

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
