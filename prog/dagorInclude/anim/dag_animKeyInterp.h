//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_stdint.h>
#include <anim/dag_animChannels.h>
#include <anim/dag_animKeys.h>

namespace AnimV20Math
{


template <class KEY>
struct AnimChan;
typedef AnimChan<AnimV20::AnimKeyPoint3> AnimChanPoint3;
typedef AnimChan<AnimV20::AnimKeyQuat> AnimChanQuat;
typedef AnimChan<AnimV20::AnimKeyReal> AnimChanReal;


inline int seconds_to_ticks(float seconds) { return roundf(seconds * AnimV20::TIME_TicksPerSec); }


struct AnimSamplerConfig
{
  // If true, sampling time must be specified in the constructor call.
  static constexpr bool is_one_shot() { return false; }
};

struct DefaultConfig : AnimSamplerConfig
{};

struct OneShotConfig : AnimSamplerConfig
{
  static constexpr bool is_one_shot() { return true; }
};


#define ONE_SHOT_CTOR_CHECK() \
  if constexpr (k_one_shot)   \
  G_ASSERT_EX(false, "time must be specified in ctor for one-shot sampler")

#define ONE_SHOT_SEEK_CHECK() \
  if constexpr (k_one_shot)   \
  G_ASSERT_EX(false, "can't seek one-shot sampler")

// For sampling several nodes of the same AnimData.
template <typename CFG>
struct PrsAnimSampler
{
  PrsAnimSampler(const AnimV20::AnimData *ad = nullptr) : anim(ad ? &ad->anim : nullptr) { ONE_SHOT_CTOR_CHECK(); }

  explicit PrsAnimSampler(const AnimV20::AnimData *ad, int time_ticks) : anim(ad ? &ad->anim : nullptr) { setTimeTicks(time_ticks); }

  explicit PrsAnimSampler(const AnimV20::AnimData *ad, float time_seconds) : anim(ad ? &ad->anim : nullptr)
  {
    setTimeTicks(seconds_to_ticks(time_seconds));
  }

  void seekTicks(int ticks)
  {
    ONE_SHOT_SEEK_CHECK();
    setTimeTicks(ticks);
  }

  void seek(float seconds)
  {
    ONE_SHOT_SEEK_CHECK();
    setTimeTicks(seconds_to_ticks(seconds));
  }

  int getTimeTicks() const { return curTime; }
  float getTimeSeconds() const { return curTime / float(AnimV20::TIME_TicksPerSec); }

  // seek to each key time
  template <typename F>
  void forEachSclKey(dag::Index16 node_id, F callback);

  vec3f samplePos(dag::Index16 node_id) const;
  quat4f sampleRot(dag::Index16 node_id) const;
  vec3f sampleScl(dag::Index16 node_id) const;

protected:
  static constexpr bool k_one_shot = CFG::is_one_shot();

  const AnimV20::AnimData::Anim *anim;
  int curTime = 0;

  void setTimeTicks(int ticks) { curTime = ticks; }
};


// For samping single node.
template <typename CFG>
struct PrsAnimNodeSampler
{
  PrsAnimNodeSampler()
  {
    ONE_SHOT_CTOR_CHECK();
    init(nullptr, dag::Index16(), dag::Index16(), dag::Index16());
  }

  PrsAnimNodeSampler(const AnimV20::PrsAnimNodeRef &prs)
  {
    ONE_SHOT_CTOR_CHECK();
    init(prs.anim, prs.posId, prs.rotId, prs.sclId);
  }

  PrsAnimNodeSampler(const AnimV20::AnimData *a, dag::Index16 p, dag::Index16 r, dag::Index16 s)
  {
    ONE_SHOT_CTOR_CHECK();
    init(a ? &a->anim : nullptr, p, r, s);
  }

  PrsAnimNodeSampler(const AnimV20::AnimData *a, dag::Index16 p, dag::Index16 r, dag::Index16 s, int ticks)
  {
    initOneShot(a ? &a->anim : nullptr, p, r, s, ticks);
  }

  explicit PrsAnimNodeSampler(const AnimV20::PrsAnimNodeRef &prs, int ticks)
  {
    initOneShot(prs.anim, prs.posId, prs.rotId, prs.sclId, ticks);
  }

