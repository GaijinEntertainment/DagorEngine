//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_DynamicShaderHelper.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_relocatableFixedVector.h>
#include <math/dag_hlsl_floatx.h>
#include <decalMatrices/decalsMatrices.h>
#include "billboard_decals_const.hlsli"
#include "billboard_decals.hlsli"
#include <EASTL/vector.h>

class ShaderMaterial;
class Point3;
class ComputeShaderElement;
struct Frustum;
struct BillboardDecalTexProps;

class BillboardDecals
{
public:
  explicit BillboardDecals();
  void close();
  bool init(float hard_distance, float softness_distance, unsigned max_holes, const char *holes_buffer_name,
    const char *generator_shader_name, const char *clear_sphere_shader_name, const char *decal_shader_name);
  void init_textures(SharedTexHolder &&diffuse, SharedTexHolder &&normal);
  bool init_textures(dag::ConstSpan<const char *> diffuse, dag::ConstSpan<const char *> normal,
    const char *texture_name = "bullet_holes");
  ~BillboardDecals();

  static BillboardDecals *create(float hard_distance, float softness_distance, unsigned max_holes,
    const char *holes_buffer_name = "holesInstances", const char *generator_shader_name = "billboard_decals_generator",
    const char *clear_sphere_shader_name = "billboard_decals_clear_sphere",
    const char *decal_shader_name = "billboard_decals"); // calls init

  void prepareRender(const Frustum &frustum, const Point3 &origin); // generate holes and frustum culling. Call it earlier, to hide
                                                                    // latency
  void renderHoleMask();
  void renderBillboards();
  int32_t addHole(const Point3 &pos, const Point3 &norm, const Point3 &up, float size, uint32_t texture_id, uint32_t matrix_id);
  bool updateHole(const Point3 &pos, const Point3 &norm, const Point3 &up, float size, uint32_t id, uint32_t texture_id,
    uint32_t matrix_id, bool allow_rapid_update);
  void removeHole(uint32_t id);
  void clearInSphere(const Point3 &pos, float sphere_radius);
  void clear(); // clear current holes

  unsigned getHolesNum() const { return numHoles; }
  unsigned int getHolesTypesCount() const { return holesTypes; }
  void afterReset();

protected:
  unsigned int numHolesInTextureH, numHolesInTextureW;
  float radius;

  DynamicShaderHelper holesRenderer;
  DynamicShaderHelper holesMaskRenderer;
  Ptr<ComputeShaderElement> holesGenerator;
  Ptr<ComputeShaderElement> clearSphereShader;
  UniqueBuf holesInstances;
  StaticTab<BillboardToUpdate, MAX_DECALS_TO_UPDATE> holesToUpdate;
  dag::RelocatableFixedVector<Point4, MAX_SPHERES_TO_CLEAR> spheresToClear; // (pos.xyz, radius)

  SharedTexHolder diffuseTex;
  SharedTexHolder bumpTex;

  unsigned int numHoles;
  unsigned int nextHoleNo;
  unsigned int maxHoles;
  float softnessDistance, hardDistance;
  unsigned int holesTypes;
  int w, h;
  unsigned int numFreeDecals = 0;
  eastl::vector<uint32_t> freeDecalIds;

  // to avoid reused a deleted hole in the same frame as it freed up
  // We collect freed ids during frame, and the put it inside freeDecalsIds before render a new frame
  eastl::vector<uint32_t> recentlyFreedIds;

private:
  void render();
};
