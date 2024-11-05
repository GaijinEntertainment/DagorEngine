// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <rendInst/rendInstExtra.h>
#include <daECS/core/coreEvents.h>
#include <gameRes/dag_stdGameRes.h>
#include <ecs/rendInst/riExtra.h>

struct RiPreviewResPreload
{
  static void requestResources(const char *, const ecs::resource_request_cb_t &res_cb)
  {
    const char *riResName = res_cb.get<ecs::string>(ECS_HASH("ri_preview__name")).c_str();
    request_ri_resources(res_cb, riResName);
  }
  bool onLoaded(ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    using namespace rendinst;
    const char *name = mgr.get<ecs::string>(eid, ECS_HASH("ri_preview__name")).c_str();
    int resIdx = getRIGenExtraResIdx(name);
    if (resIdx < 0)
    {
      auto riaddf = AddRIFlag::UseShadow | AddRIFlag::Dynamic | AddRIFlag::GameresPreLoaded;
      resIdx = addRIGenExtraResIdx(name, -1, -1, riaddf);
      if (resIdx < 0)
      {
        logerr("Failed to add ri_preview.name=<%s>(%d) while creating entity %d<%s>", name, resIdx, (ecs::entity_id_t)eid,
          g_entity_mgr->getEntityTemplateName(eid));
        return false;
      }
      addRiGenExtraDebris(resIdx, 0);
    }
    mgr.set(eid, ECS_HASH("semi_transparent__resIdx"), resIdx);
    return true;
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(RiPreviewResPreload);
ECS_REGISTER_RELOCATABLE_TYPE(RiPreviewResPreload, nullptr);
ECS_AUTO_REGISTER_COMPONENT(ecs::string, "ri_preview__name", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(int, "semi_transparent__resIdx", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(
  RiPreviewResPreload, "riPreviewResPreload", nullptr, 0, "ri_preview__name", "semi_transparent__resIdx");
