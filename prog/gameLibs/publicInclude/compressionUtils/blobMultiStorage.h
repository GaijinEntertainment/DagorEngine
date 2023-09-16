//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <ioSys/dag_memIo.h>

class BlobDataSource;
class Compression;

class BlobMultiStorage
{
public:
  enum Error
  {
    EMBS_OK = 0,
    EMBS_INVALID_VERSION,
    EMBS_NO_MORE_DATA,
    EMBS_INVALID_DATA,
    EMBS_INVALID_MODE,
    EMBS_COMPRESS_ERROR
  };

  enum Mode
  {
    EMBM_INVALID_DATA, // error state
    EMBM_INPLACE_DATA, // read only mode from external data
    EMBM_INTERNAL_DATA
  };

  BlobMultiStorage(IMemAlloc *_allocator = tmpmem, int sz = 0, int quant = 64 << 10);
  BlobMultiStorage(const void *srcData, int size);

  Error compressAndAddBlob(const BlobDataSource &srcData, const char *compressionName);
  Error compressAndAddBlob(const BlobDataSource &srcData, const Compression &compr);
  int getBlobCount();
  Error findBlobAndDecompress(BlobDataSource &outData, int blobIdx);

  Error copyBlobFrom(BlobMultiStorage &srcStorage, int srcBlobIdx);

  Error reset();

  int getDataSize() const;
  const void *getData() { return getPackDesc(); }

  Error addBlob(const void *srcData, int srcSize);

  Mode getMode() const { return mode; }

  Error getBlobDataRaw(int idx, const char *&ptr, int &size, char &compressorId);

  Error getMaxPlainBlobSize(int &size);

private:
  DynamicMemGeneralSaveCB data;
  Tab<int> blobOffsets;

  static constexpr int VERSION = 1;

#pragma pack(push, 1)
  struct PackDesc
  {
    uint32_t prefix;
    uint16_t version;
    uint16_t blobCount;

    void init()
    {
      prefix = _MAKE4C('PBBD');
      version = VERSION;
      blobCount = 0;
    }
  };

  struct PackBlobDesc
  {
    int32_t packedSize;
    int8_t compressorId;

    char *getBlobData() { return (char *)(this + 1); }
    int totalBlobSize() const { return sizeof(*this) + packedSize; }
  };
#pragma pack(pop)

  PackDesc *getPackDesc();
  Mode mode;
  const char *dataPtr;
  int dataSize;


  PackBlobDesc *getBlob(int idx);
  void updateBlobOffsets();
  BlobMultiStorage::Error addBlob(const void *srcData, int srcSize, char compressorId);
  PackBlobDesc *addEmptyBlob(int srcSize);

  bool canRead() const;
  bool canWrite() const;
};
