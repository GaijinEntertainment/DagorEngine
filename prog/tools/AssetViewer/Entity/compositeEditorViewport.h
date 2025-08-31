// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "outlineRenderer.h"
#include <dag/dag_vector.h>
#include <de3_pixelPerfectSelectionService.h>

class BBox3;
class IGenViewportWnd;
class IObjEntity;
class Point3;
class TMatrix;

class CompositeEditorViewport
{
public:
  void registerMenuAccelerators();
  void handleViewportAcceleratorCommand(unsigned id, IGenViewportWnd &wnd, IObjEntity *entity);
  IObjEntity *getHitSubEntity(IGenViewportWnd *wnd, int x, int y, IObjEntity &entity);
  bool getSelectionBox(IObjEntity *entity, BBox3 &box) const;
  void handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, IObjEntity *entity);
  void renderObjects(IGenViewportWnd &wnd, IObjEntity *entity);
  void renderTransObjects(IObjEntity *entity);
  void invalidateCache();

  static IObjEntity *getSelectedSubEntity(IObjEntity *entity);

private:
  struct CompositeEntityHit
  {
    IObjEntity *entity;
    float distance;
  };

  static void getRendInstQuantizedTm(IObjEntity &entity, TMatrix &tm);
  static IObjEntity *getNearestHit(IObjEntity &entity, const Point3 &from, const Point3 &dir);
  static void getAllHits(IObjEntity &entity, const Point3 &from, const Point3 &dir, dag::Vector<CompositeEntityHit> &hits);
  static unsigned getEntityDataBlockId(IObjEntity &entity);
  static IObjEntity *getSubEntityByDataBlockId(IObjEntity &entity, unsigned dataBlockId);
  static int getEntityRendInstExtraResourceIndex(IObjEntity &entity);
  static void fillRenderElements(IObjEntity &entity, OutlineRenderer::RIElementsCache &renderElements,
    dag::Vector<DynamicRenderableSceneInstance *> &dynmodelElements);

  static void fillPossiblePixelPerfectSelectionHits(IPixelPerfectSelectionService &pixelPerfectSelectionService, IObjEntity &entity,
    IObjEntity *entityForSelection, const Point3 &rayOrigin, const Point3 &rayDirection,
    dag::Vector<IPixelPerfectSelectionService::Hit> &hits);

  OutlineRenderer outlineRenderer;
  OutlineRenderer::RIElementsCache riElementsCache;
  dag::Vector<DynamicRenderableSceneInstance *> dynmodelElementsCache;
  dag::Vector<IPixelPerfectSelectionService::Hit> pixelPerfectSelectionHitsCache;
  IObjEntity *cachedSelectedSubEntity = nullptr;
};
