//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DynamicMemGeneralSaveCB;

class BlobDataSource
{
public:
  BlobDataSource() {}
  virtual ~BlobDataSource() {}

  virtual void fillOutStream(DynamicMemGeneralSaveCB &outData) const = 0;
  virtual bool processInputData(const void *data, int size) = 0;
  virtual void *requestBufferForDecompressedData(int size) = 0;

private:
  BlobDataSource(const BlobDataSource &);
  BlobDataSource &operator=(const BlobDataSource &);
};
