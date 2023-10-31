#include "materialMgr.h"
#include <3d/dag_materialData.h>
#include <de3_interface.h>
#include <de3_assetService.h>
#include <oldEditor/de_interface.h>
#include <dllPluginCore/core.h>
#include <debug/dag_debug.h>

int ObjLibMaterialListMgr::getMaterialId(const char *mat_asset_name)
{
  int i;

  for (i = 0; i < mat.size(); i++)
    if (mat[i] == mat_asset_name)
      return i;
  i = append_items(mat, 1);
  mat[i] = mat_asset_name;
  return i;
}

void ObjLibMaterialListMgr::setDefaultMat0(MaterialDataList &mat)
{
  G_ASSERT(mat.list.size() > 0);
  // create default material
  Ptr<MaterialData> defSm = new MaterialData;
  defSm->className = "simple";
  defSm->mat.diff = Color4(1, 1, 1);
  defSm->mtex[0] = dagRender->addManagedTexture("../commonData/tex/missing.dds");
  mat.list[0] = defSm;
}

void ObjLibMaterialListMgr::complementDefaultMat0(MaterialDataList &mat)
{
  if (mat.list.size() >= 2 && !mat.list[0].get())
  {
    G_ASSERT(mat.list[1].get());
    mat.list[0] = mat.list[1];
  }
}
void ObjLibMaterialListMgr::reclearDefaultMat0(MaterialDataList &mat)
{
  if (mat.list.size() >= 2 && mat.list[0].get() == mat.list[1].get())
    mat.list[0] = NULL;
}

MaterialDataList *ObjLibMaterialListMgr::buildMaterialDataList()
{
  MaterialDataList *m = new MaterialDataList;

  // reserve entry for default material
  m->addSubMat(NULL);

  Ptr<MaterialData> defSm;

  IAssetService *assetSrv = DAGORED2->queryEditorInterface<IAssetService>();

  for (int i = 0; i < mat.size(); i++)
  {
    Ptr<MaterialData> sm = NULL;

    if (assetSrv)
      sm = assetSrv->getMaterialData(mat[i]);

    if (!sm)
    {
      if (!defSm.get())
      {
        defSm = new MaterialData;
        defSm->className = "simple";
        defSm->mat.diff = Color4(1, 1, 1);
        defSm->mtex[0] = dagRender->addManagedTexture("../commonData/tex/missing.dds");
      }
      sm = defSm;
      DAEDITOR3.conError("can't find material %s", mat[i].str());
    }

    m->addSubMat(sm);
  }

  return m;
}
