//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_smallTab.h>
#include <phys/dag_physSysInst.h>
#include <phys/dag_physUserData.h>
#include <util/dag_index16.h>


class TMatrix;
class GeomNodeTree;
class DynamicPhysObjectData;

#ifndef NO_3D_GFX
class DynamicRenderableSceneLodsResource;
class DynamicRenderableSceneInstance;
#endif


#define DynamicPhysObject DynamicPhysObjectClass<PhysWorld>

template <class TPhysWorld = PhysWorld>
class DynamicPhysObjectClass
{
public:
  DynamicPhysObjectClass();
  ~DynamicPhysObjectClass();


  static DynamicPhysObjectClass *create(const DynamicPhysObjectData *data, PhysWorld *world, const TMatrix &tm, uint16_t fgroup = 0,
    uint16_t fmask = 0);

  void init(const DynamicPhysObjectData *data, PhysWorld *world, const TMatrix &tm, uint16_t fgroup = 0, uint16_t fmask = 0);

  void resetTm(const TMatrix &tm);

#ifndef NO_3D_GFX
  int getModelCount() const { return modelEntries.size(); }

  DynamicRenderableSceneInstance *getModel(int index) const { return modelEntries[index]->model; }

  void replaceModel(int index, DynamicRenderableSceneLodsResource *res);
#endif // NO_3D_GFX

  PhysSystemInstance *getPhysSys() const { return physSys; }

  GeomNodeTree *getNodeTree() const { return nodeTree; }

  const DynamicPhysObjectData *getData() const { return data; }

#ifndef NO_3D_GFX
  void beforeRender(const Point3 &cam_pos);
  void render(real opacity = 1);
  void renderTrans(real opacity = 1);
  void getBodyVisualTm(int index, TMatrix &tm);
#endif

protected:
  const DynamicPhysObjectData *data;

#ifndef NO_3D_GFX
  struct NodeAlignCtrl
  {
    dag::Index16 node0Id, node1Id, twistId[3];
    int16_t twistCnt = 0;
    float angDiff = 0;
  };
  struct ModelEntry
  {
    DynamicRenderableSceneInstance *model;

    SmallTab<TMatrix *, MidmemAlloc> nodeHelpers;
    SmallTab<dag::Index16, MidmemAlloc> treeIndex;
    Tab<NodeAlignCtrl> nodeAlignCtrl;
  };
  SmallTab<ModelEntry *> modelEntries;
#endif // NO_3D_GFX

  PhysSystemInstance *physSys;
  GeomNodeTree *nodeTree;

  PhysObjectUserData ud = {_MAKE4C('DPOJ')};
};
