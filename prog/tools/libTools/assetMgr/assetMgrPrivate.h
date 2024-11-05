// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/assetMgr.h>
#include <util/dag_simpleString.h>

struct DagorAssetMgr::RootEntryRec
{
  SimpleString folder;
  int nsId;
};

struct DagorAssetMgr::PerAssetIdNotifyTab
{
  struct Rec
  {
    int assetId;
    IDagorAssetChangeNotify *client;
  };
  Tab<Rec> notify;

  PerAssetIdNotifyTab() : notify(midmem) {}
};
