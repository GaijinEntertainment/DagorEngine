//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <phys/dag_physDecl.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_DObject.h>
#include <util/dag_index16.h>
#include <math/dag_geomTree.h>


class TMatrix;
class DynamicPhysObjectData;
class DynamicRenderableSceneInstance;


#define SimplePhysObject SimplePhysObjectClass<PhysWorld>

template <class TPhysWorld = PhysWorld>
class SimplePhysObjectClass
{
public:
  SimplePhysObjectClass() = default;
  ~SimplePhysObjectClass();


  static SimplePhysObjectClass *create(const DynamicPhysObjectData *data, PhysWorld *world, const TMatrix *tm = NULL);

  void init(const DynamicPhysObjectData *data, PhysWorld *world);


  void resetTm(const TMatrix &tm);

  PhysBody *getBody() const { return body; }
  GeomNodeTree *getNodeTree() const { return nodeTree.get(); }

  const TMatrix &getModel2BodyTm() const { return *tmModel2body; }
  const TMatrix &getBody2ModelTm() const { return *tmBody2model; }

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

protected:
  PhysBody *body = nullptr;
  GeomNodeTreeUniquePtr nodeTree;
  const TMatrix *tmModel2body = nullptr;
  const TMatrix *tmBody2model = nullptr;

  DynamicRenderableSceneInstance *model = nullptr;
  SmallTab<dag::Index16, MidmemAlloc> treeHelpers;

  const TMatrix *extRenderTm = nullptr;
  Ptr<DObject> physRes;
};
