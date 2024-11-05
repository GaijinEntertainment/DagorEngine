//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_overrideStates.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <3d/dag_sbufferIDHolder.h>
#include <EASTL/array.h>
#include <math/dag_Point3.h>
#include <util/dag_oaHashNameMap.h>
#include <math/dag_hlsl_floatx.h>
#include <daECS/core/componentType.h>

#include "esmShadows.h"
#include "esm_ao_decals.hlsli"

class EsmAoManager
{
public:
  static constexpr int MAX_TEXTURES = 64;

  ~EsmAoManager() { close(); }

  bool init(int resolution, float esm_exp);
  void close();

  int getTexId(const char *id_name);

  void beginRenderTexture(int tex_id, const Point3 &pos, const Point3 &frustum_size);
  void endRenderTexture();

  void clearDecals();
  void addDecal(int tex_id, const Point3 &pos, const Point3 &up, const Point3 &frustum_dir, const Point3 &frustum_size);
  void applyAoDecals(int target_width, int target_height, const Point3 &view_pos);

  ShaderElement *getShader() const { return esmShadows.getShader(); }

private:
  TMatrix getInvViewTm(const Point3 &center, const Point3 &up, const Point3 &frustum_dir);
  TMatrix getProjTm(const Point3 &frustum_size);
  void initAoDecalsStateId();

  EsmShadows esmShadows;
  eastl::array<EsmAoDecal, MAX_ESM_AO_DECALS> esmAoDecals;
  SbufferIDHolderWithVar esmAoDecalsBuf;
  DynamicShaderHelper esmAoDecalsRenderer;

  shaders::UniqueOverrideStateId aoDecalsStateId;

  FastNameMap idNames;

  int decalCnt = 0;
};

ECS_DECLARE_BOXED_TYPE(EsmAoManager);
