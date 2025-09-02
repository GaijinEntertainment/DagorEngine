//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EditorCore/ec_interface.h>
#include <oldEditor/de_common_interface.h>

class Point2;
class Point3;
class DataBlock;
class BBox2;
class IBBox2;
class FFTWater;
class HeightMapStorage;

class IWaterService : public IRenderingService
{
public:
  static constexpr unsigned HUID = 0x51963AEBu; // IWaterService

  virtual void init() = 0;
  virtual void loadSettings(const DataBlock &) = 0;
  virtual void act(float dt) = 0;
  virtual void set_level(float level) = 0;
  virtual void set_wind(float b_scale, const Point2 &wind_dir) = 0;
  virtual void set_render_quad(const BBox2 &box) = 0;
  virtual void hide_water() = 0;

  virtual void beforeRender(Stage stage) = 0;
  void renderGeometry(Stage stage) override = 0;
  virtual void buildDistanceField(const Point2 &hmap_center, float hmap_rad, float rivers_size) = 0;
  virtual void term() = 0;

  virtual void setHeightmap(HeightMapStorage &waterHmapDet, HeightMapStorage &waterHmapMain, const Point2 &waterHeightOfsScaleDet,
    const Point2 &waterHeightOfsScaleMain, BBox2 hmapDetRect, BBox2 hmapRectMain) = 0;
  virtual void exportHeightmap(BinDumpSaveCB &cwr, bool preferZstdPacking, bool allowOodlePacking) = 0;
  virtual void afterD3DReset(bool full_reset) = 0;

  virtual float get_level() const = 0;
};
