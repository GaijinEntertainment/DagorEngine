//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>
#include <rendInst/rendInstDebris.h>
#include <gamePhys/phys/dynamicPhysModel.h>
#include <gamePhys/phys/rendinstSound.h>
#include <gamePhys/collision/collisionObject.h>

class PhysBody;
class PhysJoint;

namespace rendinst
{
struct RiPhysSettings
{
  bool impulseCallbacksEnabled = false;
};

RiPhysSettings &get_mutable_ri_phys_settings();
const RiPhysSettings &get_ri_phys_settings();
} // namespace rendinst

enum class RendInstPhysType
{
  OTHER,
  TREE,
};

struct RendInstTreeSound
{
private:
  TreeSoundDesc desc;
  ri_tree_sound_cb soundCb = nullptr;
  ri_tree_sound_cb_data_t soundCBData = {};
  bool isInited = false;
  bool wasFalled = false;

public:
  void init(ri_tree_sound_cb tree_sound_cb, const TMatrix &tm, float tree_height, bool is_bush);
  bool inited() const { return isInited; }
  bool falled() const { return wasFalled; }
  void destroy(const TMatrix &tm);

  void setFalled(const TMatrix &tm);
  void updateFalling(const TMatrix &tm);
};

struct RendInstPhys
{
  TMatrix originalTm;

  rendinst::RendInstDesc desc;
  rendinst::RendInstDesc descForDestr;
  rendinst::DestroyedRi *ri;
  CollisionObject riColObj;

  gamephys::DynamicPhysModel *physModel;
  PhysBody *physBody;
  PhysBody *additionalBody;
  Tab<PhysJoint *> joints;
  TMatrix centerOfMassTm;
  Point3 scale;
  TMatrix lastValidTm;

  float ttl;
  float maxTtl;
  float maxLifeDist;
  float distConstrainedPhys;

  rendinst::TreeInstData treeInstData;

  RendInstPhysType type = RendInstPhysType::OTHER;
  RendInstTreeSound treeSound;

  RendInstPhys();
  RendInstPhys(const RendInstPhys &) = delete;
  RendInstPhys(RendInstPhys &&) = default;
  RendInstPhys(RendInstPhysType type, const rendinst::RendInstDesc &from_desc, const TMatrix &tm, float bodyMass, const Point3 &moment,
    const Point3 &scal, const Point3 &centerOfGravity, gamephys::DynamicPhysModel::PhysType phys_type, const TMatrix &original_tm,
    float max_life_time, float max_life_dist, rendinst::DestroyedRi *ri);
  RendInstPhys &operator=(const RendInstPhys &) = delete;
  RendInstPhys &operator=(RendInstPhys &&) = default;
  void cleanup();
};
