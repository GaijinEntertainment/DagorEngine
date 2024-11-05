// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compression.h"
#include <blobMultiStorage.h>
#include <blobDataSource.h>


BlobMultiStorage::BlobMultiStorage(IMemAlloc *allocator, int sz, int quant) :
  mode(EMBM_INTERNAL_DATA), data(allocator, sz, quant), dataPtr(NULL), dataSize(0)
{
  PackDesc dsc;
  data.write(&dsc, sizeof(dsc));

  PackDesc *packDesc = getPackDesc();
  G_ASSERT(packDesc);

  packDesc->init();
}


BlobMultiStorage::BlobMultiStorage(const void *srcData, int size) :
  mode(EMBM_INPLACE_DATA), data(tmpmem, 0), dataPtr((const char *)srcData), dataSize(size)
{
  G_ASSERT(srcData != NULL);

  if (dataSize < sizeof(PackDesc))
  {
    mode = EMBM_INVALID_DATA;
    return;
  }

  PackDesc *packDesc = getPackDesc();
  G_ASSERT(packDesc);

  if (packDesc->prefix != _MAKE4C('PBBD'))
  {
    mode = EMBM_INVALID_DATA;
    return;
  }

  if (packDesc->version == VERSION)
    updateBlobOffsets();
  else
    mode = EMBM_INVALID_DATA;
}


BlobMultiStorage::Error BlobMultiStorage::reset()
{
  G_ASSERT(canWrite());

  if (mode != EMBM_INTERNAL_DATA)
    return EMBS_INVALID_MODE;

  clear_and_shrink(blobOffsets);
  data.setsize(sizeof(PackDesc));

  PackDesc *packDesc = getPackDesc();
  G_ASSERT(packDesc);

  packDesc->init();

  return EMBS_OK;
}


int BlobMultiStorage::getBlobCount()
{
  G_ASSERT(canRead());

  if (mode == EMBM_INVALID_DATA)
    return 0;

  PackDesc *packDesc = getPackDesc();
  G_ASSERT(packDesc);

  return packDesc->blobCount;
}


BlobMultiStorage::Error BlobMultiStorage::compressAndAddBlob(const BlobDataSource &src, const char *compressionName)
{
  const Compression &compr = Compression::getInstanceByName(compressionName);

  return compressAndAddBlob(src, compr);
}


BlobMultiStorage::Error BlobMultiStorage::compressAndAddBlob(const BlobDataSource &src, const Compression &compr)
{
  G_ASSERT(canWrite());

  if (mode != EMBM_INTERNAL_DATA)
    return EMBS_INVALID_MODE;

  if (compr.isValid())
  {
    DynamicMemGeneralSaveCB srcData(tmpmem, 1 << 16);
    src.fillOutStream(srcData);
    int reqSize = compr.getRequiredCompressionBufferLength(srcData.size());
    if (reqSize > 0)
    {
      int oldPos = data.tell();
      G_ASSERT(oldPos == data.size());

      PackBlobDesc *blobDesc = addEmptyBlob(reqSize);
      G_ASSERT(blobDesc);
      char *blobDataPtr = blobDesc->getBlobData();
      const char *outDataPtr = compr.compress(srcData.data(), srcData.size(), blobDataPtr, reqSize);
      if (!outDataPtr || reqSize <= 0)
      {
        data.setsize(oldPos);
        data.seekto(oldPos);

        PackDesc *packDesc = getPackDesc();
        G_ASSERT(packDesc->blobCount > 0);
        --packDesc->blobCount;
        return EMBS_COMPRESS_ERROR;
      }

      int headerSize = sizeof(*blobDesc);
      int totalDataSize = reqSize + headerSize;

      blobDesc->packedSize = reqSize;
      blobDesc->compressorId = compr.getId();

      if (outDataPtr == blobDataPtr)
      {
        data.setsize(oldPos + totalDataSize);
        data.seekto(oldPos + totalDataSize);
      }
      else
      {
        data.setsize(oldPos + headerSize);
        data.seekto(oldPos + headerSize);
        data.write(outDataPtr, reqSize);
      }

      return EMBS_OK;
    }
    else
    {
      return EMBS_COMPRESS_ERROR;
    }
  }
  else
  {
    addEmptyBlob(0);
    int oldPos = data.tell();
    src.fillOutStream(data);

    // get pointer here because internal data may be reallocated during writting
    G_ASSERT(getBlobCount() > 0);
    PackBlobDesc *blobDesc = getBlob(getBlobCount() - 1);
    G_ASSERT(blobDesc);
    blobDesc->packedSize = data.tell() - oldPos;
    blobDesc->compressorId = compr.getId();
  }

  return EMBS_OK;
}


