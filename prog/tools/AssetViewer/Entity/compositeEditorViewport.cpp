// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorViewport.h"
#include "../av_appwnd.h"
#include "../av_cm.h"
#include <assets/asset.h>
#include <de3_baseInterfaces.h>
#include <de3_composit.h>
#include <de3_dataBlockIdHolder.h>
#include <de3_entityGetSceneLodsRes.h>
#include <de3_objEntity.h>
#include <debug/dag_debug3d.h>
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_interface.h>
#include <math/dag_rayIntersectBox.h>
#include <math/dag_TMatrix.h>
#include <rendInst/rendInstExtra.h>
#include <winGuiWrapper/wgw_input.h>

IObjEntity *CompositeEditorViewport::getHitSubEntity(IGenViewportWnd *wnd, int x, int y, IObjEntity &entity)
{
  if (!get_app().isCompositeEditorShown())
    return nullptr;

  IPixelPerfectSelectionService *pixelPerfectSelectionService = EDITORCORE->queryEditorInterface<IPixelPerfectSelectionService>();
  if (!pixelPerfectSelectionService)
    return nullptr;

  Point3 dir, world;
  wnd->clientToWorld(Point2(x, y), world, dir);

  pixelPerfectSelectionHitsCache.clear();
  fillPossiblePixelPerfectSelectionHits(*pixelPerfectSelectionService, entity, nullptr, world, dir, pixelPerfectSelectionHitsCache);

  pixelPerfectSelectionService->getHits(*wnd, x, y, pixelPerfectSelectionHitsCache);
  return pixelPerfectSelectionHitsCache.empty() ? nullptr : static_cast<IObjEntity *>(pixelPerfectSelectionHitsCache[0].userData);
}

bool CompositeEditorViewport::getSelectionBox(IObjEntity *entity, BBox3 &box) const
{
  IObjEntity *selectedSubEntity = getSelectedSubEntity(entity);
  if (!selectedSubEntity)
    return false;

  TMatrix tm;
  getRendInstQuantizedTm(*selectedSubEntity, tm);
  box = tm * selectedSubEntity->getBbox();
  return true;
}

void CompositeEditorViewport::registerMenuAccelerators()
{
  IWndManager &wndManager = *EDITORCORE->getWndManager();

  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_DELETE_SELECTED_NODE, ImGuiKey_Delete);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_NONE, ImGuiKey_Q);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_MOVE, ImGuiKey_W);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_ROTATE, ImGuiKey_E);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_SCALE, ImGuiKey_R);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_TOGGLE_MOVE_SNAP, ImGuiKey_S);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_TOGGLE_ANGLE_SNAP, ImGuiKey_A);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_TOGGLE_SCALE_SNAP, ImGuiMod_Shift | ImGuiKey_5);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_CANCEL_GIZMO_TRANSFORM, ImGuiKey_Escape);
}

void CompositeEditorViewport::handleViewportAcceleratorCommand(unsigned id, IGenViewportWnd &wnd, IObjEntity *entity)
{
  if (id == CM_COMPOSITE_EDITOR_DELETE_SELECTED_NODE)
  {
    BBox3 bbox;
    if (getSelectionBox(entity, bbox)) // Delete rendered nodes only.
    {
      get_app().getCompositeEditor().deleteSelectedNode();
      wnd.activate();
    }
  }
  else if (id == CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_NONE)
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_None);
  }
  else if (id == CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_MOVE)
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_Move);
  }
  else if (id == CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_ROTATE)
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_Rotate);
  }
  else if (id == CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_SCALE)
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_Scale);
  }
  else if (id == CM_COMPOSITE_EDITOR_TOGGLE_MOVE_SNAP)
  {
    get_app().getCompositeEditor().toggleSnapMode(CM_VIEW_GRID_MOVE_SNAP);
  }
  else if (id == CM_COMPOSITE_EDITOR_TOGGLE_ANGLE_SNAP)
  {
    get_app().getCompositeEditor().toggleSnapMode(CM_VIEW_GRID_ANGLE_SNAP);
  }
  else if (id == CM_COMPOSITE_EDITOR_TOGGLE_SCALE_SNAP)
  {
    get_app().getCompositeEditor().toggleSnapMode(CM_VIEW_GRID_SCALE_SNAP);
  }
  else if (id == CM_COMPOSITE_EDITOR_CANCEL_GIZMO_TRANSFORM)
  {
    EDITORCORE->endGizmo(/*apply = */ false);
  }
}

