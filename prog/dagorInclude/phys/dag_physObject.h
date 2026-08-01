//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_smallTab.h>
#include <generic/dag_functionRef.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physUserData.h>
#include <generic/dag_tab.h>
#include <util/dag_index16.h>
#include <math/dag_geomTree.h>

class TMatrix;
class DynamicPhysObjectData;
struct TwistCtrlParams;
class DynamicRenderableSceneLodsResource;
class DynamicRenderableSceneInstance;

#define DynamicPhysObject DynamicPhysObjectClass<PhysWorld>

template <class TPhysWorld = PhysWorld>
class DynamicPhysObjectClass
{
public:
  DynamicPhysObjectClass() = default;
  DynamicPhysObjectClass(const DynamicPhysObjectClass &) = delete;
  DynamicPhysObjectClass &operator=(const DynamicPhysObjectClass &) = delete;
  ~DynamicPhysObjectClass();

  static DynamicPhysObjectClass *create(const DynamicPhysObjectData *data, PhysWorld *world, const TMatrix &tm, uint16_t fgroup = 0,
    uint16_t fmask = 0);

  void init(const DynamicPhysObjectData *data, PhysWorld *world, const TMatrix &tm, uint16_t fgroup = 0, uint16_t fmask = 0);

  void resetTm(const TMatrix &tm);

  int getModelCount() const { return modelEntries.size(); }
  DynamicRenderableSceneInstance *getModel(int index) const { return modelEntries[index]->model; }
  void replaceModel(int index, DynamicRenderableSceneLodsResource *res);

  void getBodyVisualTm(int body_index, TMatrix &tm);
  void beforeRender(const Point3 &cam_pos, dag::FunctionRef<TMatrix(int)> get_body_tm = {},
    dag::FunctionRef<bool(int)> is_body_visible = {});

  PhysSystemInstance *getPhysSys() const { return physSys; }

  GeomNodeTree *getNodeTree() const { return nodeTree.get(); }

  const DynamicPhysObjectData *getData() const { return data; }

protected:
  const DynamicPhysObjectData *data = nullptr;

  struct ModelEntry
  {
    DynamicRenderableSceneInstance *model = nullptr;

    SmallTab<TMatrix *, MidmemAlloc> nodeHelpers;
    SmallTab<dag::Index16, MidmemAlloc> treeIndex;
    SmallTab<int, MidmemAlloc> nodeToBody;
    Tab<TwistCtrlParams> twistCtrl;
  };
  SmallTab<ModelEntry *> modelEntries;

  PhysSystemInstance *physSys = nullptr;
  GeomNodeTreeUniquePtr nodeTree;

  PhysObjectUserData ud{_MAKE4C('DPOJ')};
};
