// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <daECS/core/entityId.h>
#include <EASTL/array.h>
#include <EASTL/bitvector.h>
#include <EASTL/vector.h>
#include <math/dag_hlsl_floatx.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <memory/dag_framemem.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <blood_puddles/public/shaders/blood_puddles.hlsli>
#include <rendInst/rendInstDesc.h>
#include <render/daFrameGraph/daFG.h>
#include <decalMatrices/decalsMatrices.h>
#include <util/dag_simpleString.h>

class Point3;
class Point4;

namespace ecs
{
class Object;
}

class BloodPuddles
{
  using uint = uint32_t;

  constexpr static uint32_t MAX_PUDDLES = 4096;
  constexpr static uint32_t REMOVE_PUDDLES_THRESHOLD = MAX_PUDDLES / 4 * 3;

public:
  enum DecalGroup
  {
#define BLOOD_DECAL_GROUP_DECL(name, _blk_name, id) name = id,
#include <blood_puddles/public/shaders/decal_group_enum_decl.hlsli>
#undef BLOOD_DECAL_GROUP_DECL
  };
private:
  DecalsMatrices matrixManager;
  eastl::vector<PuddleInfo> puddles;
  UniqueBufHolder puddlesBuf;
  UniqueBufHolder perGroupParametersBuf;
  SharedTexHolder puddleTex;
  SharedTexHolder flowmapTex;
  uint32_t maxSplashesPerFrame = 0;
  uint32_t splashesThisFrame = 0;

  int puddlesToRemoveCount = 0;

  bool isDeferredMode = false;
  dafg::NodeHandle prepassNode;
  dafg::NodeHandle resolveNode;
  uint32_t updateOffset, updateLength;
  SimpleString splashFxTemplName;
  float puddleFreshnessTime = 0;
  float footprintEmitterDryingTime = 0;
  float footprintStrengthDecayPerStep = 1;
  float footprintMinStrength = 0.2f;
  float explosionMaxVerticalThreshold = 0.7f;

  const eastl::array<const char *, BLOOD_DECAL_GROUPS_COUNT> groupNames = {
#define BLOOD_DECAL_NO_COUNT
#define BLOOD_DECAL_GROUP_DECL(_name, blk_name, _id) #blk_name,
#include <blood_puddles/public/shaders/decal_group_enum_decl.hlsli>
#undef BLOOD_DECAL_GROUP_DECL
#undef BLOOD_DECAL_NO_COUNT
  };
  eastl::array<float, BLOOD_DECAL_GROUPS_COUNT> groupDists; // min square diastance for each blood decal group
  eastl::array<IPoint2, BLOOD_DECAL_GROUPS_COUNT> variantsRanges;
  eastl::array<Point2, BLOOD_DECAL_GROUPS_COUNT> sizeRanges;
#include <blood_puddles/private/shaders/per_group_params.hlsli>
  eastl::array<PerGroupParams, BLOOD_DECAL_GROUPS_COUNT> perGroupPackedParams;
  eastl::vector<IPoint2> footprintsRanges;

  void initAtlasResources(const ecs::Object &atlas_settings);
  void closeResources();
  void initNodes();
  void closeNodes();

  void initGroupVariantsRanges(const ecs::Object &group_attributes);

  int getDecalVariant(const int group, Point3 decal_normal) const;
  void addDecal(const int group,
    const Point3 &pos,
    const Point3 &normal,
    const Point3 &dir,
    int matrix_id,
    float size,
    const Point3 &hit_pos,
    bool projective,
    bool is_landscape,
    int variant,
    float strength);
  void startSplashEffect(const Point3 &pos, const Point3 &dir);

  float genRotate(int group, const Point3 &dir) const
  {
    switch (group)
    {
      case BloodPuddles::BLOOD_DECAL_GROUP_LEAK: return 0;

      case BloodPuddles::BLOOD_DECAL_GROUP_FOOTPRINT: return atan2(-dir.z, dir.x);
    }
    return puddles.size();
  }

  float addRandomToSize(int group, float size) const;

  bool texturesAreReady() const { return (bool)(puddleTex && flowmapTex); }

public:
  BloodPuddles();
  ~BloodPuddles();

  void initGroupAttributes(const ecs::Object &group_attributes);

