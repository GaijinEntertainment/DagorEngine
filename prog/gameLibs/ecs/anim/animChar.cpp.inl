// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/anim/anim.h>
#include <ecs/core/entitySystem.h>
#include <ecs/core/attributeEx.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/animState.h>
#include <ecs/anim/slotAttach.h>
#include <ecs/phys/physEvents.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <debug/dag_assert.h>
#include <generic/dag_relocatableFixedVector.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathUtils.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <shaders/dag_dynSceneRes.h>
#include <startup/dag_globalSettings.h>
#include <3d/dag_render.h>
#include <anim/dag_animBlend.h>
#include "render/animCharTexReplace.h"
#include <ecs/anim/animcharUpdateEvent.h>
#include <daECS/core/componentType.h>

static int harmonization_required = -1;

static inline void animchar_set_tm(AnimV20::AnimcharBaseComponent &ac, bool turn_dir, const TMatrix &tm)
{
  mat44f tm4; // construct matrix manually to avoid losing precision/perfomance to orthonormalization
  v_mat44_make_from_43cu(tm4, tm.array);
  vec4f vTurnDir = v_cast_vec4f(v_splatsi(turn_dir ? -1 : 0));
  vec3f col0 = tm4.col0;
  vec3f col2 = tm4.col2;
  tm4.col0 = v_sel(tm4.col0, col2, vTurnDir);
  tm4.col2 = v_sel(tm4.col2, v_neg(col0), vTurnDir);
  ac.setTm(tm4, /*setup_wofs*/ true);
}

static bool is_tex_harmonization_required()
{
  if (DAGOR_UNLIKELY(harmonization_required < 0))
    harmonization_required =
      ::dgs_get_settings()->getBool("harmonizationRequired", false) || ::dgs_get_settings()->getBool("harmonizationEnabled", false);
  return (bool)harmonization_required;
}

static inline void reset_animchar_resource(AnimV20::AnimcharRendComponent &animchar_render,
  const AnimV20::AnimcharBaseComponent &animchar, const char *animchar_res)
{
  GameResHandle handle = GAMERES_HANDLE_FROM_STRING(animchar_res);
  AnimV20::IAnimCharacter2 *animCharSource = (AnimV20::IAnimCharacter2 *)::get_game_resource_ex(handle, CharacterGameResClassId);
  G_ASSERTF_RETURN(animCharSource, , "%s animChar resource '%s' not found", __FUNCTION__, animchar_res);
  animCharSource->rendComp().cloneTo(&animchar_render, false, animchar.getNodeTree());
  ::release_game_resource(handle);
}

static inline void replace_animchar_resource(AnimV20::AnimcharRendComponent &animchar_render,
  const AnimV20::AnimcharBaseComponent &animchar, const char *animchar_res, const ecs::Object *objTexReplace,
  const ecs::Object *objTexSet)
{
  // object overrides singular replace
  if ((objTexReplace && !objTexReplace->empty()) || (objTexSet && !objTexSet->empty()))
  {
    AnimCharTexReplace texReplaceCtx(animchar_render, animchar_res);
    if (objTexReplace)
      for (const ecs::Object::value_type &replStrings : *objTexReplace)
        texReplaceCtx.replaceTex(replStrings.first.c_str(), replStrings.second.getStr());
    if (objTexSet)
      for (const ecs::Object::value_type &setString : *objTexSet)
      {
        ecs::Object::const_iterator setParams = setString.second.get<ecs::Object>().begin();
        texReplaceCtx.replaceTexByShaderVar(setString.first.c_str(), setParams->first.c_str(), setParams->second.getStr());
      }
    texReplaceCtx.apply(animchar, true);
  }
}

// to use template magic BoxedCreator we need appropriate constructors. So let's make type inheritance
// After we refactor it to requestResourse / create, it will be just InplaceCreator
struct AnimcharBaseConstruct final : public AnimV20::AnimcharBaseComponent
{
public:
  EA_NON_COPYABLE(AnimcharBaseConstruct);
  AnimcharBaseConstruct() = default;

  static void requestResources(const char *, const ecs::resource_request_cb_t &res_cb)
  {
    auto &res = res_cb.get<ecs::string>(ECS_HASH("animchar__res"));
    res_cb(!res.empty() ? res.c_str() : "<missing_animchar>", CharacterGameResClassId);
  }

