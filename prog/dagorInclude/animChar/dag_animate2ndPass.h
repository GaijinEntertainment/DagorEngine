//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stddef.h>
#include <EASTL/unique_ptr.h>
#include <anim/dag_animBlend.h>
#include <anim/dag_animStateHolder.h>
#include <animChar/dag_animCharacter2.h>
#include <math/dag_geomTree.h>

class Animate2ndPass
{
  Ptr<AnimV20::AnimationGraph> ag;
  Ptr<AnimV20::AnimData> a2d;
  Tab<int> fadeInStates, fadeOutStates;
  float fadeInDur = 0, fadeOutDur = 0, duration = 1;

public:
  bool load(const AnimV20::AnimcharBaseComponent *animchar, const DataBlock &b, const char *a2d_name, float dur);
  void release();

  bool isLoaded() const { return ag.get() != NULL; }
  AnimV20::AnimationGraph &getGraph() { return *ag; }
  AnimV20::AnimData &getA2D() { return *a2d; }

  float getDuration() const { return duration; }
  float getFadeOutStartTime() const { return duration - fadeOutDur; }

  void startAnim(AnimV20::AnimcharBaseComponent *animchar);
  void endAnim(AnimV20::AnimcharBaseComponent *animchar);
  float getAnimWeightForTime(float t) const
  {
    if (t <= 0)
      return 0;
    if (t < fadeInDur)
      return t / fadeInDur;
    if (t < getFadeOutStartTime())
      return 1;
    if (t >= getDuration())
      return 0;
    return (getDuration() - t) / fadeOutDur;
  }
};

struct Animate2ndPassCtx
{
  struct Ctrl final : public AnimV20::IAnimCharPostController
  {
    struct AnimMap
    {
      dag::Index16 geomId;
      int16_t animId;
    };
    int anim2ndPassOffs;
    float curT = 0;
    eastl::unique_ptr<AnimV20::AnimCommonStateHolder> state;
    Tab<AnimMap> animMap;

    Ctrl(int aoffs) : anim2ndPassOffs(aoffs) {}

    Animate2ndPass &getAnim2ndPass() { return *(Animate2ndPass *)((char *)this + anim2ndPassOffs); }

    bool overridesBlender() const override { return false; }

    void update(real dt, GeomNodeTree & /*tree*/, AnimV20::AnimcharBaseComponent *animchar) override;

    void setToAnimchar(AnimV20::AnimcharBaseComponent &animchar)
    {
      if (animchar.getPostController() == this)
        return;
      animchar.setPostController(this);
      curT = 0;
      getAnim2ndPass().startAnim(&animchar);
    }

    void resetFromAnimchar(AnimV20::AnimcharBaseComponent &animchar)
    {
      getAnim2ndPass().endAnim(&animchar);
      if (animchar.getPostController() == this)
        animchar.setPostController(nullptr);
    }

  private:
    void initAnimState(const GeomNodeTree &tree);
  };

  Animate2ndPass anim2ndPass;
  Ctrl ctrl; // Warn: must be destroyed before 'anim2ndPass' since 'Ctrl::state' hold refs to AnimationGraph's internals

  Animate2ndPassCtx() : ctrl((int)offsetof(Animate2ndPassCtx, anim2ndPass) - (int)offsetof(Animate2ndPassCtx, ctrl)) {}

  // Note: Bear in mind that pointer to ctrl is set to animchar therefore instance of this class
  // can't be relocated (moved in memory) once is set to animchar
  Animate2ndPassCtx(const Animate2ndPassCtx &) = delete;               // Not implemented
  Animate2ndPassCtx &operator=(const Animate2ndPassCtx &ctx) = delete; // Not implemented

  void release(AnimV20::AnimcharBaseComponent &ac)
  {
    if (ac.getPostController() == &ctrl)
      ac.setPostController(NULL);
    ctrl.state.reset();
    anim2ndPass.release();
  }
};
