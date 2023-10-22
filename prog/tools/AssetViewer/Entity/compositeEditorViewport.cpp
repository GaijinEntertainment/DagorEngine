#include "compositeEditorViewport.h"
#include "../av_appwnd.h"
#include <assets/asset.h>
#include <de3_baseInterfaces.h>
#include <de3_composit.h>
#include <de3_dataBlockIdHolder.h>
#include <de3_entityGetSceneLodsRes.h>
#include <de3_objEntity.h>
#include <debug/dag_debug3d.h>
#include <math/dag_rayIntersectBox.h>
#include <math/dag_TMatrix.h>
#include <rendInst/rendInstExtra.h>
#include <winGuiWrapper/wgw_input.h>

IObjEntity *CompositeEditorViewport::getHitSubEntity(IGenViewportWnd *wnd, int x, int y, IObjEntity &entity)
{
  if (!get_app().isCompositeEditorShown())
    return nullptr;

  Point3 dir, world;
  wnd->clientToWorld(Point2(x, y), world, dir);

  pixelPerfectSelectionHitsCache.clear();
  fillPossiblePixelPerfectSelectionHits(entity, nullptr, world, dir, pixelPerfectSelectionHitsCache);
  pixelPerfectEntitySelector.getHitsAt(*wnd, x, y, pixelPerfectSelectionHitsCache);

  return getNearestPixelPerfectSelectionHit(pixelPerfectSelectionHitsCache);
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

void CompositeEditorViewport::handleKeyPress(IGenViewportWnd *wnd, int vk, int modif, IObjEntity *entity)
{
  if (vk == wingw::V_DELETE)
  {
    BBox3 bbox;
    if (getSelectionBox(entity, bbox)) // Delete rendered nodes only.
    {
      get_app().getCompositeEditor().deleteSelectedNode();
      wnd->activate();
    }
  }
  else if (vk == 'Q')
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_None);
  }
  else if (vk == 'W')
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_Move);
  }
  else if (vk == 'E')
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_Rotate);
  }
  else if (vk == 'R')
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_Scale);
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

  renderElementsCache.clear();
  fillRenderElements(*selectedSubEntity, renderElementsCache);
  if (renderElementsCache.empty())
    return;

  outlineRenderer.render(wnd, renderElementsCache);
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

void CompositeEditorViewport::getRendInstQuantizedTm(IObjEntity &entity, TMatrix &tm)
{
  const IRendInstEntity *rendInstEntity = entity.queryInterface<IRendInstEntity>();
  if (!rendInstEntity || !rendInstEntity->getRendInstQuantizedTm(tm))
    entity.getTm(tm);
}

IObjEntity *CompositeEditorViewport::getNearestPixelPerfectSelectionHit(dag::ConstSpan<PixelPerfectEntitySelector::Hit> hits)
{
  const PixelPerfectEntitySelector::Hit *nearestHit = nullptr;
  for (const PixelPerfectEntitySelector::Hit &hit : hits)
  {
    G_ASSERT(hit.entityForSelection);

    if (!nearestHit || hit.z > nearestHit->z)
      nearestHit = &hit;
  }

  return nearestHit ? nearestHit->entityForSelection : nullptr;
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

void CompositeEditorViewport::fillRenderElements(IObjEntity &entity, dag::Vector<OutlineRenderer::RenderElement> &renderElements)
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

      fillRenderElements(*subEntity, renderElements);
    }
  }

  const int rendInstExtraResourceIndex = getEntityRendInstExtraResourceIndex(entity);
  DynamicRenderableSceneInstance *sceneInstance = nullptr;
  if (rendInstExtraResourceIndex < 0)
  {
    IEntityGetDynSceneLodsRes *dynSceneRes = entity.queryInterface<IEntityGetDynSceneLodsRes>();
    if (dynSceneRes)
      sceneInstance = dynSceneRes->getSceneInstance();
    if (!sceneInstance)
      return;
  }

  OutlineRenderer::RenderElement renderElement;
  renderElement.rendInstExtraResourceIndex = rendInstExtraResourceIndex;
  renderElement.sceneInstance = sceneInstance;
  getRendInstQuantizedTm(entity, renderElement.transform);
  renderElements.emplace_back(renderElement);
}

void CompositeEditorViewport::fillPossiblePixelPerfectSelectionHits(IObjEntity &entity, IObjEntity *entityForSelection,
  const Point3 &rayOrigin, const Point3 &rayDirection, dag::Vector<PixelPerfectEntitySelector::Hit> &hits)
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

      fillPossiblePixelPerfectSelectionHits(*subEntity, entityForSelection ? entityForSelection : subEntity, rayOrigin, rayDirection,
        hits);
    }
  }

  const int rendInstExtraResourceIndex = getEntityRendInstExtraResourceIndex(entity);
  DynamicRenderableSceneInstance *sceneInstance = nullptr;
  if (rendInstExtraResourceIndex < 0)
  {
    IEntityGetDynSceneLodsRes *dynSceneRes = entity.queryInterface<IEntityGetDynSceneLodsRes>();
    if (dynSceneRes)
      sceneInstance = dynSceneRes->getSceneInstance();
    if (!sceneInstance)
      return;
  }

  PixelPerfectEntitySelector::Hit hit;
  hit.entityForSelection = entityForSelection ? entityForSelection : &entity;
  hit.rendInstExtraResourceIndex = rendInstExtraResourceIndex;
  hit.sceneInstance = sceneInstance;
  hit.transform = tm;
  hit.z = 0.0f;
  hits.emplace_back(hit);
}
