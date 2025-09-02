// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <ecs/anim/anim.h>
#include <ecs/render/updateStageRender.h>
#include <gamePhys/phys/walker/humanWeapState.h>
#include <generic/dag_enumerate.h>

#include <shaders/dag_dynSceneRes.h>


template <typename Callable>
inline void gather_collimator_moa_lens_info_ecs_query(ecs::EntityId, Callable c);
template <typename Callable>
inline void gather_gun_mod_list_ecs_query(Callable c);
template <typename Callable>
inline void count_cockpit_entities_ecs_query(Callable c);
template <typename Callable>
inline void gather_weapon_template_name_ecs_query(ecs::EntityId, Callable c);

namespace
{
struct LensRenderInfo
{
  int relemId = -1;
  int rigidNo = -1;

  operator bool() const { return relemId != 1 && rigidNo != -1; }
};

const char *DYNAMIC_COLLIMATOR_MOA_SHADER_NAME = "dynamic_collimator_moa";
} // namespace

static LensRenderInfo get_lens_render_info(ecs::EntityId gun_mod_eid)
{
  LensRenderInfo lensRenderInfo;

  gather_collimator_moa_lens_info_ecs_query(gun_mod_eid,
    [&lensRenderInfo](ECS_REQUIRE(ecs::Tag gunScope) const AnimV20::AnimcharRendComponent &animchar_render,
      const ecs::string &animchar__res) {
      const DynamicRenderableSceneInstance *sceneInstance = animchar_render.getSceneInstance();
      if (!sceneInstance || !sceneInstance->getLodsResource())
        return;

      const DynamicRenderableSceneResource *sceneRes = sceneInstance->getLodsResource()->lods[0].scene;
      if (!sceneRes)
        return;

      int resRigidNo = -1;
      int resElemId = -1;

      dag::ConstSpan<DynamicRenderableSceneResource::RigidObject> rigids = sceneRes->getRigidsConst();
      for (auto [rigidNo, rigid] : enumerate(rigids))
      {
        dag::ConstSpan<ShaderMesh::RElem> relems = rigid.mesh->getMesh()->getAllElems();
        for (auto [relemId, relem] : enumerate(relems))
        {
          if (strcmp(relem.mat->getShaderClassName(), DYNAMIC_COLLIMATOR_MOA_SHADER_NAME) == 0)
          {
            if (resElemId == -1)
            {
              resRigidNo = rigidNo;
              resElemId = relemId;
            }
            else
            {
              const char *selectedNodeName = sceneRes->getNames().node.getNameSlow(rigids[resRigidNo].nodeId);
              const char *excessiveNodeName = sceneRes->getNames().node.getNameSlow(rigid.nodeId);

              // clang-format off
              logerr("[CollimatorMOA] %s material in human cockpit is not supported for multiple nodes."
                "\nWhat to check:"
                "\n1.Artists: check the asset %s. The code encountered two different nodes with %s shader."
                "\n  First Node: '%s',"
                "\n  Second Node: '%s'."
                "\n  >>The shader must be used only for ONE node<<"
                "\n2.Programmers: if the asset is fine, this case must be investigated."
                "\n  Somehow we've got collision in ShaderMesh::RElem."
                "\n   First [RigidId:%d ElemId:%d],"
                "\n   Second [RigidId:%d ElemId:%d]",
                DYNAMIC_COLLIMATOR_MOA_SHADER_NAME,
                animchar__res, DYNAMIC_COLLIMATOR_MOA_SHADER_NAME,
                selectedNodeName,
                excessiveNodeName,
                resRigidNo, resElemId,
                rigidNo, relemId
              );
              // clang-format on
            }
          }
        }
      }

      lensRenderInfo.relemId = resElemId;
      lensRenderInfo.rigidNo = resRigidNo;
    });

  return lensRenderInfo;
}

static const char *get_weapon_template_name(const ecs::EntityId eid_with_gunmods)
{
  const char *weapon_template = "<failed to gather weap template name>";

  gather_weapon_template_name_ecs_query(eid_with_gunmods,
    [&weapon_template](const int human_weap__currentGunSlot, const ecs::Object &human_weap__weapTemplates) {
      if (human_weap__currentGunSlot < eastl::size(humanWeaponSlotNames))
      {
        const char *slotName = humanWeaponSlotNames[human_weap__currentGunSlot];
        weapon_template = human_weap__weapTemplates.getMemberOr(ECS_HASH_SLOW(slotName), "<no such slot>");
      }
    });

  return weapon_template;
}

static const char *get_cockpit_template(const ecs::EntityId eid)
{
  const char *tmpl = g_entity_mgr->getEntityTemplateName(eid);
  return tmpl ? tmpl : "<failed to get cockpit tmpl>";
}

