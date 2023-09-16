//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <phys/dag_physDecl.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_DObject.h>
#include <util/dag_index16.h>


class TMatrix;
class GeomNodeTree;
class DynamicPhysObjectData;

#ifndef NO_3D_GFX
class DynamicRenderableSceneInstance;
#endif


#define SimplePhysObject SimplePhysObjectClass<PhysWorld>

template <class TPhysWorld = PhysWorld>
class SimplePhysObjectClass
{
public:
  SimplePhysObjectClass();
  ~SimplePhysObjectClass();


  static SimplePhysObjectClass *create(const DynamicPhysObjectData *data, PhysWorld *world, const TMatrix *tm = NULL);

  void init(const DynamicPhysObjectData *data, PhysWorld *world);


  void resetTm(const TMatrix &tm);

  PhysBody *getBody() const { return body; }
  GeomNodeTree *getNodeTree() const { return nodeTree; }

  const TMatrix &getModel2BodyTm() const { return *tmModel2body; }
  const TMatrix &getBody2ModelTm() const { return *tmBody2model; }

#ifndef NO_3D_GFX
  DynamicRenderableSceneInstance *getModel() const { return model; }
  void updateModelTms();

  void setExternalRenderBodyTmPtr(const TMatrix *tm) { extRenderTm = tm; }
  void getBodyVisualTm(TMatrix &tm)
  {
    if (extRenderTm)
      tm = *extRenderTm;
    else
      body->getTm(tm);
  }
#endif

protected:
  PhysBody *body;
  GeomNodeTree *nodeTree;
  const TMatrix *tmModel2body;
  const TMatrix *tmBody2model;

#ifndef NO_3D_GFX
  DynamicRenderableSceneInstance *model;
  SmallTab<dag::Index16, MidmemAlloc> treeHelpers;

  const TMatrix *extRenderTm;
#endif // NO_3D_GFX
  Ptr<DObject> physRes;
};
