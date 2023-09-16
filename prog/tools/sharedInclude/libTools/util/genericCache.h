//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_oaHashNameMap.h>
#include <util/dag_simpleString.h>
#include <util/dag_hierBitArray.h>
#include <generic/dag_tab.h>
#include <libTools/util/twoStepRelPath.h>

class DataBlock;

namespace mkbindump
{
class BinDumpSaveCB;
}

class GenericBuildCache
{
public:
  static const int HASH_SZ = 16;

  GenericBuildCache() : rec(midmem)
  {
    memset(&target, 0, sizeof(target));
    timeChanged = 0;
  }

  void reset();

  bool load(const char *cache_fname, int *end_pos = NULL);
  bool save(const char *cache_fname, mkbindump::BinDumpSaveCB *a_data = NULL);

  void resetTouchMark();
  bool checkAllTouched();
  void removeUntouched();

  bool checkFileChanged(const char *fname);
  bool checkDataBlockChanged(const char *fname_ref, DataBlock &blk);
  bool checkTargetFileChanged(const char *fname);
  void updateFileHash(const char *fname);

  const NameMapCI &getNameList() const { return fnames; }

  void setTargetFileHash(const char *fname);

  static bool getFileHash(const char *fname, unsigned char out_hash[HASH_SZ]);
  static void getDataHash(const void *data, int data_len, unsigned char out_hash[HASH_SZ]);
  static unsigned getFileTime(const char *fname, unsigned &out_sz);

  static void setSdkRoot(const char *root_dir, const char *subdir = nullptr) { sdkRoot.setSdkRoot(root_dir, subdir); }
  static const char *mkRelPath(const char *fpath) { return sdkRoot.mkRelPath(fpath); }

  bool isTimeChanged() { return timeChanged == 1; }

protected:
  struct FileDataHash
  {
    unsigned char hash[HASH_SZ];
    unsigned timeStamp;
    unsigned changed : 1;
  };

  NameMapCI fnames;
  Tab<FileDataHash> rec;
  FileDataHash target;

  typedef HierBitArray<ConstSizeBitArray<8>> bitarray_t;
  bitarray_t touchMark;
  int touchMarkSz = 0;
  unsigned timeChanged : 1;

  static TwoStepRelPath sdkRoot;

  void touch(int i) { (i < touchMarkSz) ? touchMark.set(i) : (void)0; }
};
