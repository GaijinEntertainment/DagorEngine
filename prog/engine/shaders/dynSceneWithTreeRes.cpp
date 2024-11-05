// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_dynSceneWithTreeRes.h>
#include <ioSys/dag_genIo.h>
#include <generic/dag_initOnDemand.h>

// DynSceneResNameMapResource
DynamicSceneWithTreeResource *DynamicSceneWithTreeResource::loadResource(IGenLoad &crd, int flags)
{
  DynamicSceneWithTreeResource *res = new DynamicSceneWithTreeResource;

  res->nodeTree.load(crd);
  res->scene = DynamicRenderableSceneLodsResource::loadResource(crd, flags);

  return res;
}
