// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shadersRes.h>
#include <shaders/dag_shSkinMesh.h>
#include <drv/3d/dag_driver.h>
#include <3d/fileTexFactory.h>
#include <ioSys/dag_genIo.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>

ShaderMeshResource *ShaderMeshResource::loadResource(IGenLoad &crd, ShaderMatVdata &smvd, int res_sz)
{
  if (res_sz == -1)
    res_sz = crd.readInt();
  G_ASSERTF(dumpStartOfs() + res_sz >= sizeof(ShaderMeshResource), "res_sz=%d", res_sz);

  ShaderMeshResource *res = NULL;
  void *mem = memalloc(dumpStartOfs() + res_sz, midmem);

  res = new (mem, _NEW_INPLACE) ShaderMeshResource;
  crd.read(res->dumpStartPtr(), res_sz);
  res->patchData(res_sz, smvd);

  return res;
}

ShaderMeshResource::ShaderMeshResource(const ShaderMeshResource &from)
{
  resSize = from.resSize;
  smvdRef = from.smvdRef;
  memcpy(&mesh, &from.mesh, resSize);
  mesh.rebaseAndClone(&mesh, &from.mesh);
}


ShaderMeshResource *ShaderMeshResource::clone() const
{
  void *mem = memalloc(dumpStartOfs() + resSize, midmem);
  return new (mem, _NEW_INPLACE) ShaderMeshResource(*this);
}

void ShaderMeshResource::patchData(int res_sz, ShaderMatVdata &smvd)
{
  resSize = res_sz;
  smvdRef = &smvd;
  mesh.patchData(&mesh, smvd);
}


void ShaderMeshResource::gatherUsedTex(TextureIdSet &tex_id_list) const { mesh.gatherUsedTex(tex_id_list); }
void ShaderMeshResource::gatherUsedMat(Tab<ShaderMaterial *> &mat_list) const { mesh.gatherUsedMat(mat_list); }

bool ShaderMeshResource::replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new)
{
  return mesh.replaceTexture(tex_id_old, tex_id_new);
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

ShaderSkinnedMeshResource *ShaderSkinnedMeshResource::loadResource(IGenLoad &crd, ShaderMatVdata &smvd, int res_sz)
{
  if (res_sz == -1)
    res_sz = crd.readInt();

  if (res_sz & 0x40000000) // tight format
    res_sz &= ~0x40000000;
  else
  {
    char buf[4 << 10];
    logerr("obsolete ShaderSkinnedMeshResource format, please rebuild!%s", dgs_get_fatal_context(buf, sizeof(buf)));
    crd.seekrel(res_sz);
    return nullptr;
  }

  ShaderSkinnedMeshResource *res = NULL;
  void *mem = memalloc(dumpStartOfs() + res_sz, midmem);

  res = new (mem, _NEW_INPLACE) ShaderSkinnedMeshResource;
  crd.read(res->dumpStartPtr(), res_sz);
  res->patchData(res_sz, smvd);

  return res;
}

ShaderSkinnedMeshResource::ShaderSkinnedMeshResource(const ShaderSkinnedMeshResource &from)
{
  resSize = from.resSize;
  smvdRef = from.smvdRef;
  memcpy(&mesh, &from.mesh, resSize);
  mesh.rebaseAndClone(&mesh, &from.mesh);
}


ShaderSkinnedMeshResource *ShaderSkinnedMeshResource::clone() const
{
  void *mem = memalloc(dumpStartOfs() + resSize, midmem);
  return new (mem, _NEW_INPLACE) ShaderSkinnedMeshResource(*this);
}

void ShaderSkinnedMeshResource::patchData(int res_sz, ShaderMatVdata &smvd)
{
  resSize = res_sz;
  smvdRef = &smvd;
  mesh.patchData(&mesh, smvd);
}

void ShaderSkinnedMeshResource::gatherUsedTex(TextureIdSet &tex_id_list) const { mesh.getShaderMesh().gatherUsedTex(tex_id_list); }
void ShaderSkinnedMeshResource::gatherUsedMat(Tab<ShaderMaterial *> &mat_list) const { mesh.getShaderMesh().gatherUsedMat(mat_list); }
bool ShaderSkinnedMeshResource::replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new)
{
  return mesh.getShaderMesh().replaceTexture(tex_id_old, tex_id_new);
}
