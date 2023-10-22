//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class IGrassPlanting
{
public:
  static constexpr unsigned HUID = 0x926B194Du; // IGrassPlanting

  virtual void startGrassPlanting(float cx = 0, float cz = 0, float rad = -1) = 0;
  virtual void addGrassPoint(float x, float z) = 0;
  virtual int finishGrassPlanting() = 0;
  virtual float getCellSize() const = 0;
};
