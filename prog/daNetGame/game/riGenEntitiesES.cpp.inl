// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <ecs/io/blk.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathUtils.h>
#include "main/level.h"

struct RiExtraGen
{
  bool loaded = false;
};

ECS_DECLARE_RELOCATABLE_TYPE(RiExtraGen);
ECS_REGISTER_RELOCATABLE_TYPE(RiExtraGen, nullptr);
ECS_AUTO_REGISTER_COMPONENT(RiExtraGen, "ri_extra_gen", nullptr, 0);

template <typename Callable>
static ecs::QueryCbResult find_overall_game_zone_ecs_query(Callable c);

static __forceinline int gen_riextra_entities(dag::ConstSpan<rendinst::riex_handle_t> handles,
  const char *template_name,
  const DataBlock &blk,
  const Point2 &zone_pos,
  float zone_radius_sq)
{
  int createdCount = 0;
  for (rendinst::riex_handle_t handle : handles)
  {
    mat44f m;
    rendinst::getRIGenExtra44(handle, m);
    alignas(16) TMatrix transform;
    v_mat_43ca_from_mat44(transform.m[0], m);

    if (zone_radius_sq > 0.f)
    {
      float distSq = lengthSq(Point2::xz(transform.getcol(3)) - zone_pos);
      if (distSq > zone_radius_sq)
        continue;
    }

    ecs::ComponentsInitializer attrs;
    ECS_INIT(attrs, ri_extra__handle, handle);
    ECS_INIT(attrs, transform, transform);
    // we do that automatically for all entities that have ri_extra.handle, transform and initialTransform
    // ECS_INIT(attrs, initialTransform, transform);
    for (int i = 0; i < blk.paramCount(); ++i)
      attrs[ECS_HASH_SLOW(blk.getParamName(i))] = ecs::load_comp_from_blk(blk, i);
    g_entity_mgr->createEntityAsync(template_name, eastl::move(attrs));
    createdCount++;
  }
  return createdCount;
}

bool get_overall_zone(Point2 &pos, float &radius_sq)
{
  const float zoneMargin = 75.f;
  return find_overall_game_zone_ecs_query(
           [&](const Point2 &shrinkedZonePos, float shrinkZoneRadius ECS_REQUIRE(ecs::Tag disableExternalArea)) {
             pos = shrinkedZonePos;
             radius_sq = sqr(shrinkZoneRadius + zoneMargin);
             return ecs::QueryCbResult::Stop;
           }) == ecs::QueryCbResult::Stop;
}

ECS_TAG(server)
static __forceinline void ri_extra_gen_es_event_handler(const EventLevelLoaded &,
  RiExtraGen &ri_extra_gen,
  const ecs::string &ri_extra_gen__riName,
  const ecs::string &ri_extra_gen__template)
{
  if (ri_extra_gen.loaded)
    return;
  int resIdx = rendinst::getRIGenExtraResIdx(ri_extra_gen__riName.c_str());
  if (resIdx < 0)
  {
    logerr("invalid riextra '%s' name in gen? Nothing was found", ri_extra_gen__riName.c_str());
    return;
  }

  Tab<rendinst::riex_handle_t> handles(framemem_ptr());
  rendinst::getRiGenExtraInstances(handles, resIdx);
  int createdCount = 0;

  if (!handles.empty())
  {
    Point2 zonePos(0.f, 0.f);
    float zoneRadiusSq = 0.0f;
    if (get_overall_zone(zonePos, zoneRadiusSq))
      debug("ri_extra_gen: don't create entities outside of overall zone %.2f %.2f radius %.1f", P2D(zonePos), sqrtf(zoneRadiusSq));

    createdCount = gen_riextra_entities(handles, ri_extra_gen__template.c_str(), DataBlock::emptyBlock, zonePos, zoneRadiusSq);
  }
  debug("ri_extra_gen: created %i/%i (%.1f%%) of ri_extra entities inside of overall game zone", createdCount, handles.size(),
    safediv(float(createdCount), handles.size()) * 100.f);

  ri_extra_gen.loaded = true;
}

