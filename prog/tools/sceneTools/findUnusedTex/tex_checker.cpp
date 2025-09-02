// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "tex_checker.h"

#include <debug/dag_debug.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>

#include <libTools/util/binDumpReader.h>
#include <ioSys/dag_fileIo.h>

#include <util/dag_string.h>

#include <assets/asset.h>
#include <assets/assetHlp.h>
#include <assets/assetRefs.h>

#include <windows.h>
#include <libTools/util/setupTexStreaming.h>


TexChecker::TexChecker(const char *input_file1, const char *input_file2) :
  mAssetTexturesList(midmem), mAssetTexturesPath(midmem), mTexturesInAssetsList(midmem), mBinLvlsTexList(midmem), mGameTexList(midmem)
{
  mBlk1.load(input_file1);
  if (input_file2)
    mBlk2.load(input_file2);

  DataBlock *textures = mBlk2.getBlockByName("textures");
  if (textures)
    for (int i = 0; i < textures->paramCount(); ++i)
      if (textures->getParamType(i) == DataBlock::TYPE_STRING)
        mGameTexList.push_back() = textures->getStr(i);

  readAssetBaseTexList();
  readBinLvlsTexList();

  if (mBlk1.getBool("dumpWorkLists", false))
  {
    debug(" ===== texture assets (asset base) :");
    for (int i = 0; i < mAssetTexturesList.size(); ++i)
      debug("%s", mAssetTexturesList[i].str());

    debug("\n");

    debug(" ===== texture refs in other assets (asset base) :");
    for (int i = 0; i < mTexturesInAssetsList.size(); ++i)
      debug("%s", mTexturesInAssetsList[i].str());

    debug(" ===== textures in level.bin files :");
    for (int i = 0; i < mBinLvlsTexList.size(); ++i)
      debug("%s", mBinLvlsTexList[i].str());

    debug(" ===== textures in blk (direct list) :");
    for (int i = 0; i < mGameTexList.size(); ++i)
      debug("%s", mGameTexList[i].str());
  }
}


bool TexChecker::isTextureInArray(const char *tex_name, const Tab<SimpleString> &check_array)
{
  for (int i = 0; i < check_array.size(); ++i)
  {
    String tempString(check_array[i].str());
    if (!strcmp(tex_name, tempString.toLower().str()))
      return true;
  }

  return false;
}


void TexChecker::CheckTextures(Tab<SimpleString> &out_assets, Tab<SimpleString> &out_fnames)
{
  for (int i = 0; i < mAssetTexturesList.size(); ++i)
  {
    String checkingTexName(mAssetTexturesList[i]);
    checkingTexName.toLower();
    if (isTextureInArray(checkingTexName.str(), mTexturesInAssetsList))
      continue;

    if (isTextureInArray(checkingTexName.str(), mBinLvlsTexList))
      continue;

    if (isTextureInArray(checkingTexName.str(), mGameTexList))
      continue;

    out_assets.push_back() = mAssetTexturesList[i];
    out_fnames.push_back() = mAssetTexturesPath[i];
  }
}


void TexChecker::readAssetTextures(DagorAsset *asset)
{
  IDagorAssetRefProvider *refProvider = mAssetMgr.getAssetRefProvider(asset->getType());
  if (!refProvider)
    return;

  Tab<IDagorAssetRefProvider::Ref> refs(tmpmem);
  refProvider->getAssetRefs(*asset, refs);

  for (int i = 0; i < refs.size(); ++i)
  {
    DagorAsset *cur_asset = refs[i].getAsset();
    if (!cur_asset)
      continue;

    if (strstr("tex", mAssetMgr.getAssetTypeName(cur_asset->getType())))
      mTexturesInAssetsList.push_back() = cur_asset->getName();
    else
      readAssetTextures(cur_asset);
  }
}


void TexChecker::readAssetBaseTexList()
{
  String app_dir(mBlk1.getStr("app_dir", "."));
  app_dir.append("\\");
  String fname(260, "%sapplication.blk", app_dir.str());

  DataBlock appblk;

  if (!appblk.load(fname))
    debug("application.blk not loaded");

  ::load_tex_streaming_settings(fname, NULL, true);

  // load asset base
  const DataBlock &blk = *appblk.getBlockByNameEx("assets");
  int base_nid = blk.getNameId("base");

  mAssetMgr.setupAllowedTypes(*blk.getBlockByNameEx("types"), blk.getBlockByName("export"));

  for (int i = 0; i < blk.paramCount(); i++)
    if (blk.getParamNameId(i) == base_nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
    {
      fname.printf(260, "%s%s", app_dir.str(), blk.getStr(i));
      mAssetMgr.loadAssetsBase(fname, "global");
    }

  char _path[512];
  GetModuleFileName(GetModuleHandle(NULL), (LPTSTR)_path, 512);
  ::dd_get_fname_location(_path, _path);

  if (assetrefs::load_plugins(mAssetMgr, appblk, _path))
    debug("asset refs plugins inited");
  else
    debug("asset refs plugins not inited");

  dag::ConstSpan<DagorAsset *> assets = mAssetMgr.getAssets();
  for (int i = 0; i < assets.size(); i++)
  {
    DagorAsset *a = assets[i];
    if (strstr("tex", mAssetMgr.getAssetTypeName(a->getType())))
    {
      mAssetTexturesList.push_back() = a->getName();
      mAssetTexturesPath.push_back() = a->getSrcFilePath();
    }
    else
      readAssetTextures(a);
  }

  assetrefs::unload_plugins(mAssetMgr);
}


void TexChecker::readBinLvlsTexList()
{
  DataBlock *binPahtDataBlock = mBlk1.getBlockByName("level_bins");

  if (binPahtDataBlock)
    for (int i = 0; i < binPahtDataBlock->paramCount(); ++i)
    {
      const char *binPaht = binPahtDataBlock->getStr(i);
      unsigned targetCode = 0;
      bool read_be = dagor_target_code_be(targetCode);

      FullFileLoadCB base_crd(binPaht);
      if (!base_crd.fileHandle)
      {
        debug("ERROR: cannot open <%s>", binPaht);
        return;
      }
      else
        debug("open file <%s>", binPaht);

      BinDumpReader crd(&base_crd, targetCode, read_be);
      int fileLength = df_length(base_crd.fileHandle);

      for (;;)
      {
        int pos = df_tell(base_crd.fileHandle);
        if (pos >= fileLength - 1)
          return;

        if (crd.readFourCC() != _MAKE4C('DxP2'))
          continue;

        String temp;
        crd.readDwString(temp);

        int texCount = crd.readInt32e();

        for (int i = 0; i < texCount; ++i)
        {
          String texName;
          crd.readDwString(texName);
          texName.chop(1);

          mBinLvlsTexList.push_back() = texName.str();
        }

        break;
      }
    }
}
