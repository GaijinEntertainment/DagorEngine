//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_stdint.h>
#include <anim/dag_animChannels.h>
#include <debug/dag_assert.h>

#if DAGOR_DBGLEVEL >= 1
#define ACL_ON_ASSERT_CUSTOM
#define ACL_ASSERT(expression, format, ...) G_ASSERTF(expression, format, ##__VA_ARGS__)
#endif

#include <rtm/math.h>
#include <acl/core/compressed_tracks.h>
#include <acl/decompression/decompress.h>

template <>
struct DebugConverter<acl::compressed_tracks_version16>
{
  static unsigned getDebugType(const acl::compressed_tracks_version16 &v) { return unsigned(v); }
};


namespace AnimV20Math
{


inline int seconds_to_ticks(float seconds) { return seconds < VERY_BIG_NUMBER ? roundf(seconds * AnimV20::TIME_TicksPerSec) : 0; }
inline float ticks_to_seconds(int ticks) { return ticks / float(AnimV20::TIME_TicksPerSec); }


struct transform_decompression_settings : public acl::default_transform_decompression_settings
{
  static constexpr acl::compressed_tracks_version16 version_supported() { return acl::compressed_tracks_version16::latest; }
  static constexpr bool is_wrapping_supported() { return false; }
};

struct float3_decompression_settings : public acl::default_scalar_decompression_settings
{
  static constexpr acl::compressed_tracks_version16 version_supported() { return acl::compressed_tracks_version16::latest; }
  static constexpr bool is_wrapping_supported() { return false; }
  static constexpr bool is_track_type_supported(acl::track_type8 type) { return type == acl::track_type8::float3f; }
};


struct AnimSamplerConfig
{
  // If true, sampling time must be specified in the constructor call.
  static constexpr bool is_one_shot() { return false; }

  using transform_settings = transform_decompression_settings;
  using float3_settings = float3_decompression_settings;
};

struct DefaultConfig : AnimSamplerConfig
{};

struct OneShotConfig : AnimSamplerConfig
{
  static constexpr bool is_one_shot() { return true; }
};


struct TransformWriter : public acl::track_writer
{
  static constexpr acl::default_sub_track_mode get_default_scale_mode() { return acl::default_sub_track_mode::constant; }
};

struct FullTransformWriter : public TransformWriter
{
  vec3f *p, *s;
  quat4f *r;

  FullTransformWriter(vec3f *p_, quat4f *r_, vec3f *s_) : p(p_), r(r_), s(s_) {}

  void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation)
  {
    G_UNUSED(track_index);
    if (r)
      *r = rotation;
  }

  void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation)
  {
    G_UNUSED(track_index);
    if (p)
      *p = translation;
  }

  void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale)
  {
    G_UNUSED(track_index);
    if (s)
      *s = scale;
  }
};

template <acl::animation_track_type8 ST>
struct SubtrackWriter : public TransformWriter
{
  typename eastl::conditional_t<ST == acl::animation_track_type8::rotation, quat4f, vec3f> value;

  static constexpr bool skip_all_rotations() { return ST != acl::animation_track_type8::rotation; }
  static constexpr bool skip_all_translations() { return ST != acl::animation_track_type8::translation; }
  static constexpr bool skip_all_scales() { return ST != acl::animation_track_type8::scale; }

