#pragma once

#include <regExp/regExp.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <util/dag_oaHashNameMap.h>

class IDagorAssetMsgPipe;


class DagorVirtualAssetRule
{
public:
  DagorVirtualAssetRule(DataBlock *sharedNmBlk) : reExclude(midmem)
  {
    addSrcFileAsBlk = 0;
    stopProcessing = 1;
    replaceStrings = 1;
    ignoreDupAsset = overrideDupAsset = 0;
    contents.setSharedNameMapAndClearData(sharedNmBlk);
  }
  ~DagorVirtualAssetRule()
  {
    clear_all_ptr_items(reExclude);
    clear_all_ptr_items(tagUpd);
  }

  bool load(const DataBlock &blk, IDagorAssetMsgPipe &msg_pipe);

  //! returns true and sets out_asset_name if rule is applicable for given filename
  bool testRule(const char *fname, String &out_asset_name);

  //! fills blk with VA content (must be called immediately AFTER testRule() returns true)
  void applyRule(DataBlock &dest_blk, const char *full_fn);

  bool shouldAddSrcBlk() const { return addSrcFileAsBlk; }
  bool shouldStopProcessing() const { return stopProcessing; }
  bool shouldIgnoreDuplicateAsset() const { return ignoreDupAsset; }
  bool shouldOverrideDuplicateAsset() const { return overrideDupAsset; }
  const char *getTypeName() const { return typeName.empty() ? NULL : typeName.str(); }

protected:
  RegExp reFind;
  Tab<RegExp *> reExclude;

  DataBlock contents;
  FastNameMap tagNm;
  Tab<DataBlock *> tagUpd;
  String nameTemplate, typeName;
  unsigned addSrcFileAsBlk : 1, stopProcessing : 1, replaceStrings : 1, ignoreDupAsset : 1, overrideDupAsset : 1;
};
