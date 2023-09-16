#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetFolder.h>
#include <ioSys/dag_memIo.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_string.h>
#include <libTools/util/makeBindump.h>

const char *DagorAsset::getName() const { return mgr.getAssetName(nameId); }
const char *DagorAsset::getNameSpace() const { return mgr.getAssetNameSpace(nspaceId); }
const char *DagorAsset::getSrcFileName() const { return mgr.getAssetFile(fileNameId); }
const char *DagorAsset::getTypeStr() const { return mgr.getAssetTypeName(assetType); }
const char *DagorAsset::getFolderPath() const { return mgr.getFolder(folderIdx).folderPath; }

String DagorAsset::getSrcFilePath() const
{
  const char *folder_path = getFolderPath();
  if (folder_path && *folder_path)
    return String(0, "%s/%s", getFolderPath(), getSrcFileName());
  return String(getSrcFileName());
}

String DagorAsset::getTargetFilePath() const
{
  const char *folder_path = getFolderPath();
  if (folder_path && *folder_path)
    return String(0, "%s/%s", getFolderPath(), props.getStr("name", getSrcFileName()));
  return String(props.getStr("name", getSrcFileName()));
}

String DagorAsset::getNameTypified() const { return String(0, "%s:%s", getName(), getTypeStr()); }

const DataBlock &DagorAsset::getProfileTargetProps(unsigned target, const char *profile) const
{
  const char *tstr = mkbindump::get_target_str(target);
  const DataBlock *b = (profile && *profile) ? props.getBlockByName(String(40, "%s~%s", tstr, profile)) : NULL;
  if (b)
    return *b;
  b = props.getBlockByName(tstr);
  return b ? *b : props;
}
const char *DagorAsset::resolveEffProfileTargetStr(const char *target_str, const char *profile) const
{
  const DataBlock *b = (profile && *profile) ? props.getBlockByName(String(40, "%s~%s", target_str, profile)) : NULL;
  return b ? b->getBlockName() : target_str;
}

String DagorAsset::fpath2asset(const char *fpath)
{
  if (!fpath || !*fpath)
    return {};
  String tempBuffer(dd_get_fname(fpath));
  if (tempBuffer.length() > 4 && tempBuffer[tempBuffer.length() - 4] == '.')
    erase_items(tempBuffer, tempBuffer.length() - 4, 4);
  else if (tempBuffer.length() > 5 && tempBuffer[tempBuffer.length() - 5] == '.')
    erase_items(tempBuffer, tempBuffer.length() - 5, 5);
  else
  {
    const char *ext = dd_get_fname_ext(tempBuffer);
    const char *meta = ext ? strchr(ext + 1, '?') : NULL;
    if (meta && (meta == ext + 4 || meta == ext + 5)) //-V1004
      *(char *)ext = '\0';                            //-V1004
  }
  tempBuffer.resize(strlen(tempBuffer) + 1);
  if (tempBuffer.size() > 2 && tempBuffer[tempBuffer.size() - 2] == '*')
    erase_items(tempBuffer, tempBuffer.size() - 2, 1);
  return eastl::move(tempBuffer);
}
