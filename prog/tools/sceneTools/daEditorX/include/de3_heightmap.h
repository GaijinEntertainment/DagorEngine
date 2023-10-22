//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class IHeightmap
{
public:
  static constexpr unsigned HUID = 0xE00CB40Eu; // IHeightmap

  virtual bool isLoaded() const = 0;

  virtual Point3 getHeightmapOffset() const = 0;
  virtual real getHeightmapCellSize() const = 0;
  virtual int getHeightmapSizeX() const = 0;
  virtual int getHeightmapSizeY() const = 0;

  virtual bool getHeightmapPointHt(Point3 &inout_p, Point3 *out_norm) const = 0;
  virtual bool getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid) const = 0;
};
