#include "materialRec.h"
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <math/dag_Point4.h>

//==============================================================================
MaterialRec::MaterialRec(DagorAsset &_asset) : asset(&_asset), textures(midmem)
{
  className = asset->props.getStr("class_name", "simple");
  script = asset->props.getStr("script", "");
  flags = asset->props.getInt("flags", 0);
  defMatCol = 0;

  mat.power = 0;
  mat.diff.set_xyzw(asset->props.getPoint4("diff", Point4(1, 1, 1, 1)));
  mat.amb.set_xyzw(asset->props.getPoint4("amb", Point4(1, 1, 1, 1)));
  mat.spec.set_xyzw(asset->props.getPoint4("spec", Point4(1, 1, 1, 1)));
  mat.emis.set_xyzw(asset->props.getPoint4("emis", Point4(0, 0, 0, 0)));
  if (!asset->props.paramExists("diff"))
    defMatCol |= 1 << 0;
  if (!asset->props.paramExists("amb"))
    defMatCol |= 1 << 1;
  if (!asset->props.paramExists("spec"))
    defMatCol |= 1 << 2;
  if (!asset->props.paramExists("emis"))
    defMatCol |= 1 << 3;

  DagorAssetMgr &mgr = asset->getMgr();
  const DataBlock *blk;

  blk = asset->props.getBlockByNameEx("textures");

  String texBuf;
  for (int i = 0; i < MAXMATTEXNUM; i++)
  {
    texBuf.printf(16, "tex%d", i);
    const char *texName = blk->getStr(texBuf, NULL);
    if (!texName)
      continue;

    while (textures.size() < i)
      textures.push_back(NULL);

    textures.push_back(mgr.findAsset(texName, mgr.getTexAssetTypeId()));
  }
}


//==============================================================================
void MaterialRec::saveChangesToAsset()
{
  if (className.empty())
    className = "simple";

  asset->props.setStr("class_name", className);
  asset->props.setStr("script", script);
  asset->props.setInt("flags", flags);

  if (defMatCol & (1 << 0))
    asset->props.removeParam("diff");
  else
    asset->props.setPoint4("diff", Point4::rgba(mat.diff));

  if (defMatCol & (1 << 1))
    asset->props.removeParam("amb");
  else
    asset->props.setPoint4("amb", Point4::rgba(mat.amb));

  if (defMatCol & (1 << 2))
    asset->props.removeParam("spec");
  else
    asset->props.setPoint4("spec", Point4::rgba(mat.spec));

  if (defMatCol & (1 << 3))
    asset->props.removeParam("emis");
  else
    asset->props.setPoint4("emis", Point4::rgba(mat.emis));


  for (int i = textures.size() - 1; i >= 0; i--)
    if (textures[i] == NULL)
      textures.pop_back();
    else
      break;


  DataBlock *blk = asset->props.getBlockByName("textures");
  if (blk)
    blk->clearData();
  else
    blk = asset->props.addNewBlock("textures");

  while (asset->props.removeBlock("params")) {}

  String texBuf;
  for (int i = 0; i < textures.size(); ++i)
    if (textures[i])
    {
      texBuf.printf(16, "tex%d", i);
      blk->addStr(texBuf, textures[i]->getName());
    }
}


//==============================================================================
bool MaterialRec::operator==(const MaterialRec &from) const
{
  if (className != from.className)
    return false;

  if (stricmp(script, from.script) != 0)
    return false;

  if (textures.size() != from.textures.size())
    return false;

  for (int i = 0; i < textures.size(); ++i)
    if (textures[i] != from.textures[i])
      return false;

  return true;
}


//==============================================================================
Ptr<MaterialData> MaterialRec::getMaterial() const
{
  Ptr<MaterialData> ret = new (tmpmem) MaterialData;

  ret->className = className;
  ret->matScript = script;
  ret->matName = asset->getName();
  ret->flags = flags;
  ret->mat = mat;

  memset(ret->mtex, 0xFF, sizeof(ret->mtex));

  for (int ti = 0; ti < MAXMATTEXNUM; ++ti)
  {
    bool badTex = false;

    if (ti < textures.size() && textures[ti])
      ret->mtex[ti] = ::add_managed_texture(String(64, "%s*", textures[ti]->getName()));

    if (badTex)
      ret->mtex[ti] = BAD_TEXTUREID;
  }

  return ret;
}

const char *MaterialRec::getTexRef(int idx) const
{
  if (idx >= 0 && idx < textures.size() && textures[idx])
    return textures[idx]->getName();
  return "<none>";
}
