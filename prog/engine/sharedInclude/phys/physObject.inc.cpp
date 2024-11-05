#include <phys/dag_physObject.h>
#include <phys/dag_physSysInst.h>
#include <phys/dag_physics.h>
#include <math/dag_geomTree.h>
#include <math/dag_geomNodeUtils.h>
#include <shaders/dag_dynSceneRes.h>
#include <generic/dag_tabUtils.h>
#include <debug/dag_debug.h>
#include <math/twistCtrl.h>
#include <render/dynmodelRenderer.h>
#include <3d/dag_render.h>


template <>
DynamicPhysObject::DynamicPhysObjectClass() : physSys(NULL), nodeTree(NULL)
{}


template <>
DynamicPhysObject::~DynamicPhysObjectClass()
{
#ifndef NO_3D_GFX
  for (int i = 0; i < modelEntries.size(); i++)
  {
    del_it(modelEntries[i]->model);
    del_it(modelEntries[i]);
  }
#endif

  del_it(physSys);
  del_it(nodeTree);
}


template <>
void DynamicPhysObject::resetTm(const TMatrix &tm)
{
  if (physSys)
    physSys->resetTm(tm);
}


template <>
void DynamicPhysObject::init(const DynamicPhysObjectData *phys_obj_data, PhysWorld *world, const TMatrix &tm, uint16_t fgroup,
  uint16_t fmask)
{
  if (phys_obj_data->physRes && world)
    physSys = new PhysSystemInstance(phys_obj_data->physRes, world, &tm, &ud, fgroup, fmask);
  if (phys_obj_data->nodeTree)
    nodeTree = new GeomNodeTree(*phys_obj_data->nodeTree);

#ifndef NO_3D_GFX
  for (int nModel = 0; nModel < phys_obj_data->models.size(); nModel++)
  {
    if (!phys_obj_data->models[nModel])
      continue;

    ModelEntry *entry = new ModelEntry;
    modelEntries.push_back(entry);

    entry->model = new DynamicRenderableSceneInstance(phys_obj_data->models[nModel]);

    clear_and_shrink(entry->nodeHelpers);
    clear_and_shrink(entry->treeIndex);

    if (physSys && nodeTree)
    {
      const RoNameMapEx &map = entry->model->getLodsResource()->getNames().node;

      clear_and_resize(entry->nodeHelpers, nodeTree->nodeCount());
      for (dag::Index16 j(0), je(entry->nodeHelpers.size()); j != je; ++j)
        entry->nodeHelpers[j.index()] = physSys->getTmHelper(nodeTree->getNodeName(j));

      clear_and_resize(entry->treeIndex, map.nameCount());
      iterate_names(map, [&](int i, const char *name) {
        entry->treeIndex[i] = nodeTree->findNodeIndex(name);
        G_ASSERT((int)entry->treeIndex[i] < 0x8000);
        if (!entry->treeIndex[i])
          if (TMatrix *wtm = physSys->getTmHelper(name))
          {
            entry->treeIndex[i] = dag::Index16(find_value_idx(entry->nodeHelpers, wtm));
            G_ASSERT(entry->treeIndex[i]);
            entry->treeIndex[i] = dag::Index16(entry->treeIndex[i].index() | 0x8000);
          }
      });

      dag::ConstSpan<PhysicsResource::NodeAlignCtrl> c = phys_obj_data->physRes->getNodeAlignCtrl();
      entry->nodeAlignCtrl.reserve(c.size());
      for (int i = 0; i < c.size(); i++)
      {
        NodeAlignCtrl &ctrl = entry->nodeAlignCtrl.push_back();
        ctrl.node0Id = nodeTree->findNodeIndex(c[i].node0);
        ctrl.node1Id = nodeTree->findNodeIndex(c[i].node1);
        ctrl.angDiff = c[i].angDiff;
        for (int j = 0; j < countof(c[i].twist) && !c[i].twist[j].empty(); j++)
        {
          ctrl.twistId[j] = nodeTree->findNodeIndex(c[i].twist[j]);
          if (!ctrl.twistId[j])
          {
            ctrl.twistCnt = 0;
            break;
          }
          ctrl.twistCnt = j + 1;
        }
        if (!ctrl.node0Id || !ctrl.node1Id || !ctrl.twistCnt)
          entry->nodeAlignCtrl.pop_back();
      }
    }
    else if (physSys)
    {
      const RoNameMapEx &map = entry->model->getLodsResource()->getNames().node;
      clear_and_resize(entry->nodeHelpers, map.nameCount());
      mem_set_0(entry->nodeHelpers);

      iterate_names(map, [&](int i, const char *name) {
        if (TMatrix *tmHelper = physSys->getTmHelper(name))
        {
          entry->nodeHelpers[i] = tmHelper;
          if (!entry->nodeHelpers[0])
            entry->nodeHelpers[0] = tmHelper;
        }
      });
    }
  }
#endif // NO_3D_GFX
}


template <>
DynamicPhysObject *DynamicPhysObject::create(const DynamicPhysObjectData *data, PhysWorld *world, const TMatrix &tm, uint16_t fgroup,
  uint16_t fmask)
{
  if (!data || !world)
    return NULL;

  DynamicPhysObject *obj = new (midmem) DynamicPhysObject;
  obj->data = data;
  obj->init(data, world, tm, fgroup, fmask);

  return obj;
}


