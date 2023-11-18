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
  IObjEntity *getHitSubEntity(IGenViewportWnd *wnd, int x, int y, IObjEntity &entity);
  bool getSelectionBox(IObjEntity *entity, BBox3 &box) const;
  void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif, IObjEntity *entity);
  void handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, IObjEntity *entity);
  void renderObjects(IGenViewportWnd &wnd, IObjEntity *entity);
  void renderTransObjects(IObjEntity *entity);

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
  static void fillRenderElements(IObjEntity &entity, dag::Vector<OutlineRenderer::RenderElement> &renderElements);

  static void fillPossiblePixelPerfectSelectionHits(IPixelPerfectSelectionService &pixelPerfectSelectionService, IObjEntity &entity,
    IObjEntity *entityForSelection, const Point3 &rayOrigin, const Point3 &rayDirection,
    dag::Vector<IPixelPerfectSelectionService::Hit> &hits);

  OutlineRenderer outlineRenderer;
  dag::Vector<OutlineRenderer::RenderElement> renderElementsCache;
  dag::Vector<IPixelPerfectSelectionService::Hit> pixelPerfectSelectionHitsCache;
};
