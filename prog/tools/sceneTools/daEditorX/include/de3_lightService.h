//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class Point3;
struct Color3;
class IGenSave;
class DataBlock;
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


class ISceneLightService
{
public:
  static constexpr unsigned HUID = 0x6683CB46u; // ISceneLightService

  // restores all properties to original state
  virtual void reset() = 0;

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

  // returns native direct lighting (or direct lighting derived from other properties)
  virtual void getDirectLight(Point3 &sun1_dir, Color3 &sun1_col, Point3 &sun2_dir, Color3 &sun2_col, Color3 &sky_col,
    bool native_dirlt = true) = 0;
};