  struct PuddleCtx
  {
    Point3 pos, normal;
    Point3 dir = Point3(0, -1, 0);
    rendinst::riex_handle_t riEx = rendinst::RIEX_HANDLE_NULL;
    float puddleDist = 2;
  };
  bool tryPlacePuddle(PuddleCtx &pctx);
  void addPuddleAt(const PuddleCtx &ctx, int group = BLOOD_DECAL_GROUP_PUDDLE);
  void addPuddle(const Point3 &pos)
  {
    PuddleCtx pctx;
    pctx.pos = pos;
    if (tryPlacePuddle(pctx))
      addPuddleAt(pctx);
  }
  static const int INVALID_VARIANT = 1 << 16;
  void addFootprint(Point3 pos, const Point3 &dir, const Point3 &up_dir, float strength, bool is_left, int variant);
  void update();
  void beforeRender();
  void renderShElem(const TMatrix &view_tm, const TMatrix4 &proj_tm, const ShaderElement *);
  int getCount() const { return puddles.size(); }
  int getMaxSplashesPerFrame() const { return maxSplashesPerFrame; }
  bool tryAllocateSplashEffect() { return splashesThisFrame++ < maxSplashesPerFrame; }
  float getFootprintStrengthDecayPerStep() const { return footprintStrengthDecayPerStep; }
  float getFootprintMinStrength() const { return footprintMinStrength; }
  float getexplosionMaxVerticalThreshold() const { return explosionMaxVerticalThreshold; }
  void setMaxSplashesPerFrame(int max) { maxSplashesPerFrame = max; }
  template <class F>
  void erasePuddles(const F &pred)
  {
    eastl::vector<int, framemem_allocator> queryIndices;
    const int oldSize = puddles.size();
    int removedCount = 0;
    for (int i = 0, j = 0; i < oldSize; ++i)
    {
      if (!pred(puddles[i]))
      {
        puddles[j] = puddles[i];
        ++j;
      }
      else
      {
        removedCount++;
        if (i < puddlesToRemoveCount)
          puddlesToRemoveCount--;
      }
    }
    if (removedCount)
    {
      puddles.resize(puddles.size() - removedCount);
      updateOffset = 0;
      updateLength = puddles.size();
    }
  }
  void erasePuddles()
  {
    puddles.clear();
    puddlesToRemoveCount = 0;
    updateLength = updateOffset = 0;
    matrixManager.clearItems();
  }

  void putDecal(int group,
    const Point3 &pos,
    const Point3 &normal,
    const Point3 &dir,
    float size,
    const Point3 &hit_pos,
    bool projective,
    int variant,
    float strength,
    const uint32_t matrix_id,
    const bool is_landscape);
  void putDecal(int group,
    const Point3 &pos,
    const Point3 &normal,
    float size,
    rendinst::riex_handle_t riex_handle,
    const Point3 &hit_pos,
    bool projective);
  void putDecal(int group,
    const Point3 &pos,
    const Point3 &normal,
    const Point3 &dir,
    float size,
    rendinst::riex_handle_t riex_handle,
    const Point3 &hit_pos,
    bool projective,
    int variant = INVALID_VARIANT,
    float strength = 1.0f);

  void addSplashEmitter(const Point3 &start_pos,
    const Point3 &target_pos,
    const Point3 &normal,
    const Point3 &dir,
    float size,
    rendinst::riex_handle_t riex_handle,
    const Point3 &gravity);

  void reset();
  void addSplash(const Point3 &pos, const Point3 &normal, const TMatrix &itm, int matrix_id, float size);
  float getBloodFreshness(const Point3 &pos) const;

  DecalsMatrices *getMatrixManager() { return &matrixManager; }

  void useDeferredMode(const bool v)
  {
    debug("BloodPuddles: using deferredMode: %d", (int)v);
    isDeferredMode = v;
  }
};

BloodPuddles *get_blood_puddles_mgr();
void init_blood_puddles_mgr();
void close_blood_puddles_mgr();

void create_blood_puddle_emitter(const ecs::EntityId eid, const int collNodeId, const Point3 &offset);
void create_blood_puddle_emitter(const ecs::EntityId eid, const int collNodeId);
void create_blood_puddle_emitter(const Point3 &pos);
void add_hit_blood_effect(const Point3 &pos, const Point3 &dir);

bool is_blood_enabled();
Color4 get_blood_color();
