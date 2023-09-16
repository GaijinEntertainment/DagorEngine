//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_globDef.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <math/dag_Point4.h>
#include <math/dag_Point2.h>
#include <math/dag_bounds2.h>
#include <3d/dag_sbufferIDHolder.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_resPtr.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/string.h>

struct Frustum;
class DataBlock;

class BurningDecals
{
protected:
  struct BurningDecal
  {
    Point2 localX;
    Point2 localY;
    Point2 pos;
    Point4 tc;
    float lifetime = -1;
    float strength = 0;
    float initialLifetime = -1;

    bool clipmapDecalsCreated = false;
    int clipmapDecalsCount = 0;
    Point2 clipmapDecalsDir;
    float clipmapDecalsSize = 0;
  };

  Tab<BurningDecal> decals;

  TextureIDHolderWithVar burningMap;
  TextureIDHolderWithVar bakedBurningMap;

private:
  int activeCount = 0;
  int clipmapDecalType = 0;
  int resolution = 1024;
  DynamicShaderHelper material;
  SbufferIDHolder decalDataVS;

  struct InstData
  {
    Point2 localX;
    Point2 localY;
    Point4 tc;
    Point4 pos;
  };
  carray<InstData, 100> instData;

  bool inited = false;
  bool needUpdate = false;

  const int decals_count = 500;

  enum
  {
    RENDER_TO_Burning = 0,
    RENDER_TO_DISPLACEMENT = 1,
    RENDER_TO_GRASS_MASK = 2
  };

  void renderByCount(int count);
  void createTexturesAndBuffers();

public:
  BurningDecals();
  ~BurningDecals();

  // release system
  void release();

  void update(float dt);

  // remove all
  void clear();

  // render decals
  void render();

  void afterReset();

  void createDecal(const Point2 &pos, const Point2 &dir, const Point2 &size, float strength, float duration);
  void createDecal(const Point2 &pos, float rot, const Point2 &size, float strength, float duration);

  Tab<BBox2> updated_regions;

  const Tab<BBox2> &get_updated_regions();
  void clear_updated_regions();
  void setResolution(int res) { resolution = res; }
};

namespace burning_decals_mgr
{
void createDecal(const Point2 &pos, const Point2 &dir, const Point2 &size, float strength, float duration);
void createDecal(const Point2 &pos, float rot, const Point2 &size, float strength, float duration);

// init system
void init();

// release system
void release();

// remove all decals from map
void clear();

void after_reset();

void render();
void update(float dt);

const Tab<BBox2> &get_updated_regions();
void clear_updated_regions();
void set_resolution(int res);
} // namespace burning_decals_mgr
