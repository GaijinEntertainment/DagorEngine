//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dag/dag_vector.h>
#include <math/dag_TMatrix.h>

class DynamicRenderableSceneInstance;
class IGenViewportWnd;
class IObjEntity;
class RenderableInstanceLodsResource;

class IPixelPerfectSelectionService
{
public:
  struct Hit // -V730 Not all members are inited.
  {
    TMatrix transform;                                       // Mandatory input.
    int rendInstExtraResourceIndex;                          // Mandatory input.
    RenderableInstanceLodsResource *riLodResource = nullptr; // Optional input.
    DynamicRenderableSceneInstance *sceneInstance = nullptr; // Optional input.
    void *userData = nullptr;                                // Optional input.
    float z = 0.0f;                                          // Output. The hit's Z coordinate is stored here.
  };

  static constexpr unsigned HUID = 0xC7DED060u; // IPixelPerfectSelectorService

  // Returns with true if the hit structure has been successfully initialized.
  virtual bool initializeHit(Hit &hit, IObjEntity &entity) = 0;

  // hits: both input and output.
  // As input it contains the potential hits to check.
  // As output it contains the actual hits, ordered from closest to farthest.
  virtual void getHits(IGenViewportWnd &wnd, int x, int y, dag::Vector<Hit> &hits) = 0;
};