void CompositeEditorViewport::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, IObjEntity *entity)
{
  if (!get_app().isCompositeEditorShown() || !entity)
    return;

  IObjEntity *hitEntity = getHitSubEntity(wnd, x, y, *entity);
  unsigned dataBlockId = hitEntity ? getEntityDataBlockId(*hitEntity) : IDataBlockIdHolder::invalid_id;
  get_app().getCompositeEditor().selectTreeNodeByDataBlockId(dataBlockId);
}

void CompositeEditorViewport::renderObjects(IGenViewportWnd &wnd, IObjEntity *entity)
{
  IObjEntity *selectedSubEntity = getSelectedSubEntity(entity);
  if (!selectedSubEntity)
    return;

  if (selectedSubEntity != cachedSelectedSubEntity)
  {
    cachedSelectedSubEntity = selectedSubEntity;
    riElementsCache.clear();
    dynmodelElementsCache.clear();
    fillRenderElements(*selectedSubEntity, riElementsCache, dynmodelElementsCache);
  }
  if (!riElementsCache.empty() || !dynmodelElementsCache.empty())
    outlineRenderer.render(wnd, riElementsCache, dynmodelElementsCache);
}

void CompositeEditorViewport::renderTransObjects(IObjEntity *entity)
{
  IObjEntity *selectedSubEntity = getSelectedSubEntity(entity);
  if (!selectedSubEntity)
    return;

  BBox3 bbox = selectedSubEntity->getBbox();
  TMatrix tm;
  getRendInstQuantizedTm(*selectedSubEntity, tm);

  begin_draw_cached_debug_lines();
  set_cached_debug_lines_wtm(tm);
  draw_cached_debug_box(bbox, E3DCOLOR(255, 0, 0));
  end_draw_cached_debug_lines();
}

void CompositeEditorViewport::invalidateCache()
{
  cachedSelectedSubEntity = nullptr;
  riElementsCache.clear();
  dynmodelElementsCache.clear();
}

void CompositeEditorViewport::getRendInstQuantizedTm(IObjEntity &entity, TMatrix &tm)
{
  const IRendInstEntity *rendInstEntity = entity.queryInterface<IRendInstEntity>();
  if (!rendInstEntity || !rendInstEntity->getRendInstQuantizedTm(tm))
    entity.getTm(tm);
}

void CompositeEditorViewport::getAllHits(IObjEntity &entity, const Point3 &from, const Point3 &dir,
  dag::Vector<CompositeEntityHit> &hits)
{
  ICompositObj *compositObj = entity.queryInterface<ICompositObj>();
  if (!compositObj)
    return;

  const int subEntityCount = compositObj->getCompositSubEntityCount();
  for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
  {
    IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
    if (!subEntity)
      continue;

    TMatrix tm;
    getRendInstQuantizedTm(*subEntity, tm);

    float out_t;
    if (ray_intersect_box(from, dir, subEntity->getBbox(), tm, out_t))
    {
      CompositeEntityHit hit;
      hit.entity = subEntity;
      hit.distance = out_t;
      hits.push_back(std::move(hit));
    }
  }
}

IObjEntity *CompositeEditorViewport::getNearestHit(IObjEntity &entity, const Point3 &from, const Point3 &dir)
{
  dag::Vector<CompositeEntityHit> hits;
  getAllHits(entity, from, dir, hits);
  if (hits.empty())
    return nullptr;

  CompositeEntityHit *nearestHit = nullptr;
  for (int i = 0; i < hits.size(); ++i)
  {
    if (hits[i].entity == &entity)
      hits[i].distance = FLT_MAX;

    if (!nearestHit || hits[i].distance < nearestHit->distance)
      nearestHit = &hits[i];
  }

  return nearestHit->entity;
}

