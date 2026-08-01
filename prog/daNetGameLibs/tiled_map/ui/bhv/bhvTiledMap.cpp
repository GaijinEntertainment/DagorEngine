// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvTiledMap.h"
#include "../tiledMapContext.h"
#include "ui/uiBindings.h"
#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_transform.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <perfMon/dag_statDrv.h>
#include <ecs/scripts/sqEntity.h>
#include <EASTL/algorithm.h>
#include <EASTL/fixed_vector.h>
#include <generic/dag_span.h>

using namespace darg;

namespace
{
struct OverlapItem
{
  Element *child;
  Point2 pos;
  Point2 halfSize;
  bool clampToBorder;
};

static void resolve_label_overlaps(dag::Span<OverlapItem> items, const BBox2 &map_box)
{
  const int maxIterations = 16;
  const int n = (int)items.size();
  for (int iter = 0; iter < maxIterations; ++iter)
  {
    bool anyOverlap = false;
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j)
      {
        OverlapItem &a = items[i];
        OverlapItem &b = items[j];
        const Point2 delta = b.pos - a.pos;
        const float sepX = a.halfSize.x + b.halfSize.x;
        const float sepY = a.halfSize.y + b.halfSize.y;
        const float penX = sepX - fabsf(delta.x);
        const float penY = sepY - fabsf(delta.y);
        if (penX <= 0.f || penY <= 0.f)
          continue;

        anyOverlap = true;
        if (fabsf(delta.x) < 1e-3f && fabsf(delta.y) < 1e-3f)
        {
          if (sepY <= sepX)
          {
            const float half = sepY * 0.5f;
            a.pos.y -= half;
            b.pos.y += half;
          }
          else
          {
            const float half = sepX * 0.5f;
            a.pos.x -= half;
            b.pos.x += half;
          }
        }
        else if (penX < penY)
        {
          const float push = penX * 0.5f;
          const float dir = delta.x >= 0.f ? 1.f : -1.f;
          a.pos.x -= dir * push;
          b.pos.x += dir * push;
        }
        else
        {
          const float push = penY * 0.5f;
          const float dir = delta.y >= 0.f ? 1.f : -1.f;
          a.pos.y -= dir * push;
          b.pos.y += dir * push;
        }
      }
    if (!anyOverlap)
      break;
  }

  for (OverlapItem &it : items)
  {
    Point2 p = it.pos;
    // Only keep border-clamped labels on the map; labels that opted out may
    // leave the visible area and are clipped/hidden as before.
    if (it.clampToBorder)
    {
      if (map_box.left() + it.halfSize.x <= map_box.right() - it.halfSize.x)
        p.x = eastl::clamp(p.x, map_box.left() + it.halfSize.x, map_box.right() - it.halfSize.x);
      if (map_box.top() + it.halfSize.y <= map_box.bottom() - it.halfSize.y)
        p.y = eastl::clamp(p.y, map_box.top() + it.halfSize.y, map_box.bottom() - it.halfSize.y);
    }
    it.child->transform->translate = round(p);
  }
}
} // namespace


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvTiledMap, bhv_tiled_map, cstr);


BhvTiledMap::BhvTiledMap() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


void BhvTiledMap::onAttach(Element *elem)
{
  G_UNUSED(elem);
  G_ASSERT(TiledMapContext::get_from_element(elem));
}


void BhvTiledMap::onDetach(Element *elem, DetachMode) { G_UNUSED(elem); }


