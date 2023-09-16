#ifndef __GAIJIN_GEOM_RESOURCES__
#define __GAIJIN_GEOM_RESOURCES__
#pragma once


#include <libTools/staticGeom/iStaticGeomResourcesService.h>

#include <generic/dag_tab.h>

#include <libTools/containers/dag_DynMap.h>

#include <math/dag_TMatrix.h>


class StaticGeometryContainer;
class ILogWriter;


class GeomResourcesHelper
{
public:
  GeomResourcesHelper(IStaticGeomResourcesService *svc) : objResources(midmem), resService(svc) {}
  ~GeomResourcesHelper();

  inline void setResourcesService(IStaticGeomResourcesService *svc) { resService = svc; }

  void createResources(const void *obj_id, const TMatrix &tm, const StaticGeometryContainer &cont, ILogWriter *log);
  void remapResources(const void *obj_id_old, const void *obj_id_new);
  void freeResources(const void *obj_id);
  void freeAllResources();
  void setResourcesTm(const void *obj_id, const TMatrix &tm);

  // slow method to free all unused memory. returns freed memory size
  int compact();

private:
  struct ResourceRec
  {
    TMatrix tm;
    IStaticGeomResourcesService::Resource res;
  };


  class ResTab : public Tab<ResourceRec>
  {
  public:
    ResTab() : Tab<ResourceRec>(tmpmem) {}
  };

  IStaticGeomResourcesService *resService;
  DynMap<const void *, ResTab *> objResources;

  void deleteResource(ResTab *res_tab);
};


#endif //__GAIJIN_GEOM_RESOURCES__