  explicit PrsAnimNodeSampler(const AnimV20::PrsAnimNodeRef &prs, float seconds)
  {
    initOneShot(prs.anim, prs.posId, prs.rotId, prs.sclId, seconds_to_ticks(seconds));
  }

  void seekTicks(int ticks)
  {
    ONE_SHOT_SEEK_CHECK();
    setTimeTicks(ticks);
  }

  void seek(float seconds)
  {
    ONE_SHOT_SEEK_CHECK();
    setTimeTicks(seconds_to_ticks(seconds));
  }

  vec3f samplePos() const;
  quat4f sampleRot() const;
  vec3f sampleScl() const;

  void sampleTransform(vec3f &p, quat4f &r, vec3f &s) const
  {
    p = samplePos();
    r = sampleRot();
    s = sampleScl();
  }

protected:
  static constexpr bool k_one_shot = CFG::is_one_shot();

  struct LongCtx
  {
    const AnimChanQuat *rot;
    const AnimChanPoint3 *pos, *scl;
    int curTime;
  };

  struct OneShotCtx
  {
    const AnimV20::AnimKeyQuat *krot;
    const AnimV20::AnimKeyPoint3 *kpos, *kscl;
    float arot, apos, ascl;
  };

  typename eastl::conditional_t<k_one_shot, OneShotCtx, LongCtx> ctx;

  void init(const AnimV20::AnimData::Anim *a, dag::Index16 p, dag::Index16 r, dag::Index16 s);
  void initOneShot(const AnimV20::AnimData::Anim *a, dag::Index16 p, dag::Index16 r, dag::Index16 s, int ticks);

  void setTimeTicks(int ticks)
  {
    if constexpr (!k_one_shot)
      ctx.curTime = ticks;
  }
};


template <typename CFG>
struct Float3AnimNodeSampler
{
  Float3AnimNodeSampler(const AnimV20::AnimDataChan &anim, dag::Index16 node = dag::Index16(0))
  {
    ONE_SHOT_CTOR_CHECK();
    init(anim, node);
  }

  Float3AnimNodeSampler(const AnimV20::AnimDataChan &anim, int ticks, dag::Index16 node = dag::Index16(0))
  {
    initOneShot(anim, node, ticks);
  }

  Float3AnimNodeSampler(const AnimV20::AnimDataChan &anim, float seconds, dag::Index16 node = dag::Index16(0))
  {
    initOneShot(anim, node, seconds_to_ticks(seconds));
  }

  void seekTicks(int ticks)
  {
    ONE_SHOT_SEEK_CHECK();
    setTimeTicks(ticks);
  }

  void seek(float seconds)
  {
    ONE_SHOT_SEEK_CHECK();
    setTimeTicks(seconds_to_ticks(seconds));
  }

  vec3f sample(vec3f default_value = v_zero()) const;

protected:
  static constexpr bool k_one_shot = CFG::is_one_shot();

  struct LongCtx
  {
    const AnimChanPoint3 *chan;
    int curTime;
  };

  struct OneShotCtx
  {
    const AnimV20::AnimKeyPoint3 *key;
    float alpha;
  };

  typename eastl::conditional_t<k_one_shot, OneShotCtx, LongCtx> ctx;

  void init(const AnimV20::AnimDataChan &anim, dag::Index16 node);
  void initOneShot(const AnimV20::AnimDataChan &anim, dag::Index16 node, int ticks);

  void setTimeTicks(int ticks)
  {
    if constexpr (!k_one_shot)
      ctx.curTime = ticks;
  }
};


// Implementation details:

// Animations for channels as sequence of keys
template <class KEY>
struct AnimChan
{
  PatchablePtr<KEY> key;
  PatchablePtr<uint16_t> keyTime16;
  int keyNum;
  int _resv;

  int keyTimeFirst() const { return unsigned(keyTime16[0]) << AnimV20::TIME_SubdivExp; }
  int keyTimeLast() const { return unsigned(keyTime16[keyNum - 1]) << AnimV20::TIME_SubdivExp; }
  int keyTime(int idx) const { return unsigned(keyTime16[idx]) << AnimV20::TIME_SubdivExp; }

