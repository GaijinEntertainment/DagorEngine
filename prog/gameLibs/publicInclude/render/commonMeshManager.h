//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
  void dereferenceMesh(ManagedMesh &m);

private:
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
