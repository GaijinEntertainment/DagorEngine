//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_smallTab.h>
#include <phys/dag_physSysInst.h>
#include <phys/dag_physUserData.h>
#include <phys/dag_physTwistCtrl.h>
#include <util/dag_index16.h>


class TMatrix;
class GeomNodeTree;
class DynamicPhysObjectData;

class DynamicRenderableSceneLodsResource;
class DynamicRenderableSceneInstance;


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

  int getModelCount() const { return modelEntries.size(); }
  DynamicRenderableSceneInstance *getModel(int index) const { return modelEntries[index]->model; }
  void replaceModel(int index, DynamicRenderableSceneLodsResource *res);

  void getBodyVisualTm(int body_index, TMatrix &tm);
  void beforeRender(const Point3 &cam_pos);

  PhysSystemInstance *getPhysSys() const { return physSys; }

  GeomNodeTree *getNodeTree() const { return nodeTree; }

  const DynamicPhysObjectData *getData() const { return data; }

protected:
  const DynamicPhysObjectData *data;

  struct ModelEntry
  {
    DynamicRenderableSceneInstance *model;

    SmallTab<TMatrix *, MidmemAlloc> nodeHelpers;
    SmallTab<dag::Index16, MidmemAlloc> treeIndex;
    Tab<TwistCtrlParams> twistCtrl;
  };
  SmallTab<ModelEntry *> modelEntries;

  PhysSystemInstance *physSys;
  GeomNodeTree *nodeTree;

  PhysObjectUserData ud = {_MAKE4C('DPOJ')};
};
