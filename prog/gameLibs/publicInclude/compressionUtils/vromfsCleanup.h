//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_zstdIo.h>
#include <memory/dag_memBase.h>
#include "finallySafe.h"


template <typename D>
struct NonIntrusiveVromfsDeleter
{
  void operator()(VirtualRomFsData *ptr) const
  {
    if (ptr)
    {
      FinallySafe discardBlkDict([&ptr]() { dblk::discard_non_intrusive_vromfs_blk_ddict(ptr); });
    }

    D deleter;
    deleter(ptr);
  }
};

using TmpmemNonIntrusiveVromfsDeleter = NonIntrusiveVromfsDeleter<tmpmemDeleter>;


struct ZstdDblkDDictDeleter
{
  void operator()(ZSTD_DDict_s *ptr) const
  {
    if (!ptr)
      return;
    FinallySafe releaseZstdDblkDDict([&ptr]() { dblk::release_vromfs_blk_ddict(ptr); });
  }
};


struct ZstdCDictDeleter
{
  void operator()(ZSTD_CDict_s *ptr) const
  {
    if (!ptr)
      return;
    FinallySafe destroyZstdCDict([&ptr]() { zstd_destroy_cdict(ptr); });
  }
};
