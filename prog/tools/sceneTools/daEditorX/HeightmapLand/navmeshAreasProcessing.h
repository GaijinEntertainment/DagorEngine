// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include "hmlSplineObject.h"

class NavmeshAreasProcessing
{
public:
  void init(HmapLandObjectEditor *obj_ed, const DataBlock *navmesh_props, int navmesh_idx);
  void onChange(int pcb_id);
  void onClick(int pcb_id);
  void onObjectsRemove();
  void onSplineSelectionChanged(SplineObject *obj);
  void setPropPanel(PropPanel::ContainerPropertyControl *prop_panel);
  void fillNavmeshAreasPanel();

private:
  struct AreaData
  {
    SplineObject *spline;
    PropPanel::TLeafHandle leafHandle;
  };

  int navmeshIdx = -1;
  int baseOfs = -1;
  dag::Vector<AreaData> areasData;
  PropPanel::ContainerPropertyControl *panel = nullptr;
  HmapLandObjectEditor *objEd = nullptr;
  PropPanel::ContainerPropertyControl *tree = nullptr;
  const DataBlock *navmeshProps = nullptr;

  void onAreaTypeChange();
  void createArea();
  void deleteSelectedAreas();
  void selectAreas();
  void updateTree();
  void updateButtons();
  void updateSplines();
  int getAreaIndexByLeaf(PropPanel::TLeafHandle leafHandle);
  int getAreaIndexByName(const char *name);
};
