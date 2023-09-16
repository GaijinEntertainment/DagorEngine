#include <libTools/staticGeom/geomResources.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/staticGeometryContainer.h>

#include <libTools/util/iLogWriter.h>

#include <util/dag_globDef.h>

#include <debug/dag_debug.h>


//==================================================================================================
GeomResourcesHelper::~GeomResourcesHelper() { freeAllResources(); }


//==================================================================================================
void GeomResourcesHelper::freeAllResources()
{
  ResTab *resTab = NULL;

  for (bool ok = objResources.getFirst(resTab); ok; ok = objResources.getNext(resTab))
    if (resTab)
      deleteResource(resTab);

  objResources.eraseall();
}


//==================================================================================================
void GeomResourcesHelper::createResources(const void *obj_id, const TMatrix &tm, const StaticGeometryContainer &cont, ILogWriter *log)
{
  if (!resService)
    return;

  ResTab *resTab = NULL;
  ResourceRec resRec;
  bool doSet = false;

  if (objResources.get(obj_id, resTab))
  {
    if (resTab)
      fatal("Object's resources already created");
    else
      doSet = true;
  }

  for (int ni = 0; ni < cont.nodes.size(); ++ni)
  {
    const StaticGeometryNode *node = cont.nodes[ni];
    if (node && !node->linkedResource.empty())
    {
      // this invNodeTm is need to convert node matrix from Max to Dagor coordinates
      TMatrix invNodeTm;
      invNodeTm.setcol(0, node->wtm.getcol(0));
      invNodeTm.setcol(1, node->wtm.getcol(2));
      invNodeTm.setcol(2, node->wtm.getcol(1));
      invNodeTm.setcol(3, node->wtm.getcol(3));

      IStaticGeomResourcesService::Resource res = resService->createResource(node->linkedResource, tm * invNodeTm);

      if (res)
      {
        if (!resTab)
        {
          resTab = new (tmpmem) ResTab;
          G_ASSERT(resTab);
          G_VERIFY(doSet ? objResources.set(obj_id, resTab) : objResources.add(obj_id, resTab));
        }

        resRec.res = res;
        resRec.tm = invNodeTm;

        resTab->push_back(resRec);
      }
      else if (log)
      {
        log->addMessage(ILogWriter::WARNING, "Couldn't create resource \"%s\" linked to node \"%s\"", node->linkedResource,
          node->name);
      }
    }
  }
}


//==================================================================================================
void GeomResourcesHelper::remapResources(const void *obj_id_old, const void *obj_id_new)
{
  ResTab *resTab;
  bool doSet = false;

  if (objResources.get(obj_id_new, resTab))
  {
    if (resTab)
      fatal("Object's resources already created");
    else
      doSet = true;
  }

  if (objResources.get(obj_id_old, resTab) && resTab)
  {
    objResources.set(obj_id_old, NULL);
    doSet ? objResources.set(obj_id_new, resTab) : objResources.add(obj_id_new, resTab);
  }
}


//==================================================================================================
void GeomResourcesHelper::freeResources(const void *obj_id)
{
  ResTab *resTab;
  if (objResources.get(obj_id, resTab) && resTab)
  {
    deleteResource(resTab);
    objResources.set(obj_id, NULL);
  }
}


//==================================================================================================
void GeomResourcesHelper::deleteResource(ResTab *res_tab)
{
  if (resService)
    for (auto &resRec : *res_tab)
      resService->deleteResource(resRec.res);

  del_it(res_tab);
}


//==================================================================================================
void GeomResourcesHelper::setResourcesTm(const void *obj_id, const TMatrix &tm)
{
  ResTab *resTab;
  if (objResources.get(obj_id, resTab) && resTab && resService)
  {
    for (auto &resRec : *resTab)
      resService->setResourceTm(resRec.res, tm * resRec.tm);
  }
}


//==================================================================================================
int GeomResourcesHelper::compact()
{
  const int oldSize = objResources.dataSize();
  Tab<const void *> delKeys(tmpmem);
  int incStep = objResources.size() / 8;
  if (incStep < 8)
    incStep = 8;

  const void *key;
  ResTab *resTab;

  for (bool ok = objResources.getFirst(key, resTab); ok; ok = objResources.getNext(key, resTab))
    if (!resTab)
      delKeys.push_back(key);

  objResources.erase(delKeys);

  return oldSize - objResources.dataSize();
}