  AnimcharBaseConstruct(ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    const char *animCharRes = mgr.getOr(eid, ECS_HASH("animchar__res"), ecs::nullstr);
    GameResHandle handle = GAMERES_HANDLE_FROM_STRING(animCharRes);
    if (DAGOR_UNLIKELY(!is_game_resource_loaded(handle, CharacterGameResClassId)))
      logerr("<%s> is expected to be loaded in %s while creating %d<%s>", animCharRes, __FUNCTION__, (ecs::entity_id_t)eid,
        mgr.getEntityTemplateName(eid)); // Note: `requestResources` should ensure that
    AnimV20::IAnimCharacter2 *animCharSource = (AnimV20::IAnimCharacter2 *)::get_game_resource_ex(handle, CharacterGameResClassId);
    if (DAGOR_UNLIKELY(!animCharSource))
    {
      logerr("Failed to get gameres '%s' while creating %d<%s>", animCharRes, (ecs::entity_id_t)eid, mgr.getEntityTemplateName(eid));
      return;
    }
    animCharSource->baseComp().cloneTo(this, false);
    ::release_game_resource(handle);


    // Don't set `eid` setTraceContext here - as might be (incorrectly) used as pointer on actual `trace_static_multiray` calls

    const TMatrix &tm = mgr.getOr(eid, ECS_HASH("transform"), TMatrix::IDENT);
    bool turnDir = mgr.getOr(eid, ECS_HASH("animchar__turnDir"), false);

    animchar_set_tm(*this, turnDir, tm);
    doRecalcAnimAndWtm();

    float charDepScale = mgr.getOr(eid, ECS_HASH("animchar__scale"), 1.0f);
    if (fabsf(charDepScale - 1) > 1e-3)
      if (!applyCharDepScale(charDepScale))
        logerr("animchar %s: cannot apply charDep scale=%.4f", animCharRes, charDepScale);
    if (mgr.has(eid, ECS_HASH("animchar__animStateNames")))
    {
      const auto &animStateNames = mgr.get<ecs::Object>(eid, ECS_HASH("animchar__animStateNames"));
      if (!animStateNames.empty())
        if (ecs::Object *animStateObj = mgr.getNullableRW<ecs::Object>(eid, ECS_HASH("animchar__animState")))
          for (auto &stateName : animStateNames)
            animStateObj->addMember(ECS_HASH_SLOW(stateName.first.c_str()), getAnimGraph()->getStateIdx(stateName.second.getStr()));
    }
  }

  bool onLoaded() const { return nodeTree.nodeCount() != 0; }
};

struct AnimcharRendConstruct final : public AnimV20::AnimcharRendComponent
{
public:
  EA_NON_COPYABLE(AnimcharRendConstruct);
  AnimcharRendConstruct() = default;

  AnimcharRendConstruct(ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    const char *animCharRes = mgr.getOr(eid, ECS_HASH("animchar__res"), ecs::nullstr);
    const AnimV20::AnimcharBaseComponent &acBase = mgr.get<AnimV20::AnimcharBaseComponent>(eid, ECS_HASH("animchar"));

    reset_animchar_resource(*this, acBase, animCharRes);
    setVisible(mgr.getOr(eid, ECS_HASH("animchar_render__enabled"), true));
    if (getVisualResource())
    {
      const ecs::Object *animCharReplObj = mgr.getNullable<ecs::Object>(eid, ECS_HASH("animchar__objTexReplace"));
      const ecs::Object *animCharSetObj = mgr.getNullable<ecs::Object>(eid, ECS_HASH("animchar__objTexSet"));
      replace_animchar_resource(*this, acBase, animCharRes, animCharReplObj, animCharSetObj);

      if (is_tex_harmonization_required())
      {
        const ecs::Object *animCharHarmonizationObj = mgr.getNullable<ecs::Object>(eid, ECS_HASH("animchar__objTexHarmonize"));
        if (animCharHarmonizationObj) // skin textures should be harmonized
        {
          const ecs::Object *animCharReplHarmObj =
            animCharHarmonizationObj->getNullable<ecs::Object>(ECS_HASH("animchar__objTexReplace"));
          const ecs::Object *animCharSetHarmObj = animCharHarmonizationObj->getNullable<ecs::Object>(ECS_HASH("animchar__objTexSet"));
          replace_animchar_resource(*this, acBase, animCharRes, animCharReplHarmObj, animCharSetHarmObj);
        }
      }
    }
    if (!getSceneInstance())
      setVisualResource(getVisualResource(), true, acBase.getNodeTree());
    mgr.setOptional<float>(eid, ECS_HASH("animchar_render__dist_sq"), getRenderDistanceSq());
  }
};

