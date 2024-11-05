//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_carray.h>
#include <math/dag_Point2.h>
#include <drv/3d/dag_consts.h>

class HeroWetness
{
public:
  HeroWetness();
  ~HeroWetness();

  void clearHeroWetnessVolume();
  void calcHeroWetnessVolume(float dt, const TMatrix &hero_tm, const BBox3 &hero_box, bool is_ship, bool has_water_3d,
    float water_level);

  void setDrySpeed(const Point2 &dry_speed)
  {
    wetnessParams.drySpeed = dry_speed;
    wetnessParamsGroundVehicle.drySpeed = dry_speed;
  }
  Point2 getDrySpeed() const { return wetnessParams.drySpeed; }
  Point2 getDrySpeedGroundVehicle() const { return wetnessParamsGroundVehicle.drySpeed; }

  bool isInSleep() const;
  void afterReset() { fillVertexBuffer(getVbSize()); }

protected:
  void init();
  void close();
  // hero foam
  void initHeroWaterFoam(const DataBlock *options_blk);
  void updateHeroWaterFoam(float dt, const TMatrix &hero_tm, bool is_ship);
  void fillVertexBuffer(uint32_t size);
  uint32_t getVbSize() const { return 4 * 6 * heroWetnessVolumeSlices; }

  struct WetnessParams
  {
    Point2 drySpeed;
    float maxDepth;
    float depthBias;
  };

  float wetTime = -1.0f;
  int heroWetnessVolumeSlices;
  int heroWetnessVolumeSizeX, heroWetnessVolumeSizeY;
  int heroWetnessRenderIter;
  float heroWetnessDarkening;
  float heroWetnessAboveDist;
  WetnessParams wetnessParams = {Point2(0.5f, 0.45f), 0.5f, 0.2f};
  WetnessParams wetnessParamsGroundVehicle = {Point2(0.05f, 0.45f), 0.15f, 0.15f};

  carray<Texture *, 2> heroWetnessTex;
  carray<TEXTUREID, 2> heroWetnessTexId;
  int numClearsToDo;
  int heroWetnessTexVarId;

  Vbuffer *waterHeightRendererVb;
  Ptr<ShaderMaterial> waterHeightRendererShmat;
  Ptr<ShaderElement> waterHeightRendererShElem;
  VDECL waterHeightRendererVDecl;

  int globalFrameBlockId;

  // hero foam
  Point2 heroFoamScroll;
  float heroFoamScrollSpeed;
  Point2 heroFoamFadeUnderWater;

  Point2 foamTexScale;
  Point2 foamTextureAlphaScaleOffset;
};
