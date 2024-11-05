// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_instShaderMeshRes.h>
#include <shaders/dag_shadersRes.h>
#include <shaders/dag_shaderMesh.h>
#include <drv/3d/dag_driver.h>
#include <ioSys/dag_genIo.h>
#include <math/dag_TMatrix.h>


InstShaderMeshResource *InstShaderMeshResource::loadResource(IGenLoad &crd, ShaderMatVdata &smvd, int res_sz)
{
  if (res_sz == -1)
    res_sz = crd.readInt();

  InstShaderMeshResource *res = NULL;
  void *mem = memalloc(dumpStartOfs() + res_sz, midmem);

  res = new (mem, _NEW_INPLACE) InstShaderMeshResource;
  crd.read(res->dumpStartPtr(), res_sz);
  res->resSize = res_sz;
  res->mesh.patchData(&res->mesh, smvd);

  return res;
}

InstShaderMeshResource::InstShaderMeshResource(const InstShaderMeshResource &from)
{
  resSize = from.resSize;
  smvdRef = from.smvdRef;
  memcpy(&mesh, &from.mesh, resSize);
  mesh.rebaseAndClone(&mesh, &from.mesh);
}


InstShaderMeshResource *InstShaderMeshResource::clone() const
{
  void *mem = memalloc(dumpStartOfs() + resSize, midmem);
  return new (mem, _NEW_INPLACE) InstShaderMeshResource(*this);
}
