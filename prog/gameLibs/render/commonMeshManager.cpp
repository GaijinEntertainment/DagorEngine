// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/commonMeshManager.h>
#include <shaders/dag_dynSceneRes.h>
#include <memory/dag_mem.h>


ManagedMesh::ManagedMesh(DynamicRenderableSceneInstance *m, int h) : mesh(m), handle(h) {}
ManagedMesh::ManagedMesh() : mesh(NULL), handle(-1) {}

bool ManagedMesh::isValid() const { return handle >= 0; }

void ManagedMesh::clear()
{
  handle = -1;
  del_it(mesh);
}

void CommonMeshManager::ManagedMeshRecord::clear()
{
  if (meshName)
    memfree((void *)meshName, strmem);
  meshName = NULL;
  if (res)
    res->delRef();
  res = NULL;
}

CommonMeshManager::CommonMeshManager() : managedMeshList(midmem) {}

void CommonMeshManager::clear()
{
  WinAutoLock lock_(critSec);
  debug("clear managed meshes");
  clear_and_shrink(managedMeshList);
}

void CommonMeshManager::dereferenceMesh(ManagedMesh &m)
{
  WinAutoLock lock_(critSec);
  if (m.handle >= 0)
  {
    ManagedMeshRecord *mmr = safe_at(managedMeshList, m.handle);
    if (!mmr || --mmr->refCount <= 0)
    {
      if (mmr)
        mmr->clear();
      m.clear();
    }
  }
}


void CommonMeshManager::setMesh(ManagedMesh &m)
{
  if (m.isValid() && !m.mesh)
  {
    WinAutoLock lock_(critSec);
    DynamicRenderableSceneLodsResource *res = managedMeshList[m.handle].res;
    if (res)
      m.mesh = new DynamicRenderableSceneInstance(res);
  }
}

ManagedMesh CommonMeshManager::registerOrReferenceMesh(const char *mesh_name, DynamicRenderableSceneLodsResource *resource)
{
  if (!mesh_name || !*mesh_name)
    return ManagedMesh();
  WinAutoLock lock_(critSec);
  int i;
  for (i = 0; i < managedMeshList.size(); ++i)
    if (managedMeshList[i].meshName && strcmp(managedMeshList[i].meshName, mesh_name) == 0)
      break;
  if (i < managedMeshList.size())
  {
    ManagedMeshRecord &rec = managedMeshList[i];
    if (!rec.res && resource)
    {
      rec.res = resource;
      rec.res->addRef();
    }
    rec.refCount++;
    ManagedMesh m(NULL, i);
    setMesh(m);
    return m;
  }
  else
  {
    ManagedMeshRecord *r = NULL;
    for (int j = 0; j < managedMeshList.size() && !r; ++j)
      if (!managedMeshList[j].meshName)
        r = &managedMeshList[j];
    if (!r)
      r = &managedMeshList.push_back();
    G_ASSERT(!r->res);
    r->meshName = str_dup(mesh_name, strmem);
    r->res = resource;
    if (resource)
      resource->addRef();
    r->refCount = 1;
    ManagedMesh m(NULL, r - managedMeshList.data());
    setMesh(m);
    return m;
  }
}