bool AnimcharNodesMat44::onLoaded(ecs::EntityManager &mgr, ecs::EntityId eid)
{
  const AnimV20::AnimcharBaseComponent &acBase = mgr.get<AnimV20::AnimcharBaseComponent>(eid, ECS_HASH("animchar"));

  clear_and_resize(nwtm, acBase.getNodeTree().nodeCount());
  acBase.copyNodesTo(*this);
  mgr.set<vec3f>(eid, ECS_HASH("animchar_render__root_pos"), nwtm[0].col3);
  return true;
}

// currently we can't use template magic, because object is actually created within onLoaded.
// After we refactor it to requestResourse / create, it will be just InplaceCreator
ECS_REGISTER_MANAGED_TYPE(AnimV20::AnimcharBaseComponent, nullptr,
  typename ecs::CreatorSelector<AnimV20::AnimcharBaseComponent ECS_COMMA AnimcharBaseConstruct>::type);
ECS_REGISTER_MANAGED_TYPE(AnimV20::AnimcharRendComponent, nullptr,
  typename ecs::CreatorSelector<AnimV20::AnimcharRendComponent ECS_COMMA AnimcharRendConstruct>::type);
ECS_REGISTER_RELOCATABLE_TYPE(AnimcharNodesMat44, nullptr);

