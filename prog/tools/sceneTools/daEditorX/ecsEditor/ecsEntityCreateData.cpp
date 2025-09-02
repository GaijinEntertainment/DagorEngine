// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ecsEntityCreateData.h"

#include <daECS/scene/scene.h>

ECSEntityCreateData::ECSEntityCreateData(ecs::EntityId e, const char *template_name)
{
  if (g_entity_mgr->has(e, ECS_HASH("transform")))
    attrs[ECS_HASH("transform")] = g_entity_mgr->get<TMatrix>(e, ECS_HASH("transform"));
  if (const ecs::Scene::EntityRecord *erec = ecs::g_scenes->getActiveScene().findEntityRecord(e))
  {
    for (auto &comp : erec->clist)
      attrs[ECS_HASH_SLOW(comp.first.c_str())] = ecs::ChildComponent(comp.second);
    templName = erec->templateName;
  }
  else
  {
    if (template_name && *template_name)
      templName = template_name;
    else
      templName = g_entity_mgr->getEntityTemplateName(e);
  }
}

ECSEntityCreateData::ECSEntityCreateData(const char *template_name, const TMatrix *tm)
{
  templName = template_name;
  if (tm)
    attrs[ECS_HASH("transform")] = *tm;
}

ECSEntityCreateData::ECSEntityCreateData(const char *template_name, const TMatrix *tm, const char *riex_name)
{
  templName = template_name;
  if (tm)
    attrs[ECS_HASH("transform")] = *tm;
  if (riex_name)
    attrs[ECS_HASH("ri_extra__name")] = riex_name;
}

void ECSEntityCreateData::createCb(ECSEntityCreateData *ctx)
{
  ctx->eid = g_entity_mgr->createEntitySync(ctx->templName.c_str(), ecs::ComponentsInitializer(ctx->attrs));
}
