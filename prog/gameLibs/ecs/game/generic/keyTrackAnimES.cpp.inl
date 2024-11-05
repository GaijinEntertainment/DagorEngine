// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityComponent.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/updateStage.h>
#include <ecs/core/attributeEx.h>
#include <math/dag_TMatrix.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_mathAng.h>
#include <ecs/game/generic/keyTrackAnim.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

#define ECS_DECL_ANIM_EVENT(x, ...) ECS_REGISTER_EVENT_NS(key_track_anim, x)
ECS_DECL_ALL_ANIM_EVENTS
#undef ECS_DECL_ANIM_EVENT

using namespace key_track_anim;

template <class T, class DecodedKey>
struct NoDecodingDecoder
{
  DecodedKey operator()(const T &v) const { return v; }
};

template <class T, class DecodedKey = T, typename Decoder = NoDecodingDecoder<T, DecodedKey>>
class KeyTrack
{
public:
  float getLastTime() const
  {
    if (!keys.size())
      return 0;
    return keys.back().time;
  }
  void optimize()
  {
    if (animType == ANIM_CYCLIC || cKey == 0 || keys.size() == 2) // for extrapolation we need two keys
      return;
    // remove old keys
    int removeKey = cKey - 1;
    keys.erase(keys.begin(), keys.begin() + removeKey); // remove old keys
    cKey -= removeKey;
    lastCKey -= removeKey;
  }
  void setAnimType(AnimType anim) { animType = anim; }
  void reset()
  {
    keys.resize(0);
    cKey = 0;
    lastCKey = -2;
  }
  void insertKey(const T &v, float time)
  {
    auto it = eastl::upper_bound(keys.begin(), keys.end(), time, [](float time, const KeyTime &a) { return time < a.time; });
    if (it == keys.end())
      keys.push_back(KeyTime({v, time}));
    else
    {
      int dist = eastl::distance(keys.begin(), it);
      if (abs(lastCKey - dist) <= 1) // invalidate for extrapolation
        lastCKey = -3;
      if (it->time == time)
      {
        it->value = v;
        return;
      }
      if (dist < cKey)
      {
        cKey++;
        lastCKey++;
      }
      keys.insert(it, KeyTime({v, time}));
    }
  }

  bool decodeNextKey(float time, float &relTime)
  {
    if (!advance(time))
      return false;

    int next = cKey + 1;

    if (lastCKey != cKey)
    {
      if (lastCKey == cKey - 1)
        curKey = nextKey;
      else
        curKey = cDecoder(keys[cKey].value);

      lastCKey = cKey;

      if (next == keys.size())
      {
        nextKey = curKey;
        if (animType == ANIM_EXTRAPOLATED)
          curKey = cDecoder(keys[max(0, cKey - 1)].value);
      }
      else
        nextKey = cDecoder(keys[cKey + 1].value);
    }

    float cTime = keys[cKey].time;
    if (next == keys.size()) // beyond and of time
    {
      if (animType != ANIM_EXTRAPOLATED || cKey == 0)
        relTime = 0;
      else
        relTime = max(0.f, safediv(time - keys[cKey - 1].time, cTime - keys[cKey - 1].time));
    }
    else
      relTime = max(0.f, safediv(time - cTime, keys[next].time - cTime));

    return true;
  }
  bool isFinished() const { return keys.size() <= 1 || (animType == ANIM_SINGLE && cKey == keys.size() - 1); }
  T getKey(const T &current, float time)
  {
    float relTime;
    if (!decodeNextKey(time, relTime))
      return current;
    // debug("cKey = %d, key=%@, key2=%@ tm = %f == %@", cKey, curKey, nextKey, relTime, lerp(curKey, nextKey, relTime));
    optimize();
    return lerp(curKey, nextKey, relTime);
  }

protected:
  bool advance(float &time)
  {
    if (!keys.size())
      return false;

    if (keys.size() == 1)
    {
      cKey = 0;
      return true;
    }

    if (animType == ANIM_CYCLIC && keys.size() != 1)
    {
      time = keys.front().time + fmodf(time - keys.front().time, keys.back().time - keys.front().time);
      G_ASSERT(time < keys.back().time);
    }

    if (((uint32_t)cKey) >= keys.size()) // sanity check, shant happen
      cKey = 0;
    if (cKey > 0 && time < keys[cKey].time) // time went backwards, cyclic animation or should never happen else
      cKey = 0;

    for (; cKey < (int)keys.size() - 1; ++cKey)
      if (keys[cKey + 1].time >= time)
        break;

    return true;
  }
  struct KeyTime
  {
    T value;
    float time;
  };
  eastl::vector<KeyTime> keys;
  DecodedKey curKey, nextKey;
  int cKey = 0, lastCKey = -2;
  AnimType animType = ANIM_SINGLE;
  Decoder cDecoder;
};

struct DegEulerToRad
{
  quat4f operator()(const Point3 &v) const
  {
    quat4f q;
    euler_to_quat(DegToRad(v.x), DegToRad(v.y), DegToRad(v.z), (Quat &)q);
    return q;
  }
};

class RotKeyTrack : public KeyTrack<quat4f> // yaw pitch raw KeyTrack<Point3, quat4f, DegEulerToRad>//yaw pitch raw
{
public:
  Matrix3 getKey(const Matrix3 &current, float time)
  {
    float relTime;
    if (!decodeNextKey(time, relTime))
      return current;

    optimize();
    quat4f result;
    if (relTime <= 1.0f)
      result = v_quat_qslerp(relTime, curKey, nextKey);
    else
      result = v_quat_slerp(v_splats(relTime), curKey, nextKey);

    alignas(16) Matrix3 ret;
    mat33f vRet;
    v_mat33_from_quat(vRet, result);
    v_mat_33cu_from_mat33(ret.m[0], vRet);
    return ret;
  }
};