ECS_AUTO_REGISTER_COMPONENT(ecs::Object, "animchar__animState", nullptr, 0); // as animchar writes to animState
ECS_AUTO_REGISTER_COMPONENT(ecs::string, "animchar__res", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(AnimV20::AnimcharBaseComponent, "animchar", nullptr, 0, "?animchar__animState", "animchar__res");
ECS_AUTO_REGISTER_COMPONENT_DEPS(AnimV20::AnimcharBaseComponent, "saved_animchar", nullptr, 0, "?animchar");
ECS_AUTO_REGISTER_COMPONENT_DEPS(AnimcharNodesMat44, "animchar_node_wtm", nullptr, 0, "animchar", "animchar_render",
  "animchar_render__root_pos", "animchar_render__dist_sq");
ECS_AUTO_REGISTER_COMPONENT_DEPS(AnimV20::AnimcharRendComponent, "animchar_render", nullptr, 0, "animchar");
ECS_AUTO_REGISTER_COMPONENT(vec4f, "animchar_bsph", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(vec4f, "animchar_bsph_precalculated", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(bbox3f, "animchar_bbox", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(bbox3f, "animchar_shadow_cull_bbox", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(bbox3f, "animchar_attaches_bbox", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(bbox3f, "animchar_attaches_bbox_precalculated", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(uint8_t, "animchar_visbits", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(vec3f, "animchar_render__root_pos", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(float, "animchar_render__dist_sq", nullptr, 0);
ECS_DEF_PULL_VAR(animchar);


template <typename Callable>
inline void animchar_pre_update_ecs_query(Callable c);
template <typename Callable>
// Using chunk of size 1 since animchars have extremely non-uniform update times (*) due to skipped updates.
// (*) On xb1 it's in 3us-300us range.
inline void animchar_update_ecs_query(Callable ECS_CAN_PARALLEL_FOR(c, 1));

struct AnimcharAttachRec
{
  AnimV20::AnimcharBaseComponent *animCharAttachedTo;
  int attachmentId;
#if DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_)
  ecs::EntityId attachedTo;
#endif
  AnimcharAttachRec(AnimV20::AnimcharBaseComponent *a, int aid, ecs::EntityId aeid)
  {
    animCharAttachedTo = a;
    attachmentId = aid;
    G_UNUSED(aeid);
#if DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_)
    attachedTo = aeid;
#endif
  }
  AnimcharAttachRec &operator=(const AnimcharAttachRec &) = delete;
  AnimcharAttachRec(AnimcharAttachRec &&rhs) = delete;
  ~AnimcharAttachRec()
  {
#if DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_)
    G_ASSERTF(g_entity_mgr->doesEntityExist(attachedTo), "Attached to entity %d does not exist!", (ecs::entity_id_t)attachedTo);
#endif
    animCharAttachedTo->releaseAttachment(attachmentId);
  }
};
DAG_DECLARE_RELOCATABLE(AnimcharAttachRec);

#define ANIMCHAR_ACT(dt, accum_dt, dt_threshold) \
  anim::animchar_act(dt, accum_dt, dt_threshold, animchar, animchar__turnDir, transform, animchar_node_wtm, animchar_render__root_pos)
void anim::animchar_act(float dt, float *accum_dt, const float *dt_threshold, AnimV20::AnimcharBaseComponent &animchar,
  bool animchar__turnDir, const TMatrix &transform, AnimcharNodesMat44 *animchar_node_wtm, vec3f *animchar_render__root_pos)
{
  TIME_PROFILE_DEV(animchar_act);

  animchar_set_tm(animchar, animchar__turnDir, transform);
  if (!accum_dt)
    animchar.act(dt, /*calc_anim*/ true);
  else
  {
    *accum_dt += dt;
    if (*accum_dt >= *dt_threshold)
    {
      animchar.act(*accum_dt, /*calc_anim*/ true);
      *accum_dt = 0.f;
    }
    else
    {
      TIME_PROFILE_DEV(animchar_recalc_wtm);
      animchar.recalcWtm();
    }
  }
  if (!animchar_node_wtm)
    return;

  TIME_PROFILE_DEV(animchar_copy_nodes);
  animchar_copy_nodes(animchar, *animchar_node_wtm, *animchar_render__root_pos);

#if DAECS_EXTENSIVE_CHECKS
  for (auto &n : animchar_node_wtm->nwtm)
    ANIMCHAR_VERIFY_NODE_POS(n.col3, eastl::distance(animchar_node_wtm->nwtm.data(), &n), animchar);
#endif
}

ECS_REGISTER_EVENT(UpdateAnimcharEvent);

ECS_AFTER(before_animchar_update_sync)
ECS_BEFORE(after_animchar_update_sync)
static __forceinline void animchar__updater_es(const UpdateAnimcharEvent &info)
{
  dag::RelocatableFixedVector<AnimcharAttachRec, 128, /*bOverflow*/ true, framemem_allocator> attachRecords;

  // Set attachement to the slot for update time
  {
    TIME_PROFILE_DEV(assemble_attaches);
    animchar_pre_update_ecs_query([&](ecs::EntityId eid, int slot_attach__slotId, AnimV20::AnimcharBaseComponent &animchar,
                                    ecs::EntityId slot_attach__attachedTo ECS_REQUIRE(ecs::Tag attachmentUpdate)) {
      if (slot_attach__slotId < 0)
        return;
      ECS_GET_ENTITY_COMPONENT_RW(AnimV20::AnimcharBaseComponent, animCharAttachedTo, slot_attach__attachedTo, animchar);
      if (animCharAttachedTo)
      {
        int aid = animCharAttachedTo->setAttachedChar(slot_attach__slotId, ecs::entity_id_t(eid), &animchar, /*recalcable*/ false);
        attachRecords.emplace_back(animCharAttachedTo, aid, slot_attach__attachedTo);
      }
    });
  }

  // update animchars with attachments
  animchar_update_ecs_query(
    [&](ECS_REQUIRE_NOT(ecs::Tag animchar__actOnDemand) ECS_REQUIRE_NOT(ecs::Tag animchar__physSymDependence)
          ECS_REQUIRE(eastl::true_type animchar__updatable = true) AnimV20::AnimcharBaseComponent &animchar,
      float *animchar__accumDt, const float *animchar__dtThreshold,
      AnimcharNodesMat44 *animchar_node_wtm, // always on client, never on server
      vec3f *animchar_render__root_pos,      // always on client, never on server
      const TMatrix &transform, bool animchar__turnDir = false) { ANIMCHAR_ACT(info.dt, animchar__accumDt, animchar__dtThreshold); });
}

ECS_AFTER(anim_phys_es)
ECS_REQUIRE(ecs::Tag animchar__actOnCreate)
ECS_ON_EVENT(on_appear)
static __forceinline void update_animchar_on_create_es_event_handler(const ecs::Event &, AnimV20::AnimcharBaseComponent &animchar,
  float *animchar__accumDt, const float *animchar__dtThreshold, AnimcharNodesMat44 *animchar_node_wtm,
  vec3f *animchar_render__root_pos, const TMatrix &transform, bool animchar__turnDir = false)
{
  ANIMCHAR_ACT(0.0, animchar__accumDt, animchar__dtThreshold);
}

ECS_TRACK(animchar__animStateNames)
static __forceinline void animchar_update_animstate_es_event_handler(const ecs::Event &, ecs::EntityId eid,
  const AnimV20::AnimcharBaseComponent &animchar, const ecs::Object &animchar__animStateNames, ecs::Object &animchar__animState)
{
  const auto animGraph = animchar.getAnimGraph();
  animchar__animState.clear();
  for (auto &stateName : animchar__animStateNames)
  {
    ecs::HashedConstString hashedString = ECS_HASH_SLOW(stateName.first.c_str());
    g_entity_mgr->sendEventImmediate(eid, EventChangeAnimState(hashedString, animGraph->getStateIdx(stateName.second.getStr())));
  }
}

ECS_REQUIRE(ecs::Tag animchar__actOnDemand)
ECS_TRACK(transform, animchar__turnDir)
ECS_ON_EVENT(on_appear)
static __forceinline void animchar_act_on_demand_es_event_handler(const ecs::Event &, AnimV20::AnimcharBaseComponent &animchar,
  AnimcharNodesMat44 *animchar_node_wtm, // always on client, never on server
  vec3f *animchar_render__root_pos, const TMatrix &transform, bool animchar__turnDir = false)
{
  ANIMCHAR_ACT(0.f, NULL, NULL);
}


ECS_REQUIRE(ecs::Tag animchar__actOnDemand)
ECS_REQUIRE(ecs::auto_type attachedToParent)
ECS_ON_EVENT(on_appear, on_disappear)
static __forceinline void animchar_act_on_demand_detach_es_event_handler(const ecs::Event &, AnimV20::AnimcharBaseComponent &animchar,
  AnimcharNodesMat44 *animchar_node_wtm, // always on client, never on server
  vec3f *animchar_render__root_pos, const TMatrix &transform, bool animchar__turnDir = false)
{
  ANIMCHAR_ACT(0.f, NULL, NULL);
}

ECS_REQUIRE(ecs::Tag disableUpdate)
ECS_TRACK(transform, animchar__turnDir)
ECS_ON_EVENT(on_appear)
static __forceinline void animchar_non_updatable_es_event_handler(const ecs::Event &, AnimV20::AnimcharBaseComponent &animchar,
  AnimcharNodesMat44 *animchar_node_wtm, // always on client, never on server
  vec3f *animchar_render__root_pos, const TMatrix &transform, bool animchar__turnDir = false)
{
  ANIMCHAR_ACT(0.f, NULL, NULL);
}

ECS_REQUIRE(ecs::Tag disableUpdate)
ECS_REQUIRE(ecs::auto_type attachedToParent)
ECS_ON_EVENT(on_appear, on_disappear)
static __forceinline void animchar_non_updatable_detach_es_event_handler(const ecs::Event &, AnimV20::AnimcharBaseComponent &animchar,
  AnimcharNodesMat44 *animchar_node_wtm, // always on client, never on server
  vec3f *animchar_render__root_pos, const TMatrix &transform, bool animchar__turnDir = false)
{
  ANIMCHAR_ACT(0.f, NULL, NULL);
}

ECS_TRACK(skeleton_attach__attached)
ECS_TAG(gameClient)
ECS_REQUIRE(eastl::false_type skeleton_attach__attached)
ECS_AFTER(skeleton_attach_clear_attach_es)
static __forceinline void animchar_skeleton_attach_destroy_attach_es_event_handler(const ecs::Event &,
  AnimV20::AnimcharBaseComponent &animchar,
  AnimcharNodesMat44 *animchar_node_wtm, // always on client, never on server
  vec3f *animchar_render__root_pos, const TMatrix &transform, bool animchar__turnDir = false)
{
  ANIMCHAR_ACT(0.f, NULL, NULL);
}

static __forceinline void animchar_act_on_phys_teleport_es(const EventOnEntityTeleported &, AnimV20::AnimcharBaseComponent &animchar,
  AnimcharNodesMat44 *animchar_node_wtm, // always on client, never on server
  vec3f *animchar_render__root_pos, const TMatrix &transform, bool animchar__turnDir = false)
{
  ANIMCHAR_ACT(0.f, NULL, NULL);
}

ECS_TRACK(animchar__objTexReplace)
static __forceinline void animchar_replace_texture_es_event_handler(const ecs::Event &,
  AnimV20::AnimcharRendComponent &animchar_render, const AnimV20::AnimcharBaseComponent &animchar, const ecs::string &animchar__res,
  const ecs::Object &animchar__objTexReplace)
{
  if (!animchar_render.getVisualResource() || animchar__objTexReplace.empty())
    return;

  // Set original texture
  reset_animchar_resource(animchar_render, animchar, animchar__res.c_str());
  replace_animchar_resource(animchar_render, animchar, animchar__res.c_str(), &animchar__objTexReplace, nullptr);
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void bindpose_init_es_event_handler(const ecs::Event &, AnimV20::AnimcharRendComponent &animchar_render)
{
  auto lodsRes = animchar_render.getVisualResource();
  if (!lodsRes)
    return;
  lodsRes->updateBindposes(); // inits per-resource, skips up-to-date resources
}
