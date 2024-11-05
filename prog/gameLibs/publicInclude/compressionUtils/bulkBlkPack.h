//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <compressionUtils/blobDataSource.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>

class BlobBlkProcessor : public BlobDataSource
{
public:
  void fillOutStream(DynamicMemGeneralSaveCB &outData) const;
  bool processInputData(const void *data, int size);
  void *requestBufferForDecompressedData(int size);

  DataBlock &getBlk() { return blk; }

private:
  DataBlock blk;
  Tab<char> decompressData;
};
