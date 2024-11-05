// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <ioSys/dag_dataBlock.h>
#include <assets/assetMgr.h>


class TexChecker
{
public:
  TexChecker(const char *input_file1, const char *input_file2);

  void CheckTextures(Tab<SimpleString> &out_assets, Tab<SimpleString> &out_fnames);

private:
  void readAssetBaseTexList();
  void readAssetTextures(DagorAsset *asset);
  void readBinLvlsTexList();

  bool isTextureInArray(const char *tex_name, const Tab<SimpleString> &check_array);

  DagorAssetMgr mAssetMgr;

  DataBlock mBlk1, mBlk2;

  Tab<SimpleString> mAssetTexturesList;
  Tab<SimpleString> mAssetTexturesPath;

  Tab<SimpleString> mTexturesInAssetsList;
  Tab<SimpleString> mBinLvlsTexList;
  Tab<SimpleString> mGameTexList;
};
