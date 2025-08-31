//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/array.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <generic/dag_relocatableFixedVector.h>
#include <heightmap/heightmapRenderer.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_computeShaders.h>
#include <math/dag_hlsl_floatx.h>
#include "grassInstance.hlsli"

class DataBlock;


class FastGrassRenderer
{
public:
  struct GrassTypeDesc
  {
    eastl::string name, impostorName;
    dag::RelocatableFixedVector<uint8_t, 12> biomes;

    float height = 0;
    float w_to_height_mul = 0.5f;
    float heightVariance = 0.25f;
    float stiffness = 1;
    E3DCOLOR colors[6] = {0, 0, 0, 0, 0, 0};

  protected:
    friend class FastGrassRenderer;
    uint32_t texId = ~0u;
  };

  FastGrassRenderer();
  ~FastGrassRenderer();
  void close();
  void initOrUpdate(dag::Span<GrassTypeDesc> grass_types);

  void render(const TMatrix4 &globtm, const Point3 &view_pos, float min_ht, float max_ht, float water_level, const BBox2 *clip_box);

  void driverReset();
  void invalidate();
  bool isInited() const { return isInitialized; }

public:
  // Intentionally public for easy setup, call initOrUpdate() to apply
  float sliceStep = 0.5f;
  float fadeStart = 8;
  float fadeEnd = 9;
  float stepScale = 15;
  float heightVarianceScale = 2;
  float smoothnessFadeStart = 1;
  float smoothnessFadeEnd = 30;
  float normalFadeStart = 30;
  float normalFadeEnd = 100;
  float placedFadeStart = 8;
  float placedFadeEnd = 10;
  float aoMaxStrength = 1;
  float aoCurve = 2;
  int numSamples = 4;
  int maxSamples = 64;
  float hmapCellSize = 0.5f;
  float hmapRange = 500;
  int precompResolution = 256;
  int precompCascades = 5;

protected:
  SharedTexHolder albedoTex, normalTex;
  TextureIDHolderWithVar hmapTex, gmapTex, cmapTex;
  ComputeShader precompShader;
  UniqueBufHolder grassChannelsCB, clipmapRectsCB;
  dag::Vector<FastGrassType> grassChannelData;
  dag::Vector<int> clipmapScales;

  HeightmapRenderer hmapRenderer;
  bool isInitialized = false;
  bool alreadyLoaded = false;
  bool needUpdate = true;

  void updateGrassTypes();
};

namespace fast_grass_baker
{
void init_fast_grass_baker();
void fast_grass_baker_on_render();
} // namespace fast_grass_baker
