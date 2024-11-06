// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <rendInst/rendInstExtra.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point4.h>
#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <scene/dag_tiledScene.h>


class RenderableInstanceLodsResource;
class CollisionResource;

namespace rendinst
{


struct RiExtraPool
{
  static constexpr int MAX_LODS = 4;


  RenderableInstanceLodsResource *res = nullptr;
  CollisionResource *collRes = nullptr;
  void *collHandle = nullptr;

  vec4f bsphXYZR = v_make_vec4f(0, 0, 0, 1);

  union
  {
    float distSqLOD[MAX_LODS];
    unsigned distSqLOD_i[MAX_LODS];
    vec4f distSqLODV;
  };
  int riPoolRef = -1;
  LayerFlags layers = {};
  unsigned riPoolRefLayer : 4;
  unsigned useShadow : 1, posInst : 1, destroyedColl : 1, immortal : 1, hasColoredShaders : 1, isTree : 1;
  unsigned hasOccluder : 1, largeOccluder : 1, isWalls : 1, useVsm : 1, usedInLandmaskHeight : 1;
  unsigned patchesHeightmap : 1, usingClipmap : 1;
  unsigned killsNearEffects : 1, hasTransitionLod : 1;
  uint8_t hideMask = 0;
  struct ElemMask
  {
    uint32_t atest, cullN, tessellation, plod;
  } elemMask[MAX_LODS];
  struct ElemUniqueData
  {
    int cellId;
    int offset;
  };
  struct HPandLDT
  {
    float healthPoints, lastDmgTime; // fixme: replace with short?

    void init(float max_hp)
    {
      lastDmgTime = -0.5f;
      healthPoints = max_hp;
    }
    float getHp(float t, float rrate, float max_hp)
    {
      if (lastDmgTime < 0 || t < lastDmgTime)
        return healthPoints;
      float hp = healthPoints + (t - lastDmgTime) * rrate;
      if (hp < max_hp)
        return hp;
      lastDmgTime = -0.5f;
      return healthPoints = max_hp;
    }
    void makeInvincible() { lastDmgTime = -2; }
    void makeNonRegenerating() { lastDmgTime = -1; }
    bool isInvincible() const { return lastDmgTime < -1.5f; }
    void applyDamageNoClamp(float dmg, float t)
    {
      healthPoints -= dmg;
      if (lastDmgTime > -0.75f)
        lastDmgTime = t;
    }
  };

  Tab<mat43f> riTm;
  Tab<vec4f> riXYZR; // R<0 mean 'not in grid'
  E3DCOLOR poolColors[2] = {};
  Tab<uint16_t> uuIdx;
  Tab<HPandLDT> riHP;
  Tab<ElemUniqueData> riUniqueData; // This way we'll identify them after generation, so we can keep them synced
  bbox3f lbb = {};
  bbox3f collBb = {};
  bbox3f fullWabb = {};

  dag::Vector<uint32_t> tsNodeIdx; //(2:20) TiledScene node_index for each instanse (highest 2 bits select TileScene index)
  int tsIndex = -1;

  dag::Vector<int> variableIdsToUpdateMaxHeight;
  float initialHP = -1;
  float regenHpRate = 0;
  int destroyedRiIdx = -1;
  int parentForDestroyedRiIdx = -1;
  bool scaleDebris = false;
  int destrDepth = 0;
  class DynamicPhysObjectData *destroyedPhysRes = nullptr;
  bool isDestroyedPhysResExist = false;
  int destrFxType = -1;
  int destrCompositeFxId = -1;
  float destrFxScale = 0;
  float destrTimeToLive = -1.f;
  float destrTimeToKinematic = -1.f;
  int dmgFxType = -1;
  float dmgFxScale = 1.f;
  float damageThreshold = 0;
  uint8_t destrStopsBullets : 1;

  float sphereRadius = 1.f;
  float sphCenterY = 0.f;
  float radiusFadeDrown = 0.f;
  float radiusFade = 0.f;
  unsigned lodLimits = defLodLimits;
  bool isRendinstClipmap = false;
  bool isPaintFxOnHit = false;
  bool isDynamicRendinst = false;

  struct RiLandclassCachedData
  {
    int index;
    Point4 mapping;
  };
  Tab<RiLandclassCachedData> riLandclassCachedData; // 0 size if not RI landclass (dirty hack to emulate unique ptr)

  float hardness = 1.f;
  float rendinstHeight = 0.f; // some buildings made by putting one ri at other, so the actual height of ri not equal bbox height

  float plodRadius = 0.0f;

  int clonedFromIdx = -1;

  char qlPrevBestLod = 0;
  SimpleString dmgFxTemplate;
  SimpleString destrFxTemplate;

  float scaleForPrepasses = 1.0f;

  // NOTE: bitfields can only be default-initialized in C++20
  RiExtraPool() :
    riPoolRefLayer(0),
    useShadow(false),
    posInst(false),
    destroyedColl(false),
    immortal(false),
    hasColoredShaders(false),
    isTree(false),
    hasOccluder(false),
    largeOccluder(false),
    isWalls(false),
    useVsm(false),
    usedInLandmaskHeight(false),
    patchesHeightmap(false),
    usingClipmap(false),
    killsNearEffects(false),
    hasTransitionLod(false),
    destrStopsBullets(true)
  {
    memset(distSqLOD, 0, sizeof(distSqLOD));
    memset(elemMask, 0, sizeof(elemMask));
  }
  RiExtraPool(const RiExtraPool &) = delete;
  RiExtraPool &operator=(const RiExtraPool &) = default;
  ~RiExtraPool();

  bool isEmpty() const { return riTm.size() == uuIdx.size(); }
  int getEntitiesCount() const { return riTm.size() - uuIdx.size(); }
  void setWasNotSavedToElems();

  bool isPosInst() const { return posInst; }
  bool hasImpostor() const { return posInst; }
  bool hasPLOD() const { return plodRadius > 0.0f; }
  bool isValid(uint32_t idx) const { return idx < riTm.size() && riex_is_instance_valid(riTm.data()[idx]); }
  bool isInGrid(int idx) const
  {
    if (idx < 0 || idx >= riXYZR.size())
      return false;
    uint32_t r_code = ((uint32_t *)&riXYZR[idx])[3];
    return r_code && (r_code & 0x80000000) == 0;
  }
  float getObjRad(int idx) const { return ((unsigned)idx < (unsigned)riXYZR.size()) ? v_extract_w(riXYZR[idx]) : 0.f; }

  float getHp(int idx)
  {
    if (idx < 0 || idx >= riHP.size())
      return 0.f;
    return riHP[idx].getHp(*session_time, regenHpRate, initialHP);
  }
  void setHp(int idx, float hp)
  {
    if (idx < 0 || idx >= riHP.size())
      return;
    riHP[idx].init((hp <= 0.0f) ? initialHP : hp);
    if (hp < -1.5f) // -2
      riHP[idx].makeInvincible();
    else if (hp != 0.0f) // -1 or > 0
      riHP[idx].makeNonRegenerating();
  }
  scene::node_index getNodeIdx(int idx) const
  {
    if (idx < 0 || idx >= tsNodeIdx.size())
      return scene::INVALID_NODE;
    return tsNodeIdx[idx] & 0x3fffffff;
  }
  float bsphRad() const { return v_extract_w(bsphXYZR); }

  void validateLodLimits();

  static unsigned defLodLimits;
  static const float *session_time;
};

} // namespace rendinst