  void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation)
  {
    G_UNUSED(track_index);
    G_UNUSED(rotation);
    if (ST == acl::animation_track_type8::rotation)
      value = rotation;
  }

  void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation)
  {
    G_UNUSED(track_index);
    G_UNUSED(translation);
    if (ST == acl::animation_track_type8::translation)
      value = translation;
  }

  void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale)
  {
    G_UNUSED(track_index);
    G_UNUSED(scale);
    if (ST == acl::animation_track_type8::scale)
      value = scale;
  }
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
  PrsAnimSampler(const AnimV20::AnimData *ad = nullptr)
  {
    ONE_SHOT_CTOR_CHECK();
    init(ad);
  }

  explicit PrsAnimSampler(const AnimV20::AnimData *ad, int time_ticks)
  {
    init(ad);
    seekTicks(time_ticks);
  }

  explicit PrsAnimSampler(const AnimV20::AnimData *ad, float time_seconds)
  {
    init(ad);
    seek(time_seconds);
  }

  void seekTicks(int ticks) { seek(ticks_to_seconds(ticks)); }

  void seek(float seconds) { ctx.seek(seconds, acl::sample_rounding_policy::none); }

  // seek to each key time, callback is (PrsAnimSampler &self, float seconds) -> void
  template <typename F>
  void forEachSclKey(F callback)
  {
    auto tracks = ctx.get_compressed_tracks();
    if (!tracks)
      return;
    const float sr = tracks->get_sample_rate();
    for (uint32_t i = 0, n = tracks->get_num_samples_per_track(); i < n; i++)
      callback(*this, i / sr);
  }

  void sampleTransform(dag::Index16 track_id, vec3f *p, quat4f *r, vec3f *s)
  {
    FullTransformWriter wr(p, r, s);
    ctx.decompress_track(track_id.index(), wr);
  }

  vec3f samplePosTrack(dag::Index16 track_id)
  {
    SubtrackWriter<acl::animation_track_type8::translation> wr;
    ctx.decompress_track(track_id.index(), wr);
    return wr.value;
  }

  quat4f sampleRotTrack(dag::Index16 track_id)
  {
    SubtrackWriter<acl::animation_track_type8::rotation> wr;
    ctx.decompress_track(track_id.index(), wr);
    return wr.value;
  }

  vec3f sampleSclTrack(dag::Index16 track_id)
  {
    SubtrackWriter<acl::animation_track_type8::scale> wr;
    ctx.decompress_track(track_id.index(), wr);
    return wr.value;
  }

protected:
  static constexpr bool k_one_shot = CFG::is_one_shot();

  acl::decompression_context<typename CFG::transform_settings> ctx;

  void init(const AnimV20::AnimData *ad)
  {
    if (ad && ad->anim.rot.animTracks)
      G_VERIFY(ctx.initialize(*ad->anim.rot.animTracks));
  }
};


// For samping single node.
template <typename CFG>
struct PrsAnimNodeSampler
{
  PrsAnimNodeSampler()
  {
    ONE_SHOT_CTOR_CHECK();
    init(nullptr, dag::Index16());
  }

  PrsAnimNodeSampler(const AnimV20::PrsAnimNodeRef &prs)
  {
    ONE_SHOT_CTOR_CHECK();
    init(prs.anim, prs.trackId);
  }

  PrsAnimNodeSampler(const AnimV20::AnimData *a, dag::Index16 track_id)
  {
    ONE_SHOT_CTOR_CHECK();
    init(a ? a->anim.rot.animTracks.get() : nullptr, track_id);
  }

  PrsAnimNodeSampler(const AnimV20::AnimData *a, dag::Index16 track_id, int ticks)
  {
    initOneShot(a ? a->anim.rot.animTracks.get() : nullptr, track_id, ticks_to_seconds(ticks));
  }

  explicit PrsAnimNodeSampler(const AnimV20::PrsAnimNodeRef &prs, int ticks)
  {
    initOneShot(prs.anim, prs.trackId, ticks_to_seconds(ticks));
  }

  explicit PrsAnimNodeSampler(const AnimV20::PrsAnimNodeRef &prs, float seconds) { initOneShot(prs.anim, prs.trackId, seconds); }

  void seekTicks(int ticks) { seek(ticks_to_seconds(ticks)); }

  void seek(float seconds)
  {
    ONE_SHOT_SEEK_CHECK();
    ctx.dc.seek(seconds, acl::sample_rounding_policy::none);
  }

  void sampleTransform(vec3f *p, quat4f *r, vec3f *s)
  {
    if constexpr (k_one_shot)
    {
      *p = ctx.p;
      *r = ctx.r;
      *s = ctx.s;
    }
    else
    {
      FullTransformWriter wr(p, r, s);
      ctx.dc.decompress_track(ctx.trackId.index(), wr);
    }
  }

  vec3f samplePos()
  {
    if constexpr (k_one_shot)
      return ctx.p;
    else
    {
      SubtrackWriter<acl::animation_track_type8::translation> wr;
      ctx.dc.decompress_track(ctx.trackId.index(), wr);
      return wr.value;
    }
  }

  quat4f sampleRot()
  {
    if constexpr (k_one_shot)
      return ctx.r;
    else
    {
      SubtrackWriter<acl::animation_track_type8::rotation> wr;
      ctx.dc.decompress_track(ctx.trackId.index(), wr);
      return wr.value;
    }
  }

