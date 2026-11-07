//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_DObject.h>

class DataBlock;
namespace AnimV20
{
class AnimData;
}


namespace destructables
{

// Props can be used in two modes:
// - stored somewhere, with a guarantee, that all destructables are cleared, before their lifetime ends (rendinsts do this)
// - allocated on heap with a simple refcount, in this case each owner manually calls addRef and delRef
template <typename T>
class PropsRefsBase
{
  static constexpr uint32_t REF_COUNTED = 1u << 31;
  mutable uint32_t refs = 0;

public:
  PropsRefsBase() = default;
  PropsRefsBase(const PropsRefsBase &) {}
  PropsRefsBase &operator=(const PropsRefsBase &) { return *this; }
  ~PropsRefsBase() { G_ASSERT(refCount() == 0); }

  static T *allocRefCounted()
  {
    T *t = new T();
    t->refs = REF_COUNTED | 1;
    return t;
  }
  uint32_t refCount() const { return refs & ~REF_COUNTED; }
  void addRef() const { refs++; }
  void delRef() const
  {
    if (--refs == REF_COUNTED)
      delete static_cast<const T *>(this);
  }
};

struct DestrObjProps : PropsRefsBase<DestrObjProps>
{
  bool isPersistent = false;
  bool collideWithDebris = false;

  float timeToLive = 15.f;            // negative time means never sink
  float inactiveTimeBeforeSink = 3.f; // aliased to timeForBodies in blk
  float playerCollisionTime = 2.f;    // aliased to timeToKinematic in blk
  float timeToSinkUnderground = 3.f;

  float timeToStartDisintegration = 0.f;
  float disintegrationDuration = 0.f;
  float disintegrationScale = 1.f;

  void loadFromBlk(const DataBlock &blk);
};

struct DestrAnimData : PropsRefsBase<DestrAnimData>
{
  eastl::string animRes;
  Ptr<AnimV20::AnimData> anim;

  void loadFromBlk(const DataBlock &blk);
};

} // namespace destructables
