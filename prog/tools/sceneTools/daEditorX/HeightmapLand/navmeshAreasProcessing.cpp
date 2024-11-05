// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "navmeshAreasProcessing.h"
#include "hmlCm.h"
#include "hmlObjectsEditor.h"
#include "hmlSplinePoint.h"
#include <propPanel/control/container.h>

void NavmeshAreasProcessing::init(HmapLandObjectEditor *obj_ed, const DataBlock *navmesh_props, int navmesh_idx)
{
  objEd = obj_ed;
  navmeshIdx = navmesh_idx;
  navmeshProps = navmesh_props;
  baseOfs = PID_NAVMESH_START + NM_PARAMS_COUNT * navmeshIdx;

  updateSplines();
}

void NavmeshAreasProcessing::onChange(int pcb_id)
{
  const int navMeshParam = (pcb_id - PID_NAVMESH_START) % NM_PARAMS_COUNT;
  switch (navMeshParam)
  {
    case NM_PARAM_AREATYPE: onAreaTypeChange(); break;
    case NM_PARAM_AREAS_TREE: selectAreas(); break;
  }
}
void NavmeshAreasProcessing::onClick(int pcb_id)
{
  const int navMeshParam = (pcb_id - PID_NAVMESH_START) % NM_PARAMS_COUNT;
  switch (navMeshParam)
  {
    case NM_PARAM_AREAS_CREATE: createArea(); break;
    case NM_PARAM_AREAS_DELETE: deleteSelectedAreas(); break;
  }
}

void NavmeshAreasProcessing::onObjectsRemove()
{
  if (tree == nullptr)
    return;
  updateSplines();
  updateTree();
  updateButtons();
}

void NavmeshAreasProcessing::setPropPanel(PropPanel::ContainerPropertyControl *prop_panel) { panel = prop_panel; }

void NavmeshAreasProcessing::fillNavmeshAreasPanel()
{
  tree = panel->createMultiSelectTreeCheckbox(baseOfs + NM_PARAM_AREAS_TREE, "Navmesh areas", hdpi::_pxScaled(300));
  PropPanel::ContainerPropertyControl *treeContainer = tree->getContainer();

  updateTree();

  panel->createButton(baseOfs + NM_PARAM_AREAS_CREATE, "Create new area");
  panel->createButton(baseOfs + NM_PARAM_AREAS_DELETE, "Delete", false);
}

void NavmeshAreasProcessing::onAreaTypeChange()
{
  if (navmeshProps->getInt("navArea", 0) != NM_PARAM_AREATYPE_POLY - NM_PARAM_AREATYPE_MAIN)
    return;

  if (areasData.size() == 0)
  {
    createArea();
  }
  else
  {
    updateSplines();
    updateTree();
  }
}

void NavmeshAreasProcessing::createArea()
{
  SplineObject *poly = new SplineObject(true);
  poly->setEditLayerIdx(EditLayerProps::activeLayerIdx[poly->lpIndex()]);
  poly->setNavmeshIdx(navmeshIdx);
  objEd->addObject(poly, false);

  Point2 minPos = navmeshProps->getPoint2("rect0", Point2(-500, -500));
  Point2 maxPos = navmeshProps->getPoint2("rect1", Point2(500, 500));
  Point3 points[] = {Point3::x0y(minPos), Point3(minPos.x, 0, maxPos.y), Point3::x0y(maxPos), Point3(maxPos.x, 0, minPos.y)};
  for (int i = 0; i < 4; ++i)
  {
    SplinePointObject *pt = new SplinePointObject();
    pt->setPos(points[i]);
    pt->arrId = poly->points.size();
    pt->spline = poly;
    objEd->addObject(pt, false);
  }

  for (int i = 0; i < areasData.size(); ++i)
    areasData[i].spline->selectObject(false);
  poly->selectObject();
  poly->onCreated();

  String name = String(0, "Navmesh area 001");
  while (getAreaIndexByName(name) >= 0)
    ::make_next_numbered_name(name, 3);
  poly->setName(name);

  areasData.emplace_back(AreaData{poly, nullptr});

  PropPanel::TLeafHandle leaf = tree->createTreeLeaf(0, name, "mesh");
  tree->setCheckboxValue(leaf, true);
  areasData.back().leafHandle = leaf;
  tree->setSelLeaf(leaf);
  updateButtons();
}

void NavmeshAreasProcessing::deleteSelectedAreas()
{
  PtrTab<SplineObject> splineObjects;
  dag::Vector<PropPanel::TLeafHandle> selectedLeafs;
  tree->getSelectedLeafs(selectedLeafs);
  for (int i = 0; i < selectedLeafs.size(); ++i)
  {
    int idx = getAreaIndexByLeaf(selectedLeafs[i]);
    if (idx == -1)
      continue;
    splineObjects.push_back(areasData[idx].spline);
  }
  objEd->removeObjects((RenderableEditableObject **)splineObjects.data(), splineObjects.size(), false);
}

void NavmeshAreasProcessing::selectAreas()
{
  for (int i = 0; i < areasData.size(); ++i)
    areasData[i].spline->selectObject(false);

  dag::Vector<PropPanel::TLeafHandle> selectedLeafs;
  tree->getSelectedLeafs(selectedLeafs);
  for (int i = 0; i < selectedLeafs.size(); ++i)
  {
    int idx = getAreaIndexByLeaf(selectedLeafs[i]);
    if (idx == -1)
      continue;
    areasData[idx].spline->selectObject();
  }

  updateButtons();
}

void NavmeshAreasProcessing::updateTree()
{
  tree->clear();
  for (int i = 0; i < areasData.size(); ++i)
  {
    auto &areaData = areasData[i];
    PropPanel::TLeafHandle leaf = tree->createTreeLeaf(0, areaData.spline->getName(), "mesh");
    tree->setCheckboxValue(leaf, true);
    areaData.leafHandle = leaf;
  }
}

void NavmeshAreasProcessing::updateButtons()
{
  bool enableDelete = false;
  if (areasData.size() > 1)
  {
    dag::Vector<PropPanel::TLeafHandle> selectedLeafs;
    tree->getSelectedLeafs(selectedLeafs);
    enableDelete = selectedLeafs.size() > 0 && selectedLeafs.size() < areasData.size();
  }
  panel->setEnabledById(baseOfs + NM_PARAM_AREAS_DELETE, enableDelete);
}

void NavmeshAreasProcessing::updateSplines()
{
  areasData.clear();
  const int splinesCount = objEd->splinesCount();
  for (int i = 0; i < splinesCount; ++i)
  {
    const auto &spline = objEd->getSpline(i);
    if (spline->getProps().navmeshIdx != navmeshIdx)
      continue;

    areasData.emplace_back(AreaData{spline, nullptr});
  }
}

int NavmeshAreasProcessing::getAreaIndexByLeaf(PropPanel::TLeafHandle leafHandle)
{
  if (!leafHandle)
    return -1;

  for (int i = 0; i < areasData.size(); ++i)
    if (areasData[i].leafHandle == leafHandle)
      return i;

  return -1;
}

int NavmeshAreasProcessing::getAreaIndexByName(const char *name)
{
  if (!name)
    return -1;

  for (int i = 0; i < areasData.size(); ++i)
    if (strcmp(areasData[i].spline->getName(), name) == 0)
      return i;

  return -1;
}