  vec3f sampleScl()
  {
    if constexpr (k_one_shot)
      return ctx.s;
    else
    {
      SubtrackWriter<acl::animation_track_type8::scale> wr;
      ctx.dc.decompress_track(ctx.trackId.index(), wr);
      return wr.value;
    }
  }

protected:
  static constexpr bool k_one_shot = CFG::is_one_shot();

  struct Context
  {
    acl::decompression_context<typename CFG::transform_settings> dc;
    dag::Index16 trackId;
  };

  struct OneShot
  {
    vec4f p, r, s;
  };

  typename eastl::conditional_t<k_one_shot, OneShot, Context> ctx;

  void init(const AnimV20::AnimChan *a, dag::Index16 track_id)
  {
    ctx.trackId = track_id;
    if (a)
      G_VERIFY(ctx.dc.initialize(*a));
  }

  void initOneShot(const AnimV20::AnimChan *a, dag::Index16 track_id, float seconds)
  {
    if (a)
    {
      acl::decompression_context<typename CFG::transform_settings> dc;
      G_VERIFY(dc.initialize(*a));
      dc.seek(seconds, acl::sample_rounding_policy::none);
      FullTransformWriter wr(&ctx.p, &ctx.r, &ctx.s);
      dc.decompress_track(track_id.index(), wr);
    }
    else
    {
      ctx.p = v_zero();
      ctx.r = V_C_UNIT_0001;
      ctx.s = V_C_ONE;
    }
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
    init(anim, node);
    seekTicks(ticks);
  }

  Float3AnimNodeSampler(const AnimV20::AnimDataChan &anim, float seconds, dag::Index16 node = dag::Index16(0))
  {
    init(anim, node);
    seek(seconds);
  }

  void seekTicks(int ticks) { seek(ticks_to_seconds(ticks)); }

  void seek(float seconds) { ctx.seek(seconds, acl::sample_rounding_policy::none); }

  vec3f sample(vec3f default_value = v_zero())
  {
    Writer wr(default_value);
    ctx.decompress_track(trackId.index(), wr);
    return wr.value;
  }

protected:
  static constexpr bool k_one_shot = CFG::is_one_shot();

  acl::decompression_context<typename CFG::float3_settings> ctx;
  dag::Index16 trackId;

  struct Writer : public acl::track_writer
  {
    vec3f value;
    Writer(vec3f v) : value(v) {}

    void RTM_SIMD_CALL write_float3(uint32_t track_index, rtm::vector4f_arg0 v)
    {
      G_UNUSED(track_index);
      value = v;
    }
  };

  void init(const AnimV20::AnimDataChan &anim, dag::Index16 node)
  {
    trackId = anim.getTrackId(node);
    if (anim.animTracks)
      G_VERIFY(ctx.initialize(*anim.animTracks));
  }
};

} // end of namespace AnimV20Math


__forceinline bool AnimV20::AnimDataChan::hasKeys() const
{
  if (!animTracks)
    return false;
  return animTracks->get_num_samples_per_track() > 0;
}

__forceinline bool AnimV20::AnimDataChan::hasAnimation() const
{
  if (!animTracks)
    return false;
  return animTracks->get_num_samples_per_track() > 1;
}

__forceinline unsigned AnimV20::AnimDataChan::getNumKeys() const
{
  if (!animTracks)
    return 0;
  return animTracks->get_num_samples_per_track();
}

inline int AnimV20::AnimDataChan::keyTimeLast() const
{
  if (!animTracks)
    return 0;
  return AnimV20Math::seconds_to_ticks(animTracks->get_duration());
}


inline bool AnimV20::PrsAnimNodeRef::hasAnimation() const
{
  if (!anim)
    return false;
  return anim->is_animated(trackId.index());
}

inline int AnimV20::PrsAnimNodeRef::keyTimeLast() const
{
  if (!anim)
    return 0;
  if (!anim->is_animated(trackId.index()))
    return 0;
  return AnimV20Math::seconds_to_ticks(anim->get_duration());
}

inline float AnimV20::PrsAnimNodeRef::getDuration() const
{
  if (!anim)
    return 0;
  if (!anim->is_animated(trackId.index()))
    return 0;
  return anim->get_duration();
}
