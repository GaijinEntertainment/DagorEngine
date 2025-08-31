//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <osApiWrappers/dag_critSec.h>

class DynamicRenderableSceneInstance;
class DynamicRenderableSceneLodsResource;

struct ManagedMesh
{
  DynamicRenderableSceneInstance *mesh; // this could be NULL, but ManagedMesh is still valid
  int handle;

  explicit ManagedMesh(DynamicRenderableSceneInstance *m, int h);
  ManagedMesh();
  bool isValid() const;
  void clear();
};

class CommonMeshManager
{
public:
  CommonMeshManager();

  void clear();
  void setMesh(ManagedMesh &m);
  ManagedMesh registerOrReferenceMesh(const char *mesh_name, DynamicRenderableSceneLodsResource *resource);
  void addReferenceToMesh(const ManagedMesh &m);
  void dereferenceMesh(ManagedMesh &m);
  void debugAllMeshes();

private:
  friend struct ManagedMeshRef;
  int registerOrReferenceMeshImpl(const char *mesh_name, DynamicRenderableSceneLodsResource **in_out_resource);

  struct ManagedMeshRecord
  {
    ManagedMeshRecord() : meshName(NULL), res(NULL), refCount(0) {}
    ManagedMeshRecord(ManagedMeshRecord &&r) : meshName(r.meshName), res(r.res), refCount(r.refCount)
    {
      r.meshName = nullptr;
      r.res = nullptr;
      r.refCount = 0;
    }
    ~ManagedMeshRecord() { clear(); }
    void clear();

  public:
    const char *meshName;
    DynamicRenderableSceneLodsResource *res;
    int refCount;
  };

  WinCritSec critSec;
  Tab<ManagedMeshRecord> managedMeshList;
};
DAG_DECLARE_RELOCATABLE(CommonMeshManager::ManagedMeshRecord);

inline DynamicRenderableSceneInstance *get_mesh_managed(ManagedMesh &mesh, CommonMeshManager &mesh_manager)
{
  mesh_manager.setMesh(mesh);
  return mesh.mesh;
}

struct ManagedMeshRef
{
  ManagedMeshRef() = default;
  explicit ManagedMeshRef(CommonMeshManager &mgr) : mgr(&mgr) { mMesh.clear(); }
  explicit ManagedMeshRef(CommonMeshManager &mgr, const char *name, DynamicRenderableSceneLodsResource *res) : ManagedMeshRef(mgr)
  {
    initMesh(name, res);
  }
  ManagedMeshRef(const ManagedMeshRef &) = delete;
  void operator=(const ManagedMeshRef &) = delete;
  ManagedMeshRef(ManagedMeshRef &&rhs) : mgr(rhs.mgr), resPtr(rhs.resPtr), mMesh(rhs.mMesh)
  {
    rhs.resPtr = nullptr;
    rhs.mMesh.clear();
  }
  ManagedMeshRef &operator=(ManagedMeshRef &&rhs);
  ~ManagedMeshRef() { clear(); }
  ManagedMeshRef copyRef() const;
  void initMesh(const char *mesh_name, DynamicRenderableSceneLodsResource *res = nullptr);
  void initMesh(CommonMeshManager &mgr, const char *mesh_name, DynamicRenderableSceneLodsResource *res = nullptr);

  bool isValid() const { return mMesh.isValid(); }
  void clear();
  DynamicRenderableSceneLodsResource *getMeshRes() const { return resPtr; }
  DynamicRenderableSceneInstance *newInstanceRawPtr() const;
  eastl::unique_ptr<DynamicRenderableSceneInstance, eastl::default_delete<DynamicRenderableSceneInstance>> newInstance() const;

private:
  CommonMeshManager *mgr = nullptr;
  DynamicRenderableSceneLodsResource *resPtr = nullptr;
  ManagedMesh mMesh;
};