#ifndef NO_3D_GFX
template <>
void DynamicPhysObject::replaceModel(int index, DynamicRenderableSceneLodsResource *res)
{
  G_ASSERT(index >= 0);
  G_ASSERT(index < modelEntries.size());
  delete modelEntries[index]->model;
  modelEntries[index]->model = new DynamicRenderableSceneInstance(res);
}

template <>
void DynamicPhysObject::beforeRender(const Point3 &cam_pos)
{
  if (physSys && modelEntries.size() == 1 && physSys->getBodyCount() == 1)
  {
    // "single-body at root" case
    TMatrix itm = physSys->getPhysicsResource()->getBodies()[0].tmInvert;
    TMatrix tm;
    physSys->getBody(0)->getTm(tm);

    tm = tm * itm;
    if (nodeTree)
    {
      nodeTree->setRootTmScalar(tm);
      nodeTree->invalidateWtm();
      nodeTree->calcWtm();
    }

    modelEntries[0]->model->setNodeWtm(0, tm);
    if (modelEntries[0]->model->getLodsResource()->getNames().node.nameCount() == 1)
    {
      modelEntries[0]->model->beforeRender(cam_pos);
      return;
    }
  }

  if (physSys)
  {
    physSys->updateTms();

    for (ModelEntry *entry : modelEntries)
    {
      if (nodeTree)
      {
        nodeTree->invalidateWtm(dag::Index16(0));
        for (dag::Index16 i(0), ie(entry->nodeHelpers.size()); i != ie; ++i)
          if (entry->nodeHelpers[i.index()])
          {
            nodeTree->partialCalcWtm(i);
            nodeTree->setNodeWtmScalar(i, *entry->nodeHelpers[i.index()]);
            nodeTree->invalidateWtm(i);
          }
        for (int j = 0; j < entry->nodeAlignCtrl.size(); j++)
          apply_twist_ctrl(*nodeTree, entry->nodeAlignCtrl[j].node0Id, entry->nodeAlignCtrl[j].node1Id,
            make_span(entry->nodeAlignCtrl[j].twistId, entry->nodeAlignCtrl[j].twistCnt), entry->nodeAlignCtrl[j].angDiff);

        nodeTree->calcWtm();
        for (int i = 0; i < entry->treeIndex.size(); ++i)
          if (!entry->treeIndex[i])
            continue;
          else if (entry->treeIndex[i].index() < 0x8000)
            entry->model->setNodeWtm(i, nodeTree->getNodeWtmRel(entry->treeIndex[i]));
          else
            entry->model->setNodeWtm(i, *entry->nodeHelpers[entry->treeIndex[i].index() - 0x8000]);
        continue;
      }

      for (int i = 0; i < entry->nodeHelpers.size(); ++i)
        if (entry->nodeHelpers[i])
          entry->model->setNodeWtm(i, *entry->nodeHelpers[i]);
    }
  }

  if (nodeTree)
  {
    float distSq = lengthSq(cam_pos - nodeTree->getRootWposScalar());
    for (ModelEntry *entry : modelEntries)
      entry->model->chooseLodByDistSq(distSq);
  }
  else
    for (ModelEntry *entry : modelEntries)
    {
      // go through nodes and choose lod by min dist for the whole model
      // as otherwise we can have a situation when some of the nodes of a model
      // are far away from camera, while others are close and we'll choose
      // based on 0'th node, which could be incorrect.
      DynamicRenderableSceneInstance *dynres = entry->model;
      const uint32_t nodeCount = dynres->getNodeCount();
      if (nodeCount > 0)
      {
        float minDistSq = lengthSq(cam_pos - dynres->getNodeWtm(0).getcol(3));
        for (uint32_t i = 1; i < nodeCount; ++i)
          minDistSq = min(minDistSq, lengthSq(cam_pos - dynres->getNodeWtm(i).getcol(3)));
        dynres->chooseLodByDistSq(minDistSq);
      }
      else
        entry->model->beforeRender(cam_pos);
    }
}


template <>
void DynamicPhysObject::render(real opacity /*= 1*/)
{
  if (dynrend::is_initialized())
  {
    for (int n = 0; n < modelEntries.size(); n++)
      if (!dynrend::render_in_tools(modelEntries[n]->model, dynrend::RenderMode::Opaque))
        modelEntries[n]->model->render(opacity);
  }
  else
  {
    for (int n = 0; n < modelEntries.size(); n++)
      modelEntries[n]->model->render(opacity);
  }
}


template <>
void DynamicPhysObject::renderTrans(real opacity /*= 1*/)
{
  if (dynrend::is_initialized())
  {
    for (int n = 0; n < modelEntries.size(); n++)
      if (!dynrend::render_in_tools(modelEntries[n]->model, dynrend::RenderMode::Translucent))
        modelEntries[n]->model->renderTrans(opacity);
  }
  else
  {
    for (int n = 0; n < modelEntries.size(); n++)
      modelEntries[n]->model->renderTrans(opacity);
  }
}

template <>
void DynamicPhysObject::getBodyVisualTm(int index, TMatrix &tm)
{
  G_ASSERT((uint32_t)index < physSys->getBodyCount());

  physSys->getBody(index)->getTm(tm);
}

#endif // NO_3D_GFX
