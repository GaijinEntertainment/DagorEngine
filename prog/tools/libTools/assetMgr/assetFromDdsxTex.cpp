#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMsgPipe.h>
#include "assetCreate.h"
#include <3d/dag_texMgr.h>
#include <util/dag_fastIntList.h>
#include <util/dag_texMetaData.h>
#include <debug/dag_debug.h>

bool DagorAssetMgr::mountBuiltDdsxTex(const char *mount_folder_name)
{
  if (!folders.size())
  {
    folders.push_back(new DagorAssetFolder(-1, "Root", NULL));
    perFolderStartAssetIdx.push_back(0);
  }

  int s_asset_num = assets.size();
  int nsid = nspaceNames.addNameId("gameres");
  int fidx = folders.size();
  folders.push_back(new DagorAssetFolder(0, mount_folder_name, "::"));
  folders[fidx]->flags &= ~(DagorAssetFolder::FLG_EXPORT_ASSETS | DagorAssetFolder::FLG_SCAN_ASSETS |
                            DagorAssetFolder::FLG_SCAN_FOLDERS | DagorAssetFolder::FLG_INHERIT_RULES);

  folders[0]->subFolderIdx.push_back(fidx);
  perFolderStartAssetIdx.push_back(assets.size());

  post_msg(*msgPipe, msgPipe->NOTE, "mounting built ddsx packs to %s...", mount_folder_name);

  if (texAssetType != -1)
  {
    DagorAssetPrivate *ca = NULL;
    for (TEXTUREID id = first_managed_texture(0); id != BAD_TEXTUREID; id = next_managed_texture(id, 0))
    {
      const char *tex_nm = get_managed_texture_name(id);
      TextureMetaData tmd;
      SimpleString nm(tmd.decode(tex_nm));
      if (nm.empty() || nm[strlen(nm) - 1] != '*')
        continue;

      nm[strlen(nm) - 1] = '\0';
      if (!ca)
        ca = new DagorAssetPrivate(*this);

      ca->setNames(assetNames.addNameId(nm), nsid, true);
      if (perTypeNameIds[texAssetType].addInt(ca->getNameId()))
      {
        DataBlock &blk = ca->props;
        tmd.write(ca->props);

        ca->setAssetData(fidx, -1, texAssetType);
        assets.push_back(ca);
        ca = NULL;
      }
      else
        post_msg(*msgPipe, msgPipe->WARNING, "duplicate asset %s of type <%s> in namespace %s", ca->getName(),
          typeNames.getName(texAssetType), nspaceNames.getName(nsid));
    }
    del_it(ca);
  }

  updateGlobUniqueFlags();

  post_msg(*msgPipe, msgPipe->NOTE, "added %d texture assets", assets.size() - s_asset_num);
  return true;
}