static void logerr_multiple_collimators(const ecs::EntityId selected_cockpit_eid, const ecs::EntityId selected_gunmod_eid,
  const ecs::EntityId excessive_gunmod_eid)
{
  const char *selectedCockpitTemplate = get_cockpit_template(selected_cockpit_eid);
  const char *selectedWeapon = get_weapon_template_name(selected_cockpit_eid);

  const char *selectedGunModItemTemplate = g_entity_mgr->getOr(selected_gunmod_eid, ECS_HASH("item__template"), "");
  const char *excessiveGunModItemTemplate = g_entity_mgr->getOr(excessive_gunmod_eid, ECS_HASH("item__template"), "");

  logerr("MOA Collimator: %s material in human cockpit is not supported for multiple scopes."
         "\nWeapon: %s"
         "\nSelected scope: %s"
         "\nExcessive scope:%s"
         "\ncockpitEid: %d, cockpitTemplate: %s",
    DYNAMIC_COLLIMATOR_MOA_SHADER_NAME, selectedWeapon, selectedGunModItemTemplate, excessiveGunModItemTemplate, selected_cockpit_eid,
    selectedCockpitTemplate);
}

static void logwarn_multiple_cockpits(const ecs::EntityId selected_cockpit_eid, const ecs::EntityId excessive_cockpit_eid)
{
  const char *selectedCockpitTemplate = get_cockpit_template(selected_cockpit_eid);
  const char *excessiveCockpitTemplate = get_cockpit_template(excessive_cockpit_eid);
  logwarn("MOA Collimator: encountered multiple cockpits entities (ecs::Tag cockpitEntity, ecs::EntityId watchedByPlr)."
          "\nPossibly because of the broken spectator."
          "\nvalidate_observed_entity_es should fix watching references."
          "\nAs a workaround we delay update of the current gunmod with moa collimator."
          "\nFirst Cockpit: [eid: %d, template: %s]"
          "\nSecond Cockpit: [eid: %d, template: %s]",
    selected_cockpit_eid, selectedCockpitTemplate, excessive_cockpit_eid, excessiveCockpitTemplate);
}

static bool is_spectator_broken_and_we_need_to_delay_update()
{
  ecs::EntityId cockpitEid;
  ecs::EntityId secondCockpitEid;

  count_cockpit_entities_ecs_query([&](ECS_REQUIRE(ecs::Tag cockpitEntity) ECS_REQUIRE(ecs::EntityId watchedByPlr)
                                       ECS_REQUIRE(const ecs::EidList &human_weap__currentGunModEids) const ecs::EntityId eid) {
    if (cockpitEid)
      secondCockpitEid = eid;
    else
      cockpitEid = eid;
  });

  if (secondCockpitEid)
  {
    logwarn_multiple_cockpits(cockpitEid, secondCockpitEid);
    return true;
  }

  return false;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_REQUIRE(ecs::Tag cockpitEntity)
ECS_REQUIRE(ecs::EntityId watchedByPlr)
ECS_REQUIRE(ecs::EidList human_weap__currentGunModEids)
ECS_TRACK(human_weap__currentGunModEids)
static void collimator_moa_toggle_update_es_event_handler(const ecs::Event &)
{
  const ecs::EntityId renderEid = g_entity_mgr->getSingletonEntity(ECS_HASH("collimator_moa_render"));
  if (renderEid)
    ECS_SET(renderEid, collimator_moa_render__relem_update_required, true);
}

ECS_TAG(render)
static void collimator_moa_update_lens_eids_es(const UpdateStageInfoBeforeRender &, ecs::EntityId &collimator_moa_render__gun_mod_eid,
  int &collimator_moa_render__rigid_id, int &collimator_moa_render__relem_id, bool &collimator_moa_render__relem_update_required)
{
  if (!collimator_moa_render__relem_update_required)
    return;

  collimator_moa_render__gun_mod_eid = ecs::EntityId{};
  collimator_moa_render__rigid_id = -1;
  collimator_moa_render__relem_id = -1;

  if (is_spectator_broken_and_we_need_to_delay_update())
    return;

  bool foundLoadingEntity = false;
  ecs::EntityId cockpitEid;

  gather_gun_mod_list_ecs_query(
    [&](ECS_REQUIRE(ecs::Tag cockpitEntity) ECS_REQUIRE(ecs::EntityId watchedByPlr) const ecs::EntityId eid,
      const ecs::EidList &human_weap__currentGunModEids) {
      for (ecs::EntityId gunModEid : human_weap__currentGunModEids)
      {
        if (gunModEid == collimator_moa_render__gun_mod_eid)
        {
          logwarn(
            "MOA Collimator: encountered duplicate gunModEid(%d) with collimator moa material inside human_weap__currentGunModEids",
            gunModEid);
          continue;
        }

        if (g_entity_mgr->isLoadingEntity(gunModEid))
        {
          foundLoadingEntity = true;
          continue;
        }

        const LensRenderInfo rInfo = get_lens_render_info(gunModEid);
        if (rInfo)
        {
          if (collimator_moa_render__gun_mod_eid)
          {
            logerr_multiple_collimators(cockpitEid, collimator_moa_render__gun_mod_eid, gunModEid);
            continue;
          }

          cockpitEid = eid;
          collimator_moa_render__gun_mod_eid = gunModEid;
          collimator_moa_render__rigid_id = rInfo.rigidNo;
          collimator_moa_render__relem_id = rInfo.relemId;
        }
      }
    });

  collimator_moa_render__relem_update_required = foundLoadingEntity && !collimator_moa_render__gun_mod_eid;
}