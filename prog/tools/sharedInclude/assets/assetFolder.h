//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_simpleString.h>


class DagorAssetFolder
{
public:
  enum
  {
    FLG_EXPORT_ASSETS = 0x0001,
    FLG_SCAN_ASSETS = 0x0002,
    FLG_SCAN_FOLDERS = 0x0004,
    FLG_INHERIT_RULES = 0x0008,
  };

  DagorAssetFolder(int parent, const char *folder_name, const char *folder_path) :
    parentIdx(parent),
    folderName(folder_name),
    folderPath(folder_path),
    startVaRuleIdx(-1),
    vaRuleCount(0),
    subFolderIdx(midmem),
    flags(FLG_EXPORT_ASSETS | FLG_SCAN_ASSETS | FLG_SCAN_FOLDERS | FLG_INHERIT_RULES)
  {}

public:
  SimpleString folderName, folderPath;
  short startVaRuleIdx, vaRuleCount;
  short parentIdx, flags;
  Tab<short> subFolderIdx;
  DataBlock exportProps;
};
