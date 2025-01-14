// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetUtils.h>
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <osApiWrappers/dag_shellExecute.h>
#include <osApiWrappers/dag_unicode.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <shlobj.h>
#endif

void dag_reveal_in_explorer(const DagorAsset *asset)
{
  if (!asset)
    return;

  String fpath(asset->isVirtual() ? asset->getTargetFilePath() : asset->getSrcFilePath());
  dag_reveal_in_explorer(fpath);
}

void dag_reveal_in_explorer(const DagorAssetFolder *folder)
{
  if (!folder)
    return;

  String path(folder->folderPath);
  os_shell_execute("open", path, nullptr, nullptr);
}

void dag_reveal_in_explorer(String file_path)
{
#if _TARGET_PC_WIN
  file_path.replaceAll("/", "\\");
  Tab<wchar_t> stor;
  LPITEMIDLIST fpath_id = nullptr;
  if (SHParseDisplayName(convert_path_to_u16(stor, file_path), nullptr, &fpath_id, 0, nullptr) == S_OK)
  {
    SHOpenFolderAndSelectItems(fpath_id, 0, nullptr, 0);
    CoTaskMemFree /*ILFree*/ (fpath_id);
    return;
  }
  DEBUG_DUMP_VAR(fpath_id);
  DEBUG_DUMP_VAR(file_path);
#endif
  char fileLocationBuf[DAGOR_MAX_PATH];
  os_shell_execute("open", dd_get_fname_location(fileLocationBuf, file_path), nullptr, nullptr);
}
