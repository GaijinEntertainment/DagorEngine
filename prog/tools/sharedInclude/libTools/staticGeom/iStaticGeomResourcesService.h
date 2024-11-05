// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