  __forceinline KEY *findKey(int t32, float *out_t) const
  {
    if (keyNum == 1 || t32 <= keyTimeFirst())
    {
      *out_t = 0;
      return &key[0];
    }
    if (t32 >= keyTimeLast())
    {
      *out_t = 0;
      return &key[keyNum - 1];
    }

    int t = t32 >> AnimV20::TIME_SubdivExp, a = 0, b = keyNum - 1;
    while (b - a > 1)
    {
      int c = (a + b) / 2;
      if (keyTime16[c] == t)
      {
        if (keyTime(c) == t32)
        {
          *out_t = 0;
          return &key[c];
        }
        *out_t = float(t32 - (t << AnimV20::TIME_SubdivExp)) / float((keyTime16[c + 1] - t) << AnimV20::TIME_SubdivExp);
        return &key[c];
      }
      else if (keyTime16[c] < t)
        a = c;
      else
        b = c;
    }
    *out_t = float(t32 - keyTime(a)) / float(keyTime(b) - keyTime(a));
    return &key[a];
  }

  __forceinline KEY *findKeyEx(int t32, float *out_t, int &dkeys) const
  {
    if (keyNum == 1 || t32 <= keyTimeFirst())
    {
      dkeys = 0;
      *out_t = 0;
      return &key[0];
    }
    if (t32 >= keyTimeLast())
    {
      dkeys = 0;
      *out_t = 0;
      return &key[keyNum - 1];
    }

    int t = t32 >> AnimV20::TIME_SubdivExp, a = 0, b = keyNum - 1;
    while (b - a > 1)
    {
      int c = (a + b) / 2;
      if (keyTime16[c] == t)
      {
        if (keyTime(c) == t32)
        {
          dkeys = (c < keyNum - 1) ? (keyTime16[c + 1] - keyTime16[c]) : 0;
          *out_t = 0;
          return &key[c];
        }
        dkeys = keyTime16[c + 1] - keyTime16[c];
        *out_t = float(t32 - (t << AnimV20::TIME_SubdivExp)) / float((keyTime16[c + 1] - t) << AnimV20::TIME_SubdivExp);
        return &key[c];
      }
      else if (keyTime16[c] < t)
        a = c;
      else
        b = c;
    }
    dkeys = keyTime16[b] - keyTime16[a];
    *out_t = float(t32 - keyTime(a)) / float(keyTime(b) - keyTime(a));
    return &key[a];
  }
};

static_assert(sizeof(AnimChanPoint3) == sizeof(AnimChanQuat));

// Key data

__forceinline real interp_key(const AnimV20::AnimKeyReal &a, real t) { return ((a.k3 * t + a.k2) * t + a.k1) * t + a.p; }

__forceinline vec3f interp_key(const AnimV20::AnimKeyPoint3 &a, vec4f t)
{
  return v_madd(v_madd(v_madd(a.k3, t, a.k2), t, a.k1), t, a.p);
}

__forceinline vec4f interp_key(const AnimV20::AnimKeyQuat &a, const AnimV20::AnimKeyQuat &b, real t)
{
  return v_quat_qsquad(t, a.p, a.b0, a.b1, b.p);
}


// Sampler implementation

__forceinline const AnimChanPoint3 *get_point3_anim(const AnimV20::AnimDataChan &anim, dag::Index16 node_id)
{
  if (node_id.index() >= anim.nodeNum)
    return nullptr;
  return (const AnimChanPoint3 *)anim.nodeAnim.get() + node_id.index();
}

__forceinline const AnimChanQuat *get_quat_anim(const AnimV20::AnimDataChan &anim, dag::Index16 node_id)
{
  if (node_id.index() >= anim.nodeNum)
    return nullptr;
  return (const AnimChanQuat *)anim.nodeAnim.get() + node_id.index();
}

template <typename KEY>
__forceinline const KEY *find_key(const AnimChan<KEY> *ch, int t, float *a)
{
  if (!ch || ch->keyNum == 0)
  {
    *a = 0;
    return nullptr;
  }
  return ch->findKey(t, a);
}

__forceinline vec3f sample_anim(const AnimChanPoint3 *anim, int time, vec3f default_value)
{
  if (!anim)
    return default_value;
  float alpha = 0;
  auto *k = anim->findKey(time, &alpha);
  return (alpha != 0.f) ? interp_key(k[0], v_splats(alpha)) : k->p;
}

__forceinline quat4f sample_anim(const AnimChanQuat *anim, int time, quat4f default_value)
{
  if (!anim)
    return default_value;
  float alpha = 0;
  auto *k = anim->findKey(time, &alpha);
  return (alpha != 0.f) ? interp_key(k[0], k[1], alpha) : k->p;
}


template <typename CFG>
__forceinline vec3f PrsAnimSampler<CFG>::samplePos(dag::Index16 node_id) const
{
  if (!anim || !node_id)
    return v_zero();
  return sample_anim(get_point3_anim(anim->pos, node_id), curTime, v_zero());
}

template <typename CFG>
__forceinline vec3f PrsAnimSampler<CFG>::sampleScl(dag::Index16 node_id) const
{
  if (!anim || !node_id)
    return V_C_ONE;
  return sample_anim(get_point3_anim(anim->scl, node_id), curTime, V_C_ONE);
}

template <typename CFG>
__forceinline quat4f PrsAnimSampler<CFG>::sampleRot(dag::Index16 node_id) const
{
  if (!anim || !node_id)
    return V_C_UNIT_0001;
  return sample_anim(get_quat_anim(anim->rot, node_id), curTime, V_C_UNIT_0001);
}

template <typename CFG>
template <typename F>
__forceinline void PrsAnimSampler<CFG>::forEachSclKey(dag::Index16 node_id, F callback)
{
  if (!anim)
    return;
  auto a = get_point3_anim(anim->scl, node_id);
  if (!a)
    return;
  for (int ki = 0; ki < a->keyNum; ki++)
  {
    seekTicks(a->keyTime(ki));
    callback();
  }
}


template <typename CFG>
void PrsAnimNodeSampler<CFG>::init(const AnimV20::AnimData::Anim *a, dag::Index16 p, dag::Index16 r, dag::Index16 s)
{
  if constexpr (k_one_shot)
  {
    G_ASSERT_EX(false, "can't init() one-shot PrsAnimNodeSampler");
  }
  else
  {
    if (a)
    {
      ctx.rot = get_quat_anim(a->rot, r);
      ctx.pos = get_point3_anim(a->pos, p);
      ctx.scl = get_point3_anim(a->scl, s);
    }
    else
    {
      ctx.rot = nullptr;
      ctx.pos = nullptr;
      ctx.scl = nullptr;
    }
    ctx.curTime = 0;
  }
}

template <typename CFG>
__forceinline void PrsAnimNodeSampler<CFG>::initOneShot(const AnimV20::AnimData::Anim *a, dag::Index16 p, dag::Index16 r,
  dag::Index16 s, int ticks)
{
  if constexpr (k_one_shot)
  {
    if (a)
    {
      ctx.krot = find_key(get_quat_anim(a->rot, r), ticks, &ctx.arot);
      ctx.kpos = find_key(get_point3_anim(a->pos, p), ticks, &ctx.apos);
      ctx.kscl = find_key(get_point3_anim(a->scl, s), ticks, &ctx.ascl);
    }
    else
    {
      ctx.krot = nullptr;
      ctx.kpos = nullptr;
      ctx.kscl = nullptr;
    }
  }
  else
  {
    init(a, p, r, s);
    setTimeTicks(ticks);
  }
}

template <typename CFG>
__forceinline vec3f PrsAnimNodeSampler<CFG>::samplePos() const
{
  if constexpr (k_one_shot)
  {
    if (!ctx.kpos)
      return v_zero();
    return *(int *)&ctx.apos ? interp_key(ctx.kpos[0], v_splats(ctx.apos)) : ctx.kpos->p;
  }
  else
    return sample_anim(ctx.pos, ctx.curTime, v_zero());
}

template <typename CFG>
__forceinline vec3f PrsAnimNodeSampler<CFG>::sampleScl() const
{
  if constexpr (k_one_shot)
  {
    if (!ctx.kscl)
      return V_C_ONE;
    return *(int *)&ctx.ascl ? interp_key(ctx.kscl[0], v_splats(ctx.ascl)) : ctx.kscl->p;
  }
  else
    return sample_anim(ctx.scl, ctx.curTime, V_C_ONE);
}

template <typename CFG>
__forceinline quat4f PrsAnimNodeSampler<CFG>::sampleRot() const
{
  if constexpr (k_one_shot)
  {
    if (!ctx.krot)
      return V_C_UNIT_0001;
    return *(int *)&ctx.arot ? interp_key(ctx.krot[0], ctx.krot[1], ctx.arot) : ctx.krot->p;
  }
  else
    return sample_anim(ctx.rot, ctx.curTime, V_C_UNIT_0001);
}


template <typename CFG>
void Float3AnimNodeSampler<CFG>::init(const AnimV20::AnimDataChan &anim, dag::Index16 node)
{
  if constexpr (k_one_shot)
  {
    G_ASSERT_EX(false, "can't init() one-shot Float3AnimNodeSampler");
  }
  else
  {
    ctx.chan = get_point3_anim(anim, node);
    ctx.curTime = 0;
  }
}

template <typename CFG>
__forceinline void Float3AnimNodeSampler<CFG>::initOneShot(const AnimV20::AnimDataChan &anim, dag::Index16 node, int ticks)
{
  if constexpr (k_one_shot)
  {
    ctx.key = find_key(get_point3_anim(anim, node), ticks, &ctx.alpha);
  }
  else
  {
    init(anim, node);
    setTimeTicks(ticks);
  }
}

template <typename CFG>
__forceinline vec3f Float3AnimNodeSampler<CFG>::sample(vec3f default_value) const
{
  if constexpr (k_one_shot)
  {
    if (!ctx.key)
      return default_value;
    return *(int *)&ctx.alpha ? interp_key(ctx.key[0], v_splats(ctx.alpha)) : ctx.key->p;
  }
  else
    return sample_anim(ctx.chan, ctx.curTime, default_value);
}

} // end of namespace AnimV20Math