int BhvTiledMap::update(UpdateStage /*stage*/, darg::Element *elem, float dt)
{
  (void)dt;
  TIME_PROFILE(bhv_tiled_map_update);

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  TiledMapContext *tiledMapCtx = TiledMapContext::get_from_element(elem);
  if (!tiledMapCtx)
  {
    LOGERR_ONCE("%s: TiledMapContext = %p", __FUNCTION__, tiledMapCtx);
    return 0;
  }

  TMatrix tm;
  tiledMapCtx->calcTmFromView(tm);
  Point3 eyePosition = tiledMapCtx->getCurViewItm().getcol(3);

  int resultFlags = 0;

  const Sqrat::Object &dataKey = elem->csk->data;

  BBox2 mapBox = elem->transformedBbox;
  eastl::fixed_vector<OverlapItem, 32> overlapItems;

  for (int iChild = 0, nChildren = elem->children.size(); iChild < nChildren; ++iChild)
  {
    Element *child = elem->children[iChild];
    if (!child->transform)
      continue;

    Sqrat::Object childElemData = child->props.scriptDesc.RawGetSlot(dataKey);
    if (childElemData.IsNull())
      continue;

    Point3 worldPos(0, 0, 0), lookDir(0, 0, 0);
    bool worldPosValid = false, lookDirValid = false;
    Sqrat::Object worldPosObj;
    ecs::EntityId eid = ecs::INVALID_ENTITY_ID;

    bool useAttrTransform = false;
    if ((eid = childElemData.RawGetSlotValue(strings->zoneEid, ecs::INVALID_ENTITY_ID)) != ecs::INVALID_ENTITY_ID)
      useAttrTransform = true;
    else if ((eid = childElemData.RawGetSlotValue(strings->eid, ecs::INVALID_ENTITY_ID)) != ecs::INVALID_ENTITY_ID)
      useAttrTransform = true;

    if (useAttrTransform)
    {
      const TMatrix *transform = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform_lastFrame"));
      if (!transform)
        transform = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform"));
      if (!transform)
      {
        if (const ecs::EntityId *attachedTo = g_entity_mgr->getNullable<ecs::EntityId>(eid, ECS_HASH("game_effect__attachedTo")))
        {
          transform = g_entity_mgr->getNullable<TMatrix>(*attachedTo, ECS_HASH("transform_lastFrame"));
          if (!transform)
            transform = g_entity_mgr->getNullable<TMatrix>(*attachedTo, ECS_HASH("transform"));
        }
      }
      if (transform)
      {
        worldPos = transform->getcol(3);
        lookDir = transform->getcol(0);
        worldPosValid = lookDirValid = true;
      }
    }
    else if (!(worldPosObj = childElemData.RawGetSlot(strings->worldPos)).IsNull())
    {
      worldPos = worldPosObj.Cast<Point3 &>();
      worldPosValid = true;
    }

    bool wasHidden = child->isHidden();
    child->setHidden(!worldPosValid);

    if (worldPosValid && tiledMapCtx->isViewInitialized())
    {
      // zero of this transform should be the center of the map element
      Point2 screenPos = tiledMapCtx->worldToMap(worldPos);

      BBox2 screenBbox;
      screenBbox += elem->screenCoord.size / 2.f;
      screenBbox += -elem->screenCoord.size / 2.f;

      float maxDistance = childElemData.RawGetSlotValue(strings->maxDistance, -1.0f);
      const bool clampToBorder = childElemData.RawGetSlotValue(strings->clampToBorder, false);
      if (maxDistance > 0 && lengthSq(worldPos - eyePosition) > sqr(maxDistance))
        child->setHidden(true);
      else if (clampToBorder)
      {
        bool isClipped = !(screenBbox & screenPos);
        if (isClipped)
          child->setGroupStateFlags(ui::S_CLIPPED);
        else
          child->clearGroupStateFlags(ui::S_CLIPPED);

        Point2 childOffset = child->screenCoord.size / 2.f;
        screenPos.x = eastl::clamp(screenPos.x, screenBbox.left() + childOffset.x, screenBbox.right() - childOffset.x);
        screenPos.y = eastl::clamp(screenPos.y, screenBbox.top() + childOffset.y, screenBbox.bottom() - childOffset.y);
      }
      else if (childElemData.RawGetSlotValue(strings->hideOutside, false))
        child->setHidden(!(screenBbox & screenPos));

      bool dirRotate = lookDirValid && childElemData.RawGetSlotValue(strings->dirRotate, false);
      if (dirRotate)
      {
        Point3 d = (lookDir % tm);         // inversed order because of the left-handed coordinate system
        float lookAng = atan2f(d.x, -d.z); // inversed z because of the left-handed coordinate system
        child->transform->rotate = lookAng;
      }

      child->transform->translate = round(screenPos);
      G_ASSERT(isfinite(child->transform->translate.x) && isfinite(child->transform->translate.y));

      if (!child->isHidden() && childElemData.RawGetSlotValue(strings->avoidOverlap, false))
      {
        // size is zero for label-style children, so the visual extent lives in contentSize.
        const float padding = childElemData.RawGetSlotValue(strings->overlapPadding, 0.0f);
        Point2 halfSize;
        halfSize.x = 0.5f * (eastl::max(child->screenCoord.size.x, child->screenCoord.contentSize.x) + padding);
        halfSize.y = 0.5f * (eastl::max(child->screenCoord.size.y, child->screenCoord.contentSize.y) + padding);
        overlapItems.push_back(OverlapItem{child, screenPos, halfSize, clampToBorder});
      }
    }

    if (child->isHidden() != wasHidden)
      resultFlags |= R_REBUILD_RENDER_AND_INPUT_LISTS;
  }

  if (overlapItems.size() > 1)
    resolve_label_overlaps(make_span(overlapItems.data(), overlapItems.size()), mapBox);

  return resultFlags;
}
