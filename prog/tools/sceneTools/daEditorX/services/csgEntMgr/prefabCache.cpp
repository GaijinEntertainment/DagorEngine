// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "prefabCache.h"
#include <de3_interface.h>
#include <libTools/dagFileRW/dagFileNode.h>

Tab<PrefabGeomCacheRec *> PrefabGeomCacheRec::prefab_geom_cache;
static PtrTab<MaterialData> commonMat;

void PrefabGeomCacheRec::clear()
{
  clear_all_ptr_items(prefab_geom_cache);
  clear_and_shrink(commonMat);
}
void PrefabGeomCacheRec::evictCache(int asset_name_id)
{
  for (int i = 0; i < prefab_geom_cache.size(); i++)
    if (prefab_geom_cache[i]->assetNameId == asset_name_id)
    {
      erase_ptr_items(prefab_geom_cache, i, 1);
      return;
    }
}
PrefabGeomCacheRec *PrefabGeomCacheRec::getCache(int asset_name_id)
{
  for (int i = 0; i < prefab_geom_cache.size(); i++)
    if (prefab_geom_cache[i]->assetNameId == asset_name_id)
      return prefab_geom_cache[i];
  return NULL;
}
PrefabGeomCacheRec *PrefabGeomCacheRec::fetchCache(DagorAsset *a, bool recheck)
{
  if (recheck)
    if (PrefabGeomCacheRec *r = a ? getCache(a->getNameId()) : NULL)
      return r;
  PrefabGeomCacheRec *r = new PrefabGeomCacheRec(a->getNameId());
  r->loadDag(*a);
  prefab_geom_cache.push_back(r);
  return r;
}

PrefabGeomCacheRec::PrefabGeomCacheRec(int nid) { assetNameId = nid; }
PrefabGeomCacheRec::~PrefabGeomCacheRec() { clear_all_ptr_items(mesh); }

void PrefabGeomCacheRec::addMaterials(MaterialDataList &out_mdl, Tab<int> &mat_remap)
{
  if (!mdl.list.size())
  {
    mat_remap.resize(1);
    mat_remap[0] = 0;
    return;
  }

  mat_remap.resize(mdl.list.size());
  for (int i = 0; i < mdl.list.size(); i++)
  {
    int found_idx = -1;
    for (int j = 0; j < out_mdl.list.size(); j++)
      if (mdl.list[i] == out_mdl.list[j])
      {
        mat_remap[i] = found_idx = j;
        break;
      }
    if (found_idx < 0)
    {
      mat_remap[i] = out_mdl.list.size();
      out_mdl.list.push_back(mdl.list[i]);
    }
  }
}

void PrefabGeomCacheRec::loadDag(const DagorAsset &asset)
{
  AScene sc;
  if (!::load_ascene(asset.getTargetFilePath(), sc, LASF_MATNAMES | LASF_NULLMATS, false))
  {
    DAEDITOR3.conError("Couldn't load ascene from \"%s\" file", asset.getTargetFilePath());
    return;
  }
  sc.root->calc_wtm();
  loadDagNode(sc.root);
}

int PrefabGeomCacheRec::addMat(MaterialData *md)
{
  for (int i = 0; i < mdl.list.size(); i++)
    if (md == mdl.list[i] || md->isEqual(*mdl.list[i]))
      return i;

  int m_idx = mdl.list.size();
  for (int i = 0; i < commonMat.size(); i++)
    if (md->isEqual(*commonMat[i]))
    {
      mdl.list.push_back(commonMat[i]);
      return m_idx;
    }

  commonMat.push_back(md);
  mdl.list.push_back(md);
  return m_idx;
}
void PrefabGeomCacheRec::loadDagNode(Node *node)
{
  if (node->obj && node->obj->isSubOf(OCID_MESHHOLDER))
    if (Mesh *m = ((MeshHolderObj *)node->obj)->mesh)
    {
      m->transform(node->wtm);
      Tab<int> mat_remap;
      mat_remap.resize(node->mat->list.size());
      mem_set_ff(mat_remap);

      for (int i = 0; i < m->face.size(); i++)
      {
        int mat = m->face[i].mat;
        if (mat < 0 || mat >= mat_remap.size())
          m->face[i].mat = 0;
        else if (mat_remap[mat] >= 0)
          m->face[i].mat = mat_remap[mat];
        else
        {
          mat_remap[mat] = addMat(node->mat->list[mat]);
          m->face[i].mat = mat_remap[mat];
        }
      }

      mesh.push_back(m);
      ((MeshHolderObj *)node->obj)->mesh = NULL;
    }

  for (int i = 0; i < node->child.size(); ++i)
    if (node->child[i])
      loadDagNode(node->child[i]);
}
