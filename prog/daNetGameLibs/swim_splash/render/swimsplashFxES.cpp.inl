// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/anim/anim.h>
#include <ecs/delayedAct/actInThread.h>
#include <gamePhys/collision/collisionLib.h>
#include <render/fx/fx.h>
#include <camera/sceneCam.h>
#include <game/capsuleApproximation.h>

ECS_UNICAST_EVENT_TYPE(EventSwimSplash, int, Point3, float) /*[0] int (fx id), [1] Point3 (worldPos), float (scale) */
ECS_REGISTER_EVENT(EventSwimSplash, int, Point3, float)

struct NodeTracker
{
  dag::Index16 nodeIdx;
  Point3 prevPos;
  float timeSinceLastSplash = 100000.0f;

  NodeTracker(dag::Index16 node_idx, const Point3 &pos) : nodeIdx(node_idx) { prevPos = pos; }

  bool updateTrace(const Point3 &curr_pos, float dt, float min_t, Point3 &intersect, float &speed)
  {
    timeSinceLastSplash += dt;
    Point3 deltaPos = curr_pos - prevPos;
    float prevHeight = 0.0f, currHeight = 0.0f;
    bool doSwimSplash = dt > 1e-5f && fabs(deltaPos.y) > 1e-5f && timeSinceLastSplash > min_t &&
                        dacoll::traceht_water(prevPos, prevHeight) && dacoll::traceht_water(curr_pos, currHeight);
    if (doSwimSplash)
    {
      prevHeight = prevPos.y - prevHeight;
      currHeight = curr_pos.y - currHeight;
      doSwimSplash = sign(prevHeight) != sign(currHeight);
      if (doSwimSplash)
      {
        speed = length(deltaPos) / dt;
        intersect = curr_pos - Point3(0.0f, 1.0f, 0.0f) * currHeight;
        timeSinceLastSplash = 0.0f;
      }
    }
    prevPos = curr_pos;
    return doSwimSplash;
  }
};

struct SwimSplash
{
  Tab<NodeTracker> nodes;
  Point3 offset;
  int fxId = -1;

  void update(
    ecs::EntityId eid, const AnimV20::AnimcharBaseComponent &animchar, float dt, float scale_mul, float min_t, float max_speed)
  {
    if (fxId < 0)
      return;

    const GeomNodeTree &geomTree = animchar.getNodeTree();
    for (int i = 0; i < nodes.size(); ++i)
    {
      Point3 wpos = geomTree.getNodeWposScalar(nodes[i].nodeIdx);
      ANIMCHAR_VERIFY_NODE_POS_S(wpos, nodes[i].nodeIdx, animchar);
      Point3 intersect;
      float speed;
      if (nodes[i].updateTrace(wpos, dt, min_t, intersect, speed))
      {
        if (speed >= max_speed)
          return;
        float scale = powf(speed, 2.0f / 3.0f) * scale_mul; // energy of impact scales with speed^2 and water mass with scale^3
        Point3 worldPos = intersect + offset * scale;
        g_entity_mgr->sendEvent(eid, EventSwimSplash(fxId, worldPos, scale));
      }
    }
  }

  bool onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    const ecs::string *fxName = mgr.getNullable<ecs::string>(eid, ECS_HASH("swimsplash__fx"));
    G_ASSERT_RETURN(fxName != nullptr, false);
    if (*fxName->c_str())
      fxId = acesfx::get_type_by_name(fxName->c_str());
    if (*fxName->c_str() && fxId < 0)
      logerr("%s: failed to resolve acesfx for %s=<%s>", __FUNCTION__, "swimsplash.fx", fxName->c_str());
    if (fxId < 0)
      return false;

    const AnimV20::AnimcharBaseComponent &animChar = mgr.get<AnimV20::AnimcharBaseComponent>(eid, ECS_HASH("animchar"));
    const GeomNodeTree &tree = animChar.getNodeTree();
    const CapsuleApproximation &capsuleApproximation =
      mgr.get<ecs::SharedComponent<CapsuleApproximation>>(eid, ECS_HASH("capsule_approximation"));
    for (const CapsuleData &capsule : capsuleApproximation.capsuleDatas)
    {
      nodes.push_back(NodeTracker(capsule.nodeIndex, tree.getNodeWposScalar(capsule.nodeIndex)));
    }

    offset = mgr.get<Point3>(eid, ECS_HASH("swimsplash__offset"));

    return true;
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(SwimSplash);
ECS_REGISTER_RELOCATABLE_TYPE(SwimSplash, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(SwimSplash, "swimsplash", nullptr, 0, "animchar", "capsule_approximation");


ECS_REQUIRE(SwimSplash &swimsplash)
static inline void swim_spash_fx_es_event_handler(const EventSwimSplash &evt)
{
  int fxId = evt.get<0>();
  const Point3 &worldPos = evt.get<1>();
  float scale = evt.get<2>();
  acesfx::start_effect_pos(fxId, worldPos, true, scale);
}

ECS_TAG(render)
ECS_AFTER(start_async_phys_sim_es) // after start_async_phys_sim_es to start phys sim job earlier
ECS_REQUIRE(eastl::true_type animchar__visible = true)
__forceinline void swimsplash_es(const ParallelUpdateFrameDelayed &info,
  ecs::EntityId eid,
  SwimSplash &swimsplash,
  const AnimV20::AnimcharBaseComponent &animchar,
  const TMatrix &transform,
  float swimsplash__maxDistanceFromCameraToUpdate,
  float swimsplash__scaleMul,
  float swimsplash__minTimeBetweenSplashes,
  float swimsplash__maxRenderingSpeed)
{
  const Point3 camPos = get_cam_itm().getcol(3);

  if (swimsplash__maxDistanceFromCameraToUpdate > 0.0f &&
      lengthSq(camPos - transform.getcol(3)) > sqr(swimsplash__maxDistanceFromCameraToUpdate))
    return;
  swimsplash.update(eid, animchar, info.dt, swimsplash__scaleMul, swimsplash__minTimeBetweenSplashes, swimsplash__maxRenderingSpeed);
}