__forceinline bool AnimV20::AnimDataChan::hasKeys(dag::Index16 node_id) const
{
  if (node_id.index() >= nodeNum)
    return false;
  return ((const AnimV20Math::AnimChanPoint3 *)nodeAnim.get() + node_id.index())->keyNum > 0;
}

__forceinline bool AnimV20::AnimDataChan::hasAnimation(dag::Index16 node_id) const
{
  if (node_id.index() >= nodeNum)
    return false;
  return ((const AnimV20Math::AnimChanPoint3 *)nodeAnim.get() + node_id.index())->keyNum > 1;
}

__forceinline unsigned AnimV20::AnimDataChan::getNumKeys(dag::Index16 node_id) const
{
  if (node_id.index() >= nodeNum)
    return 0;
  return ((const AnimV20Math::AnimChanPoint3 *)nodeAnim.get() + node_id.index())->keyNum;
}

inline int AnimV20::AnimDataChan::keyTimeFirst(dag::Index16 node_id) const
{
  if (node_id.index() >= nodeNum)
    return INT_MAX;
  auto a = (const AnimV20Math::AnimChanPoint3 *)nodeAnim.get() + node_id.index();
  if (a->keyNum == 0)
    return INT_MAX;
  return a->keyTimeFirst();
}

inline int AnimV20::AnimDataChan::keyTimeLast(dag::Index16 node_id) const
{
  if (node_id.index() >= nodeNum)
    return INT_MIN;
  auto a = (const AnimV20Math::AnimChanPoint3 *)nodeAnim.get() + node_id.index();
  if (a->keyNum == 0)
    return INT_MIN;
  return a->keyTimeLast();
}


