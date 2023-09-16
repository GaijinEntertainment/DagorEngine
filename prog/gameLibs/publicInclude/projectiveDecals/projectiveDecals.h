//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <generic/dag_staticTab.h>
#include <math/dag_hlsl_floatx.h>
#include "projective_decals_const.hlsli"
#include "projective_decals.hlsli"
#include "projectiveDecalsBuffer.h"
#include <EASTL/vector.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>

struct Frustum;
class Point3;

using ComputeShaderPtr = eastl::unique_ptr<ComputeShaderElement>;
class ProjectiveDecals
{
public:
  ProjectiveDecals() = default;
  ~ProjectiveDecals();

  ProjectiveDecals(const ProjectiveDecals &) = delete;
  ProjectiveDecals &operator=(const ProjectiveDecals &) = delete;
  ProjectiveDecals(ProjectiveDecals &&) = delete;
  ProjectiveDecals &operator=(ProjectiveDecals &&) = delete;

  void close();
  bool init(const char *generator_shader_name, const char *decal_shader_name, const char *decal_cull_shader,
    const char *decal_create_indirect);

  void init_buffer(uint32_t initial_size)
  {
    buffer =
      DecalsBuffer(initial_size, "decalInstances", String(0, "decal_buffer__%s", decalRenderer.shader->getShaderClassName()).c_str());
  }

  void updateDecal(uint32_t decal_id, const TMatrix &tm, float rad, uint16_t texture_id, uint16_t matrix_id, const Point4 &params,
    uint16_t flags);
  void updateParams(uint32_t decal_id, const Point4 &params, uint16_t flags);


  void prepareRender(const Frustum &frustum); // generate decals and frustum culling. Call it earlier, to hide latency
  void render();
  void afterReset();

  void clear(); // clear current decals


  unsigned getDecalsNum() const { return decalsToRender; }

protected:
  DecalsBuffer buffer;

  DynamicShaderHelper decalRenderer;
  ComputeShaderPtr decalUpdater;
  ComputeShaderPtr decalCuller;
  ComputeShaderPtr decalCreator;

  Tab<ProjectiveToUpdate> decalsToUpdate;
  BufPtr indirectBuffer;
  uint32_t decalsToRender = 0;

  void setUpIndirectBuffer();
};

class ResizableDecals : public ProjectiveDecals
{
  dag::Vector<uint32_t> freeDecalIds;
  uint32_t bufferSize = 0;
  int decalsCount = 0;

public:
  void init_buffer(uint32_t initial_size);
  int allocate_decal_id(DecalsBuffer &buffer);
  // unsafe for double removing one decal
  void free_decal_id(int decal_id);
  void clear();

  int addDecal(const TMatrix &transform, float rad, uint16_t matrix_id, const Point4 &params)
  {
    int decal_id = allocate_decal_id(buffer);
    updateDecal(decal_id, transform, rad, decal_id, matrix_id, params, UPDATE_ALL);
    return decal_id;
  }
  int addDecal(const TMatrix &transform, float rad, uint16_t texture_id, uint16_t matrix_id, const Point4 &params)
  {
    int decal_id = allocate_decal_id(buffer);
    updateDecal(decal_id, transform, rad, texture_id, matrix_id, params, UPDATE_ALL);
    return decal_id;
  }
  void removeDecal(uint32_t id)
  {
    G_ASSERTF(id != -1, "-1 invalid decal id");
    updateDecal(id, TMatrix::ZERO, 0, 0, 0, Point4(0, 0, 0, 0), UPDATE_ALL);
    free_decal_id(id);
  }
};


class RingBufferDecals : public ProjectiveDecals
{
  unsigned int ringBufferIndex = 0, ringBufferCapacity;

public:
  void init_buffer(uint32_t maximum_size);

  int allocate_decal_id();

  void clear();

  int addDecal(const TMatrix &transform, float rad, uint16_t matrix_id, const Point4 &params)
  {
    int decal_id = allocate_decal_id();
    updateDecal(decal_id, transform, rad, decal_id, matrix_id, params, UPDATE_ALL);
    return decal_id;
  }
  int addDecal(const TMatrix &transform, float rad, uint16_t texture_id, uint16_t matrix_id, const Point4 &params)
  {
    int decal_id = allocate_decal_id();
    updateDecal(decal_id, transform, rad, texture_id, matrix_id, params, UPDATE_ALL);
    return decal_id;
  }
};
