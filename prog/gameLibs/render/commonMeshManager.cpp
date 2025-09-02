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
#if DAGOR_DBGLEVEL > 0
  debugAllMeshes();
#endif
  clear_and_shrink(managedMeshList);
}

void CommonMeshManager::debugAllMeshes()
{
  WinAutoLock lock_(critSec);
  for (int i = 0; i < managedMeshList.size(); i++)
    if (managedMeshList[i].res || managedMeshList[i].refCount)
      debug("CommonMeshManager:  %i %s refs:%i res:%p", i, managedMeshList[i].meshName, managedMeshList[i].refCount,
        managedMeshList[i].res);
}

void CommonMeshManager::addReferenceToMesh(const ManagedMesh &m)
{
  WinAutoLock lock_(critSec);
  if (m.handle >= 0)
  {
    ManagedMeshRecord *mmr = safe_at(managedMeshList, m.handle);
    G_ASSERT_RETURN(mmr, );
    mmr->refCount++;
  }
}

void CommonMeshManager::dereferenceMesh(ManagedMesh &m)
{
  WinAutoLock lock_(critSec);
  if (m.handle >= 0)
  {
    ManagedMeshRecord *mmr = safe_at(managedMeshList, m.handle);
    if (!mmr || --mmr->refCount <= 0)
    {
      G_ASSERT(mmr && mmr->refCount == 0);
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
  DynamicRenderableSceneLodsResource *outRes = resource;
  const int i = registerOrReferenceMeshImpl(mesh_name, &outRes);
  if (i == -1)
    return ManagedMesh();
  ManagedMesh m(nullptr, i);
  setMesh(m);
  return m;
}

int CommonMeshManager::registerOrReferenceMeshImpl(const char *mesh_name, DynamicRenderableSceneLodsResource **in_out_resource)
{
  if (!mesh_name || !*mesh_name)
  {
    *in_out_resource = nullptr;
    return -1;
  }
  WinAutoLock lock_(critSec);
  int i;
  for (i = 0; i < managedMeshList.size(); ++i)
    if (managedMeshList[i].meshName && strcmp(managedMeshList[i].meshName, mesh_name) == 0)
      break;
  if (i < managedMeshList.size())
  {
    ManagedMeshRecord &rec = managedMeshList[i];
    if (!rec.res && *in_out_resource)
    {
      rec.res = *in_out_resource;
      rec.res->addRef();
    }
    rec.refCount++;
    *in_out_resource = rec.res;
    return i;
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
    r->res = *in_out_resource;
    if (r->res)
      r->res->addRef();
    r->refCount = 1;
    return r - managedMeshList.data();
  }
}

ManagedMeshRef &ManagedMeshRef::operator=(ManagedMeshRef &&rhs)
{
  if (&rhs == this)
    return *this;
  ManagedMesh oldMesh = mMesh;
  mgr = rhs.mgr;
  mMesh = rhs.mMesh;
  resPtr = rhs.resPtr;
  rhs.resPtr = nullptr;
  rhs.mMesh.clear();
  if (oldMesh.isValid())
    mgr->dereferenceMesh(oldMesh); // clear mesh after copying to avoid delRef on resource, in case its the same one
  return *this;
}

ManagedMeshRef ManagedMeshRef::copyRef() const
{
  G_ASSERT_RETURN(mgr, {});
  ManagedMeshRef ref(*mgr);
  mgr->addReferenceToMesh(mMesh);
  ref.mMesh = mMesh;
  ref.resPtr = resPtr;
  return ref;
}

void ManagedMeshRef::clear()
{
  if (mMesh.isValid())
  {
    G_ASSERT_RETURN(mgr, );
    mgr->dereferenceMesh(mMesh);
  }
  mMesh.clear();
  resPtr = nullptr;
}

void ManagedMeshRef::initMesh(CommonMeshManager &new_mgr, const char *mesh_name, DynamicRenderableSceneLodsResource *res)
{
  if (mgr != &new_mgr)
  {
    clear();
    mgr = &new_mgr;
  }
  initMesh(mesh_name, res);
}

void ManagedMeshRef::initMesh(const char *mesh_name, DynamicRenderableSceneLodsResource *res)
{
  G_ASSERT_RETURN(mgr, );
  if (isValid() && res && resPtr == res)
    return;
  ManagedMesh oldMesh = mMesh;
  mMesh.clear();
  resPtr = res;
  mMesh.handle = mgr->registerOrReferenceMeshImpl(mesh_name, &resPtr);
  if (oldMesh.isValid())
    mgr->dereferenceMesh(oldMesh);
}

DynamicRenderableSceneInstance *ManagedMeshRef::newInstanceRawPtr() const
{
  return resPtr ? new DynamicRenderableSceneInstance(resPtr) : nullptr;
}

eastl::unique_ptr<DynamicRenderableSceneInstance> ManagedMeshRef::newInstance() const
{
  return eastl::unique_ptr<DynamicRenderableSceneInstance>(newInstanceRawPtr());
}