struct AnimKeyTrackTransform
{
  RotKeyTrack rot;
  KeyTrack<Point3> pos;
};

ECS_DECLARE_RELOCATABLE_TYPE(AnimKeyTrackTransform);
ECS_REGISTER_RELOCATABLE_TYPE(AnimKeyTrackTransform, nullptr);
ECS_AUTO_REGISTER_COMPONENT(AnimKeyTrackTransform, "anim_key_track", nullptr, 0);

static __forceinline void key_track_anim_es(const ecs::UpdateStageInfoAct &info, TMatrix &transform,
  AnimKeyTrackTransform &anim_key_track, bool &anim_track_on ECS_REQUIRE(eastl::true_type anim_track_on))
{
  (*(Matrix3 *)(char *)&transform) = anim_key_track.rot.getKey((*(Matrix3 *)(char *)&transform), info.curTime);
  transform.setcol(3, anim_key_track.pos.getKey(transform.getcol(3), info.curTime));
  if (anim_key_track.pos.isFinished() && anim_key_track.rot.isFinished())
    anim_track_on = false;
}

template <class Key, class T>
__forceinline void reset_anim(const Key &key, float time, int animType, T &track)
{
  track.reset();
  track.setAnimType((AnimType)clamp(animType, (int)ANIM_SINGLE, (int)ANIM_EXTRAPOLATED));
  track.insertKey(key, time);
}

template <class Key, class T>
__forceinline void add_key(const Key &key, float time, bool isAdditional, T &track)
{
  track.insertKey(key, time + (isAdditional ? track.getLastTime() : 0));
}

inline void key_track_change_anim_es_event_handler(const CmdResetPosAnim &evt, AnimKeyTrackTransform &anim_key_track,
  bool &anim_track_on)
{
  reset_anim(evt.get<0>(), evt.get<1>(), evt.get<2>(), anim_key_track.pos);
  anim_track_on = true;
}

inline void key_track_change_anim_es_event_handler(const CmdResetRotAnim &evt, AnimKeyTrackTransform &anim_key_track,
  bool &anim_track_on)
{
  reset_anim(v_ldu(&evt.get<0>().x), evt.get<1>(), evt.get<2>(), anim_key_track.rot);
  anim_track_on = true;
}

inline void key_track_change_anim_es_event_handler(const CmdAddPosAnim &evt, AnimKeyTrackTransform &anim_key_track,
  bool &anim_track_on)
{
  add_key(evt.get<0>(), evt.get<1>(), evt.get<2>(), anim_key_track.pos);
  anim_track_on = true;
}

inline void key_track_change_anim_es_event_handler(const CmdAddRotAnim &evt, AnimKeyTrackTransform &anim_key_track,
  bool &anim_track_on)
{
  add_key(v_ldu(&evt.get<0>().x), evt.get<1>(), evt.get<2>(), anim_key_track.rot);
  anim_track_on = true;
}

inline void key_track_change_anim_es_event_handler(const CmdStopAnim &, bool &anim_track_on) { anim_track_on = false; }

template <class KeyTrack, class Type>
struct AttrKeyTrack
{
  typedef Type key_type;
  ecs::component_t compName;
  ecs::HashedConstString getCompName() const { return ecs::HashedConstString{nullptr, compName}; }
  KeyTrack track;
  bool isAnimated = false;
  void setName(const ecs::component_t n) { compName = n; }
  inline void animate(const ecs::UpdateStageInfoAct &info, ecs::EntityId eid)
  {
    if (!isAnimated)
      return;
    if (!g_entity_mgr->has(eid, getCompName()))
    {
      isAnimated = false;
      return;
    }
    auto &v = g_entity_mgr->template getRW<Type>(eid, getCompName());
    v = track.getKey(v, info.curTime);
    isAnimated = !track.isFinished();
  }
};

typedef AttrKeyTrack<KeyTrack<float>, float> AttrAnimKeyTrackFloat;


ECS_DECLARE_RELOCATABLE_TYPE(AttrAnimKeyTrackFloat);
ECS_REGISTER_RELOCATABLE_TYPE(AttrAnimKeyTrackFloat, nullptr);
ECS_AUTO_REGISTER_COMPONENT(AttrAnimKeyTrackFloat, "anim_float_attr", nullptr, 0);

static __forceinline void attr_float_track_anim_es(const ecs::UpdateStageInfoAct &info, ecs::EntityId eid,
  AttrAnimKeyTrackFloat &anim_float_attr)
{
  anim_float_attr.animate(info, eid);
}

inline void attr_float_key_track_change_anim_es_event_handler(const CmdResetAttrFloatAnim &evt, ecs::EntityId eid,
  AttrAnimKeyTrackFloat &anim_float_attr)
{
  anim_float_attr.setName(evt.get<0>());
  if (auto attr = g_entity_mgr->getNullable<float>(eid, anim_float_attr.getCompName()))
  {
    reset_anim(*attr, evt.get<1>(), evt.get<2>(), anim_float_attr.track);
    anim_float_attr.isAnimated = true;
  }
}

inline void attr_float_key_track_change_anim_es_event_handler(const CmdAddAttrFloatAnim &evt, ecs::EntityId eid,
  AttrAnimKeyTrackFloat &anim_float_attr)
{
  if (g_entity_mgr->is<float>(eid, anim_float_attr.getCompName())) // we assume component can be removed. May be better listen for
                                                                   // event?
  {
    add_key(evt.get<0>(), evt.get<1>(), evt.get<2>(), anim_float_attr.track);
    anim_float_attr.isAnimated = true;
  }
}