BlobMultiStorage::Error BlobMultiStorage::findBlobAndDecompress(BlobDataSource &outData, int blobIdx)
{
  G_ASSERT(canRead());

  if (mode == EMBM_INVALID_DATA)
    return EMBS_INVALID_DATA;

  PackBlobDesc *packBlobDesc = getBlob(blobIdx);
  if (!packBlobDesc)
    return EMBS_NO_MORE_DATA;

  void *blobData = packBlobDesc->getBlobData();

  const Compression &compr = Compression::getInstanceById(packBlobDesc->compressorId);

  if (compr.isValid())
  {
    int bufSize = compr.getRequiredDecompressionBufferLength(blobData, packBlobDesc->packedSize);
    if (bufSize < 0)
      return EMBS_INVALID_DATA;

    void *decompressedDataPtr = outData.requestBufferForDecompressedData(bufSize);
    if (!decompressedDataPtr)
      return EMBS_INVALID_DATA;

    const char *result = compr.decompress(blobData, packBlobDesc->packedSize, decompressedDataPtr, bufSize);
    if (!result)
    {
      return EMBS_INVALID_DATA;
    }

    outData.processInputData(result, bufSize);
  }
  else
  {
    outData.processInputData(blobData, packBlobDesc->packedSize);
  }


  return EMBS_OK;
}


BlobMultiStorage::Error BlobMultiStorage::copyBlobFrom(BlobMultiStorage &srcStorage, int srcBlobIdx)
{
  G_ASSERT(canWrite());
  G_ASSERT(srcStorage.canRead());
  G_ASSERT(srcBlobIdx >= 0 && srcBlobIdx < srcStorage.getBlobCount());

  if (mode != EMBM_INTERNAL_DATA)
    return EMBS_INVALID_MODE;

  PackBlobDesc *desc = srcStorage.getBlob(srcBlobIdx);
  if (!desc)
    return EMBS_NO_MORE_DATA;

  if (desc->packedSize <= 0)
    return EMBS_INVALID_DATA;

  addBlob(desc->getBlobData(), desc->packedSize, desc->compressorId);

  return EMBS_OK;
}


BlobMultiStorage::PackDesc *BlobMultiStorage::getPackDesc()
{
  G_ASSERT(canRead());

  switch (mode)
  {
    case EMBM_INTERNAL_DATA: return (PackDesc *)data.data();
    case EMBM_INPLACE_DATA: return (PackDesc *)dataPtr;
    case EMBM_INVALID_DATA: return NULL;
  }

  return NULL;
}


int BlobMultiStorage::getDataSize() const
{
  G_ASSERT(canRead());

  switch (mode)
  {
    case EMBM_INTERNAL_DATA: return data.size();
    case EMBM_INPLACE_DATA: return dataSize;
    case EMBM_INVALID_DATA: return 0;
  }

  return 0;
}


BlobMultiStorage::PackBlobDesc *BlobMultiStorage::getBlob(int idx)
{
  G_ASSERT(canRead());

  G_ASSERT(idx >= 0 && idx < blobOffsets.size());

  if (idx < 0 || idx >= blobOffsets.size())
    return NULL;

  char *ptr = (char *)getPackDesc();
  if (!ptr)
    return NULL;

  G_ASSERT(blobOffsets[idx] >= sizeof(PackDesc));
  if ((blobOffsets[idx] + sizeof(PackBlobDesc)) > getDataSize())
    return NULL;

  PackBlobDesc *res = (PackBlobDesc *)(ptr + blobOffsets[idx]);
  if ((blobOffsets[idx] + res->totalBlobSize()) > getDataSize())
    return NULL;

  return res;
}


