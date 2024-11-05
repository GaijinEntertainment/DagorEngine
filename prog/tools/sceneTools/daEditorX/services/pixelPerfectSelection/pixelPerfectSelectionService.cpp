// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_pixelPerfectSelectionService.h>
#include "pixelPerfectSelection.h"
#include <assets/asset.h>
#include <de3_baseInterfaces.h>
#include <de3_entityGetSceneLodsRes.h>
#include <de3_objEntity.h>
#include <rendInst/rendInstExtra.h>

namespace rendinst
{
extern int maxExtraRiCount;
}

class PixelPerfectSelectionService : public IPixelPerfectSelectionService
{
  static int getEntityRendInstExtraResourceIndex(IObjEntity &entity)
  {
    // If riExtra rendering is not enabled.
    if (rendinst::maxExtraRiCount <= 0)
      return -1;

    IRendInstEntity *rendInstEntity = entity.queryInterface<IRendInstEntity>();
    if (!rendInstEntity)
      return -1;

    DagorAsset *asset = rendInstEntity->getAsset();
    if (!asset)
      return -1;

    return rendinst::addRIGenExtraResIdx(asset->getName(), -1, -1, {});
  }

  virtual bool initializeHit(Hit &hit, IObjEntity &entity) override
  {
    const int rendInstExtraResourceIndex = getEntityRendInstExtraResourceIndex(entity);
    RenderableInstanceLodsResource *riLodResource = nullptr;
    DynamicRenderableSceneInstance *sceneInstance = nullptr;
    if (rendInstExtraResourceIndex < 0)
    {
      IEntityGetRISceneLodsRes *sceneRes = entity.queryInterface<IEntityGetRISceneLodsRes>();
      if (sceneRes)
      {
        riLodResource = sceneRes->getSceneLodsRes();
        if (!riLodResource)
          return false;
      }
      else
      {
        IEntityGetDynSceneLodsRes *dynSceneRes = entity.queryInterface<IEntityGetDynSceneLodsRes>();
        if (!dynSceneRes)
          return false;

        sceneInstance = dynSceneRes->getSceneInstance();
        if (!sceneInstance)
          return false;
      }
    }

    hit.rendInstExtraResourceIndex = rendInstExtraResourceIndex;
    hit.riLodResource = riLodResource;
    hit.sceneInstance = sceneInstance;
    return true;
  }

  virtual void getHits(IGenViewportWnd &wnd, int x, int y, dag::Vector<Hit> &hits) override
  {
    pixelPerfectSelection.getHitsAt(wnd, x, y, hits);
  }

  PixelPerfectSelection pixelPerfectSelection;
};

PixelPerfectSelectionService pixe_perfect_selection_service;

void *get_pixel_perfect_selection_service() { return &pixe_perfect_selection_service; }
