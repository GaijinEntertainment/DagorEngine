#ifndef __GAIJIN_ISTATIC_GEOM_RESOURCES_SERVICE__
#define __GAIJIN_ISTATIC_GEOM_RESOURCES_SERVICE__
#pragma once


class TMatrix;


class IStaticGeomResourcesService
{
public:
  typedef const void *Resource;

  virtual Resource createResource(const char *name, const TMatrix &tm) = 0;
  virtual void deleteResource(Resource res) = 0;
  virtual void setResourceTm(Resource res, const TMatrix &tm) = 0;
};


#endif //__GAIJIN_ISTATIC_GEOM_RESOURCES_SERVICE__
