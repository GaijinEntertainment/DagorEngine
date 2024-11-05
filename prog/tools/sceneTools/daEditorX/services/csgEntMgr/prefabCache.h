// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/asset.h>
#include <3d/dag_materialData.h>
#include <math/dag_mesh.h>
#include <generic/dag_tab.h>

class Node;


struct PrefabGeomCacheRec
{
  int assetNameId;
  Tab<Mesh *> mesh;
  MaterialDataList mdl;

  PrefabGeomCacheRec(int nid);
  ~PrefabGeomCacheRec();

  void addMaterials(MaterialDataList &mdl, Tab<int> &mat_remap);

public:
  static void clear();
  static void evictCache(int asset_name_id);
  static PrefabGeomCacheRec *getCache(int asset_name_id);
  static PrefabGeomCacheRec *fetchCache(DagorAsset *a, bool recheck_cache_ready = false);

protected:
  static Tab<PrefabGeomCacheRec *> prefab_geom_cache;

  void loadDag(const DagorAsset &asset);
  void loadDagNode(Node *node);
  int addMat(MaterialData *md);
};