void BlobMultiStorage::updateBlobOffsets()
{
  clear_and_shrink(blobOffsets);

  PackDesc *packDesc = getPackDesc();
  if (!packDesc)
  {
    mode = EMBM_INVALID_DATA;
    return;
  }

  int sz = getDataSize();

  if (sz < sizeof(PackDesc))
  {
    mode = EMBM_INVALID_DATA;
    return;
  }

  int blobCount = getBlobCount();
  if (blobCount < 0)
  {
    mode = EMBM_INVALID_DATA;
    return;
  }

  if (!blobCount)
    return;

  char *blobBinData = (char *)packDesc;
  int offset = sizeof(*packDesc);

  blobOffsets.reserve(blobCount);

  bool invalid = false;
  for (int i = 0; i < blobCount; ++i)
  {
    if (sz < (offset + sizeof(PackBlobDesc)))
    {
      invalid = true;
      break;
    }

    PackBlobDesc *packBlobDesc = (PackBlobDesc *)(blobBinData + offset);

    if (packBlobDesc->packedSize < 0)
    {
      invalid = true;
      break;
    }

    int blobTotalSize = packBlobDesc->totalBlobSize();
    if ((offset + blobTotalSize) > sz)
    {
      invalid = true;
      break;
    }

    blobOffsets.push_back(offset);

    offset += blobTotalSize;
  }

  if (offset != sz || invalid)
  {
    clear_and_shrink(blobOffsets);
    mode = EMBM_INVALID_DATA;
  }
}


BlobMultiStorage::Error BlobMultiStorage::addBlob(const void *srcData, int srcSize, char compressorId)
{
  G_ASSERT(canWrite());

  if (mode != EMBM_INTERNAL_DATA)
    return EMBS_INVALID_MODE;

  PackBlobDesc desc;
  desc.packedSize = srcSize;
  desc.compressorId = compressorId;

  blobOffsets.push_back(data.tell());

  data.resize(data.tell() + desc.totalBlobSize());
  data.write(&desc, sizeof(desc));
  data.write(srcData, srcSize);

  PackDesc *packDesc = getPackDesc();
  G_ASSERT(packDesc);

  ++packDesc->blobCount;

  return EMBS_OK;
}


BlobMultiStorage::Error BlobMultiStorage::addBlob(const void *srcData, int srcSize)
{
  return addBlob(srcData, srcSize, Compression::getInstanceByName(NULL).getId());
}


BlobMultiStorage::PackBlobDesc *BlobMultiStorage::addEmptyBlob(int srcSize)
{
  G_ASSERT(canWrite());

  if (mode != EMBM_INTERNAL_DATA)
    return NULL;

  blobOffsets.push_back(data.tell());

  PackBlobDesc desc;
  desc.packedSize = srcSize;
  desc.compressorId = Compression::getInstanceByName(NULL).getId();

  data.resize(data.tell() + desc.totalBlobSize());
  data.write(&desc, sizeof(desc));

  data.setsize(data.tell() + srcSize);
  data.seektoend(0);

  PackDesc *packDesc = getPackDesc();
  G_ASSERT(packDesc);

  ++packDesc->blobCount;

  return getBlob(packDesc->blobCount - 1);
}


BlobMultiStorage::Error BlobMultiStorage::getBlobDataRaw(int idx, const char *&ptr, int &size, char &compressorId)
{
  G_ASSERT(canRead());

  PackBlobDesc *blobDesc = getBlob(idx);
  G_ASSERT(blobDesc);

  if (!blobDesc)
    return EMBS_INVALID_DATA;

  G_ASSERT(blobDesc->packedSize >= 0);

  ptr = blobDesc->getBlobData();
  size = blobDesc->packedSize;
  compressorId = (char)blobDesc->compressorId;

  return EMBS_OK;
}

bool BlobMultiStorage::canRead() const { return mode != EMBM_INVALID_DATA; }

bool BlobMultiStorage::canWrite() const { return mode == EMBM_INTERNAL_DATA; }


BlobMultiStorage::Error BlobMultiStorage::getMaxPlainBlobSize(int &size)
{
  size = 0;
  if (mode == EMBM_INVALID_DATA)
    return EMBS_INVALID_MODE;
  G_ASSERT(canRead());

  PackDesc *packDesc = getPackDesc();
  G_ASSERT(packDesc);

  for (int i = 0; i < packDesc->blobCount; i++)
  {
    const char *blobPtr = NULL;
    int blobSize = 0;
    char comprId;
    BlobMultiStorage::Error res = getBlobDataRaw(i, blobPtr, blobSize, comprId);
    if (res != EMBS_OK)
      return res;
    const Compression &compr = Compression::getInstanceById(comprId);
    if (compr.getId() != comprId)
    {
      mode = EMBM_INVALID_DATA;
      return EMBS_INVALID_DATA;
    }
    G_ASSERT(blobPtr != NULL);
    G_ASSERT(blobSize >= 0);
    int currentSize = compr.getRequiredDecompressionBufferLength(blobPtr, blobSize);
    if (currentSize <= 0)
      currentSize = blobSize;
    if (currentSize > size)
      size = currentSize;
  }
  return EMBS_OK;
}
