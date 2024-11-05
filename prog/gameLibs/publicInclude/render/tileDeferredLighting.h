//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_postFxRenderer.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <render/omniLightsManager.h>

class TileDeferredLighting
{
public:
  static constexpr int MAX_LIGHTS_PER_TILE = 16;
  static constexpr int TILE_SIZE = 16;

  TileDeferredLighting();
  ~TileDeferredLighting() { close(); }
  bool init(int w, int h);
  void close();
  int preparePointLights(const StaticTab<OmniLightsManager::RawLight, OmniLightsManager::MAX_LIGHTS> &visibleLights);
  void classifyPointLights(const TextureIDPair &far_depth, const TextureIDPair &close_depth);

protected:
  void initAllLights();
  void closeAllLights();
  int width, height;
  TextureIDHolderWithVar lightsIndicesArray;
  static constexpr int LIGHTS_RING_BUFFER = 3;
  carray<TextureIDHolder, LIGHTS_RING_BUFFER> allLights;
  carray<int, LIGHTS_RING_BUFFER> allLightsCount;

  int currentAllLightsId;
  PostFxRenderer prepareLightCount;
};
