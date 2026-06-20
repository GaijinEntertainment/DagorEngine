//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <phys/dag_physResource.h>
#include <shaders/dag_rendInstRes.h>
#include <util/dag_simpleString.h>
#include <vecmath/dag_vecMathDecl.h>
#include <generic/dag_staticTab.h>
#include <util/dag_multicastEvent.h>

#include <rendInst/riexHandle.h>
#include <rendInst/rendInstDesc.h>
#include <rendInst/treeDestr.h>


namespace rendinst::props
{

using custom_props_load_cb_t = void(int props_id, const char *ri_name, const DataBlock *ri_blk, const DataBlock *ri_cfg_blk);
extern MulticastEvent<custom_props_load_cb_t> custom_props_load_cb;
using custom_props_clear_all_cb_t = void(bool is_reload);
extern MulticastEvent<custom_props_clear_all_cb_t> custom_props_clear_all_cb;

int get_custom_props_id(int pool, int layer, bool is_extra);
inline int get_custom_props_id(const RendInstDesc &desc) { return get_custom_props_id(desc.pool, desc.layer, desc.cellIdx < 0); }
inline int get_custom_props_id(const riex_handle_t handle) { return get_custom_props_id(handle_to_ri_type(handle), 0, true); }


struct DebrisProps
{
  mat44f debrisTm;
  Tab<short> debrisPoolIdx;
  int delayedPoolIdx;
  int fxType;
  float fxScale;
  float damageDelay;
  float submersion;
  float inclination;
  float impulseOnExplosion;
  SimpleString fxTemplate;
  DebrisProps() : delayedPoolIdx(-1), fxType(-1), fxScale(1.f) {} //-V730
  bool needsUpdate() const { return submersion > 0.f || inclination > 0.f; }
};

struct DestrProps
{
  DynamicPhysObjectData *res = nullptr; // Exists only on clients for now
  float destructionImpulse = 0.f;
  float collisionHeightScale = 1.0f;
  bool destructable = false;
  bool apexDestructible = false;
  SimpleString apexDestructionOptionsPresetName;
  bool isParent = false;
  bool destructibleByParent = false;
  int destroyNeighbourDepth = 1;
  SimpleString tag;
  SimpleString destroyedByTag;
  SimpleString destrName;
  int destrFxId = -1;
  SimpleString destrFxName;
  SimpleString destrFxTemplate;

  DestrProps() = default;
  DestrProps(const DestrProps &p) : DestrProps() { operator=(p); }
  DestrProps(DestrProps &&p) : DestrProps() { operator=(eastl::move(p)); }
  DestrProps &operator=(const DestrProps &p);
  DestrProps &operator=(DestrProps &&p)
  {
    eastl::swap(res, p.res);
    eastl::swap(destructionImpulse, p.destructionImpulse);
    eastl::swap(collisionHeightScale, p.collisionHeightScale);
    destructable = p.destructable;
    apexDestructible = p.apexDestructible;
    eastl::swap(apexDestructionOptionsPresetName, p.apexDestructionOptionsPresetName);
    isParent = p.isParent;
    destructibleByParent = p.destructibleByParent;
    destroyNeighbourDepth = p.destroyNeighbourDepth;
    eastl::swap(tag, p.tag);
    eastl::swap(destroyedByTag, p.destroyedByTag);
    eastl::swap(destrName, p.destrName);
    eastl::swap(destrFxId, p.destrFxId);
    eastl::swap(destrFxName, p.destrFxName);
    eastl::swap(destrFxTemplate, p.destrFxTemplate);
    return *this;
  }
  ~DestrProps();
};

enum class CanopyShape : uint8_t
{
  BOX,
  CONE,
  SPHEROID
};

struct RendinstProperties
{
  int matId;
  bool overrideMaterialForTraces;
  bool immortal;
  bool damageable;
  bool stopsBullets;
  bool bushBehaviour;
  bool treeBehaviour;
  CanopyShape canopyShape;
  float canopyTopOffset;
  float canopyTopPart;
  float canopyWidthPart;
  float canopyOpacity;
  float trunkRadius;
  float soundOcclusion;
  rendinstdestr::BranchDestr treeBranchDestrFromDamage;
  rendinstdestr::BranchDestr treeBranchDestrOther;
};

template <typename T>
const T *get_by_desc(const rendinst::RendInstDesc &desc);

template <typename T>
const T *get_by_handle(riex_handle_t handle)
{
  return get_by_desc<T>(rendinst::RendInstDesc(handle));
}

template <typename T>
const T *get_by_riex_pool(int pool)
{
  return get_by_handle<T>(make_handle(pool, 0));
}

} // namespace rendinst::props

DAG_DECLARE_RELOCATABLE(rendinst::props::RendinstProperties);
DAG_DECLARE_RELOCATABLE(rendinst::props::DestrProps);
DAG_DECLARE_RELOCATABLE(rendinst::props::DebrisProps);