inline int AnimV20::PrsAnimNodeRef::keyTimeFirst() const
{
  if (!anim)
    return INT_MAX;
  int t = INT_MAX;
  if (auto a = AnimV20Math::get_point3_anim(anim->pos, posId))
    t = min(t, a->keyTimeFirst());
  if (auto a = AnimV20Math::get_quat_anim(anim->rot, rotId))
    t = min(t, a->keyTimeFirst());
  if (auto a = AnimV20Math::get_point3_anim(anim->scl, sclId))
    t = min(t, a->keyTimeFirst());
  return t;
}

inline int AnimV20::PrsAnimNodeRef::keyTimeLast() const
{
  if (!anim)
    return INT_MIN;
  int t = INT_MIN;
  if (auto a = AnimV20Math::get_point3_anim(anim->pos, posId))
    t = max(t, a->keyTimeLast());
  if (auto a = AnimV20Math::get_quat_anim(anim->rot, rotId))
    t = max(t, a->keyTimeLast());
  if (auto a = AnimV20Math::get_point3_anim(anim->scl, sclId))
    t = max(t, a->keyTimeLast());
  return t;
}

inline float AnimV20::PrsAnimNodeRef::getDuration() const
{
  if (!anim)
    return 0;
  return keyTimeLast() / float(AnimV20::TIME_TicksPerSec);
}
