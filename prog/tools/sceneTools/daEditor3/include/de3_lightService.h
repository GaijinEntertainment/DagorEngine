//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tabFwd.h>
class Point3;
struct Color3;
class IGenSave;
class DataBlock;
class SH3Lighting;
class SHUnifiedLighting;
class StaticGeometryContainer;

namespace StaticSceneBuilder
{
class StdTonemapper;
}
namespace mkbindump
{
class BinDumpSaveCB;
}

struct SunLightProps;
struct SkyLightProps;
class IShLightingProvider;


class ISceneLightService
{
public:
  static constexpr unsigned HUID = 0x6683CB46u; // ISceneLightService

  // restores all properties to original state
  virtual void reset() = 0;

  // installs optional SH lighting provider
  virtual void installSHLightingProvider(IShLightingProvider *lt, bool sh3_sun_lt) = 0;

  // return true when SH+sun lighting is used
  virtual bool isSHSunLtUsed() const = 0;

  // sky and multiple suns properties management
  virtual void setSunCount(int sun_count) = 0;
  virtual int getSunCount() const = 0;

  virtual SkyLightProps &getSky() = 0;
  virtual SunLightProps *getSun(int sun_idx) = 0;
  virtual const SunLightProps &getSunEx(int sun_idx) = 0;

  virtual void saveSunsAndSky(DataBlock &blk) const = 0;
  virtual void loadSunsAndSky(const DataBlock &blk) = 0;

  // automated update of direct light from sun & sky settings (controlled by sync flags)
  virtual void setSyncLtColors(bool sync) = 0;
  virtual void setSyncLtDirs(bool sync) = 0;
  virtual bool getSyncLtColors() = 0;
  virtual bool getSyncLtDirs() = 0;
  virtual bool updateDirLightFromSunSky() = 0;

  // direct lighting shaders support
  virtual void updateShaderVars() = 0;
  virtual bool applyLightingToGeometry() const = 0;

  // tone mapping management
  virtual void saveToneMapper(DataBlock &blk) const = 0;
  virtual void loadToneMapper(const DataBlock &blk) = 0;
  virtual StaticSceneBuilder::StdTonemapper &toneMapper() = 0;

  virtual void tonemapSHLighting(SH3Lighting &inout_lt) const = 0;

  // spherical harmonics calculation
  virtual bool calcSHLighting(SH3Lighting &out_lt, bool add_sky = true, bool add_suns = true, bool tonemap = true) const = 0;

  virtual void calcDefaultSHLighting() = 0;

  // returns interpolated SH3/SHDirLt lighting in point
  virtual void getSHLighting(const Point3 &p, SHUnifiedLighting &out_lt, bool dir_lt = false, bool sun_as_dir1 = false) const = 0;

  // returns mean SH3/SHDirLt lighting in points
  virtual void getSHLighting(dag::ConstSpan<Point3> pt, SHUnifiedLighting &out_lt, bool dir_lt = false,
    bool sun_as_dir1 = false) const = 0;

  // returns native direct lighting (or direct lighting derived from other properties)
  virtual void getDirectLight(Point3 &sun1_dir, Color3 &sun1_col, Point3 &sun2_dir, Color3 &sun2_col, Color3 &sky_col,
    bool native_dirlt = true) = 0;

  // SH3 light manager build/save/load
  virtual void buildLightManager(mkbindump::BinDumpSaveCB &cwr, StaticGeometryContainer &ltgeom) = 0;
  virtual void buildLightManager(const char *fname, StaticGeometryContainer &ltgeom) = 0;
  virtual bool loadLightManager(const char *fname) = 0;
  virtual void destroyLightManager() = 0;
};

class IShLightingProvider
{
public:
  virtual bool getLighting(const Point3 &p, SH3Lighting &out_lt) = 0;
};
