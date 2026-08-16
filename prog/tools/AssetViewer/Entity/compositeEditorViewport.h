// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "outlineRenderer.h"
#include "editModeRenderer.h"
#include <dag/dag_vector.h>
#include <de3_pixelPerfectSelectionService.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>
#include <propPanel/control/menu.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/unique_ptr.h>

class IEditorCommandSystem;
class IGenViewportWnd;
class IObjEntity;
class Point3;

class CompositeEditorViewport : public PropPanel::IMenuEventHandler
{
public:
  CompositeEditorViewport();

  static void registerEditorCommands(IEditorCommandSystem &command_system);
  void registerMenuAccelerators();
  void handleViewportAcceleratorCommand(unsigned id, IGenViewportWnd &wnd, IObjEntity *entity);
  IObjEntity *getHitSubEntity(IGenViewportWnd *wnd, int x, int y, IObjEntity &entity);
  bool getSelectionBox(IObjEntity *entity, BBox3 &box) const;
  void handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, IObjEntity *entity, int key_modif);
  bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, IObjEntity *entity, int key_modif);
  bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, IObjEntity *entity, int key_modif);
  void renderObjects(IGenViewportWnd &wnd, IObjEntity *entity);
  void renderTransObjects(IGenViewportWnd &wnd, IObjEntity *entity);
  void updateImgui();
  void invalidateCache();

  // IMenuEventHandler
  int onMenuItemClick(unsigned id) override;

  static IObjEntity *getSelectedSubEntity(IObjEntity *entity);
  static void getSelectedSubEntities(IObjEntity *entity, dag::Vector<IObjEntity *> &entities, int &parent_idx);

private:
  struct CompositeEntityHit
  {
    IObjEntity *entity;
    float distance;
  };

  static IObjEntity *getNearestHit(IObjEntity &entity, const Point3 &from, const Point3 &dir);
  static void getAllHits(IObjEntity &entity, const Point3 &from, const Point3 &dir, dag::Vector<CompositeEntityHit> &hits);
  static unsigned getEntityDataBlockId(IObjEntity &entity);
  static IObjEntity *getSubEntityByDataBlockId(IObjEntity &entity, unsigned dataBlockId);
  static int getEntityRendInstExtraResourceIndex(IObjEntity &entity);
  static void fillRenderElements(IObjEntity &entity, OutlineRenderer::RIElementsCache &renderElements,
    dag::Vector<DynamicRenderableSceneInstance *> &dynmodelElements, unsigned excludeDataBlockId = 0);

  static void fillPossiblePixelPerfectSelectionHits(IPixelPerfectSelectionService &pixelPerfectSelectionService, IObjEntity &entity,
    IObjEntity *entityForSelection, const Point3 &rayOrigin, const Point3 &rayDirection,
    dag::Vector<IPixelPerfectSelectionService::Hit> &hits);

  dag::Vector<IObjEntity *> subEntitySelectionLookup;

  // relevant sub-entity data for transparent object rendering
  struct SubEntityNode
  {
    IObjEntity *entity = nullptr;

    bool isSelected = false;
    bool isSelectedParent = false;
    bool isAncestorSelectedParent = false;

    bool spatialsCached = false;
    BBox3 bbox;
    TMatrix tm;

    bool screenPosCached = false;
    eastl::optional<Point2> sPos;
  };

  static void cacheSubEntitySpatials(SubEntityNode &s);
  static void cacheSubEntityScreenPos(SubEntityNode &s, IGenViewportWnd &wnd);

  ska::flat_hash_map<unsigned int, SubEntityNode> subEntityNodeLookup;

  OutlineRenderer outlineRenderer;
  EditModeRenderer editModeRenderer;
  struct OutlineElementsCache
  {
    OutlineRenderer::RIElementsCache riElements;
    dag::Vector<DynamicRenderableSceneInstance *> dynmodelElements;
    E3DCOLOR color;
  };
  dag::Vector<OutlineElementsCache> outlineElementsCache;

  struct TintElementsCache
  {
    EditModeRenderer::RIElementsCache riElements;
    dag::Vector<DynamicRenderableSceneInstance *> dynmodelElements;
    EditModeRenderer::RIElementsCache occluderRiElements;
    dag::Vector<DynamicRenderableSceneInstance *> occluderDynmodelElements;
  };
  TintElementsCache tintCache;
  dag::Vector<IObjEntity *> cachedTintGhostEntities;

  dag::Vector<IPixelPerfectSelectionService::Hit> pixelPerfectSelectionHitsCache;

  dag::Vector<IObjEntity *> cachedSelectedSubEntities;
  int cachedParentIdx = -1;

  eastl::unique_ptr<PropPanel::IMenu> popupMenu;
};