unsigned CompositeEditorViewport::getEntityDataBlockId(IObjEntity &entity)
{
  IDataBlockIdHolder *dbih = entity.queryInterface<IDataBlockIdHolder>();
  return dbih ? dbih->getDataBlockId() : IDataBlockIdHolder::invalid_id;
}

IObjEntity *CompositeEditorViewport::getSubEntityByDataBlockId(IObjEntity &entity, unsigned dataBlockId)
{
  ICompositObj *compositObj = entity.queryInterface<ICompositObj>();
  if (!compositObj)
    return nullptr;

  const int subEntityCount = compositObj->getCompositSubEntityCount();
  for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
  {
    IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
    if (!subEntity)
      continue;

    IDataBlockIdHolder *dbih = subEntity->queryInterface<IDataBlockIdHolder>();
    if (dbih && dbih->getDataBlockId() == dataBlockId)
      return subEntity;
  }

  return nullptr;
}

IObjEntity *CompositeEditorViewport::getSelectedSubEntity(IObjEntity *entity)
{
  const unsigned dataBlockId = get_app().getCompositeEditor().getSelectedTreeNodeDataBlockId();
  if (!entity || dataBlockId == IDataBlockIdHolder::invalid_id)
    return nullptr;

  return getSubEntityByDataBlockId(*entity, dataBlockId);
}

int CompositeEditorViewport::getEntityRendInstExtraResourceIndex(IObjEntity &entity)
{
  IRendInstEntity *rendInstEntity = entity.queryInterface<IRendInstEntity>();
  if (!rendInstEntity)
    return -1;

  DagorAsset *asset = rendInstEntity->getAsset();
  if (!asset)
    return -1;

  return rendinst::addRIGenExtraResIdx(asset->getName(), -1, -1, {});
}

void CompositeEditorViewport::fillRenderElements(IObjEntity &entity, OutlineRenderer::RIElementsCache &renderElements,
  dag::Vector<DynamicRenderableSceneInstance *> &dynmodelElements)
{
  ICompositObj *compositObj = entity.queryInterface<ICompositObj>();
  if (compositObj)
  {
    const int subEntityCount = compositObj->getCompositSubEntityCount();
    for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
    {
      IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
      if (!subEntity)
        continue;

      fillRenderElements(*subEntity, renderElements, dynmodelElements);
    }
  }

  if (auto *rendInstEntity = entity.queryInterface<IRendInstEntity>())
  {
    const int ri_idx = rendInstEntity->getRIIndex();
    if (DAGOR_LIKELY(ri_idx >= 0))
    {
      TMatrix tm;
      getRendInstQuantizedTm(entity, tm);
      renderElements.emplace(ri_idx, tm);
    }
  }
  else if (auto *dynSceneRes = entity.queryInterface<IEntityGetDynSceneLodsRes>())
  {
    if (auto *sceneInstance = dynSceneRes->getSceneInstance(); sceneInstance)
      dynmodelElements.emplace_back(sceneInstance);
  }
}

void CompositeEditorViewport::fillPossiblePixelPerfectSelectionHits(IPixelPerfectSelectionService &pixelPerfectSelectionService,
  IObjEntity &entity, IObjEntity *entityForSelection, const Point3 &rayOrigin, const Point3 &rayDirection,
  dag::Vector<IPixelPerfectSelectionService::Hit> &hits)
{
  TMatrix tm;
  getRendInstQuantizedTm(entity, tm);

  float out_t;
  if (!ray_intersect_box(rayOrigin, rayDirection, entity.getBbox(), tm, out_t))
    return;

  ICompositObj *compositObj = entity.queryInterface<ICompositObj>();
  if (compositObj)
  {
    const int subEntityCount = compositObj->getCompositSubEntityCount();
    for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
    {
      IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
      if (!subEntity)
        continue;

      fillPossiblePixelPerfectSelectionHits(pixelPerfectSelectionService, *subEntity,
        entityForSelection ? entityForSelection : subEntity, rayOrigin, rayDirection, hits);
    }
  }

  IPixelPerfectSelectionService::Hit hit;
  if (!pixelPerfectSelectionService.initializeHit(hit, entity))
    return;

  hit.transform = tm;
  hit.userData = entityForSelection ? entityForSelection : &entity;
  hits.emplace_back(hit);
}