ECS_ON_EVENT(on_appear)
static __forceinline void ri_extra_gen_mark_dynamic_es_event_handler(const ecs::Event &, const ecs::string &ri_extra_gen__blk)
{
  DataBlock blk;
  bool res = blk.load(ri_extra_gen__blk.c_str());
  G_ASSERTF(res, "Cannot load %s", ri_extra_gen__blk.c_str());
  if (!res)
    return;
  for (int i = 0; i < blk.paramCount(); ++i)
    rendinst::setRIGenExtraResDynamic(blk.getParamName(i));
}

ECS_TAG(server)
ECS_ON_EVENT(EventLevelLoaded, EventRIGenExtraRequested)
static __forceinline void ri_extra_gen_blk_es_event_handler(const ecs::Event &evt,
  ecs::EntityId eid,
  RiExtraGen &ri_extra_gen,
  const ecs::string &ri_extra_gen__blk,
  const ecs::string *ri_extra_gen__createEntityWhenDone = nullptr)
{
  debug("ri_extra_gen: %@ <%@>: on event <%@> (ri_extra_gen.loaded=%@)", (ecs::entity_id_t)eid,
    g_entity_mgr->getEntityTemplateName(eid), evt.getName(), ri_extra_gen.loaded);

  if (ri_extra_gen.loaded)
    return;

  DataBlock blk;
  bool res = blk.load(ri_extra_gen__blk.c_str());
  G_ASSERTF(res, "Cannot load %s", ri_extra_gen__blk.c_str());
  if (!res)
  {
    logerr("Cannot load %s", ri_extra_gen__blk.c_str());
    return;
  }

  Point2 zonePos(0.f, 0.f);
  float zoneRadiusSq = 0.0f;
  if (get_overall_zone(zonePos, zoneRadiusSq))
    debug("ri_extra_gen: %@ <%@> don't create entities outside of overall zone %@ radius %.1f", (ecs::entity_id_t)eid,
      g_entity_mgr->getEntityTemplateName(eid), zonePos, sqrtf(zoneRadiusSq));

  int createdCount = 0, totalCount = 0;

  for (int i = 0; i < blk.paramCount(); ++i)
  {
    const char *asset = blk.getParamName(i);
    const char *templ = blk.getStr(i);
#if DAECS_EXTENSIVE_CHECKS
    int sameParam = blk.findParam(blk.getParamNameId(i), i);
    if (sameParam >= 0 && strcmp(blk.getStr(sameParam), templ) == 0)
      logerr("rendinst templates/doors file <%s> has duplicate asset <%s> with same template=<%s>", ri_extra_gen__blk.c_str(), asset,
        templ);
#endif
    int resIdx = rendinst::getRIGenExtraResIdx(asset);
    if (resIdx < 0)
      continue;
    Tab<rendinst::riex_handle_t> handles(framemem_ptr());
    rendinst::getRiGenExtraInstances(handles, resIdx);
    if (handles.empty())
      continue;
    createdCount += gen_riextra_entities(handles, templ, *blk.getBlockByNameEx(asset), zonePos, zoneRadiusSq);
    totalCount += handles.size();
  }
  debug("ri_extra_gen: %@ <%@> created %i/%i (%.1f%%) of ri_extra entities inside of overall game zone %@ radius %.1f",
    (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid), createdCount, totalCount,
    safediv(float(createdCount), totalCount) * 100.f, zonePos, sqrtf(zoneRadiusSq));
  if (ri_extra_gen__createEntityWhenDone && !ri_extra_gen__createEntityWhenDone->empty())
    g_entity_mgr->createEntityAsync(ri_extra_gen__createEntityWhenDone->c_str());
  ri_extra_gen.loaded = true;
}
