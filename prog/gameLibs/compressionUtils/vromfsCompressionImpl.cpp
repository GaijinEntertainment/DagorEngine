// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compressionImpl.h"
#include "vromfsCompressionImpl.h"
#include <zlib.h>
#include <openssl/md5.h>
#include <generic/dag_tab.h>
#include <supp/dag_zstdObfuscate.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zstdIo.h>

// vrom header:
// 'VRFs' or 'VRFx': 4 bytes
// target code: 4 bytes, validated with dagor_target_code_valid() from
//      prog/tools/sharedInclude/libTools/util/targetClass.h
//      Target code determines whether sizes are LE or BE (dagor_target_code_be())
// plain size: 4 bytes
// packed size: 4 bytes
// optional header (VRFx only):
// ext header size: 2 bytes
// flags: 2 bytes
// version: 4 bytes
// data: <packed size> bytes. If packed size is zero then <plain size> bytes.
//

static const int FULL_VROM_HEADER_SIZE = sizeof(VirtualRomFsDataHdr);
static const int VROM_HEADER_SIZE = FULL_VROM_HEADER_SIZE - 1; // the 'V' letter does not get through
static const int MD5_HASH_SIZE = 16;
static const int REASONABLE_SIGNATURE_SIZE = 4096;
static const unsigned VRFS_4C = _MAKE4C('VRFs');
static const unsigned VRFX_4C = _MAKE4C('VRFx');

// The compression level varies from 1 to 22 as of zstd 1.2, 22 being extremely slow.
// According to the testing results presented in zstd's readme, level 9 gives
// a decent compression rate of 3.5 on the Silesia compression corpus and processes
// data at about 55 MB/s on a Core i7-6700K
static const int DEFAULT_ZSTD_COMPRESSION_LEVEL = 9;

static void unused_make_sure_platform_is_little_endian()
{
#if _TARGET_CPU_BE
  char big_endian_not_supported;
  G_STATIC_ASSERT(sizeof(bid_endian_not_supported) == 0);
#endif
}


static bool check_md5_hash(const char *data, int size, const char *hash)
{
  if (!hash || !data)
    return false;
  unsigned char calculatedMd5Hash[MD5_HASH_SIZE];
  MD5_CTX md5Context;
  MD5_Init(&md5Context);
  MD5_Update(&md5Context, (const unsigned char *)data, size);
  MD5_Final(calculatedMd5Hash, &md5Context);
  return memcmp(calculatedMd5Hash, hash, MD5_HASH_SIZE) == 0;
}


VromfsCompression::VromfsCompression() : checkerFactoryCb(NULL) { defaultLevel = DEFAULT_ZSTD_COMPRESSION_LEVEL; }


VromfsCompression::VromfsCompression(signature_checker_factory_cb callback, const char *fname) :
  checkerFactoryCb(callback), fileName(fname)
{
  defaultLevel = DEFAULT_ZSTD_COMPRESSION_LEVEL;
}


int VromfsCompression::getMinLevel() const { return 1; }


int VromfsCompression::getMaxLevel() const
{
  return 20; // avoid using ZSTD_maxCLevel() since level>20 means ultra compression and requires much memory
}


#define IS_NOT_VROM_IF_TRUE(cond)                                                \
  if (cond)                                                                      \
  {                                                                              \
    if (checkType == CHECK_VERBOSE)                                              \
    {                                                                            \
      logwarn("%s(%d): condition triggered: %s", __FUNCTION__, __LINE__, #cond); \
    }                                                                            \
    return IDT_DATA;                                                             \
  }

VromfsCompression::InputDataType VromfsCompression::getDataType(const char *data, int size, CheckType checkType) const
{
  IS_NOT_VROM_IF_TRUE(size < VROM_HEADER_SIZE);
  G_ASSERT(data != NULL);
  char reconstructedHeader[FULL_VROM_HEADER_SIZE];
  memcpy(&reconstructedHeader[1], data, VROM_HEADER_SIZE);
  reconstructedHeader[0] = 'V';
  const VirtualRomFsDataHdr *header = (const VirtualRomFsDataHdr *)reconstructedHeader;
  unsigned vrfs4C = header->label;
  IS_NOT_VROM_IF_TRUE(vrfs4C != VRFS_4C && vrfs4C != VRFX_4C);
  //== only LE targets are supported now
  IS_NOT_VROM_IF_TRUE(header->target != _MAKE4C('PC') && header->target != _MAKE4C('iOS') && header->target != _MAKE4C('and') &&
                      header->target != _MAKE4C('PS4'));

  unsigned extHeaderSize = 0;
  if (vrfs4C == VRFX_4C)
  {
    IS_NOT_VROM_IF_TRUE(size < VROM_HEADER_SIZE + sizeof(VirtualRomFsExtHdr));
    const VirtualRomFsExtHdr *extHdr = (const VirtualRomFsExtHdr *)&data[VROM_HEADER_SIZE];
    extHeaderSize = extHdr->size;
    IS_NOT_VROM_IF_TRUE(size < VROM_HEADER_SIZE + extHeaderSize);
  }
  unsigned totalHeaderSize = VROM_HEADER_SIZE + extHeaderSize;

  unsigned packedSize = header->packedSz();
  unsigned plainSize = header->fullSz;
  IS_NOT_VROM_IF_TRUE(packedSize > Compression::MAX_PLAIN_DATA_SIZE);
  IS_NOT_VROM_IF_TRUE(plainSize > Compression::MAX_PLAIN_DATA_SIZE);
  unsigned srcBodySize = packedSize ? packedSize : plainSize;

  int signatureOffset = totalHeaderSize + srcBodySize + MD5_HASH_SIZE;
  IS_NOT_VROM_IF_TRUE(size < signatureOffset);
  int signatureSize = size - signatureOffset;
  G_ASSERT(signatureSize >= 0);
  if (checkerFactoryCb)
  {
    IS_NOT_VROM_IF_TRUE(signatureSize == 0);
    IS_NOT_VROM_IF_TRUE(signatureSize > REASONABLE_SIGNATURE_SIZE);
    IS_NOT_VROM_IF_TRUE(!header->signedContents());
  }

  if (checkType == CHECK_LAZY)
    return packedSize ? IDT_VROM_COMPRESSED : IDT_VROM_PLAIN;

  if (packedSize)
  {
    Tab<char> plainData(tmpmem);
    plainData.resize(plainSize);
    if (header->zstdPacked())
    {
      DEOBFUSCATE_ZSTD_DATA(const_cast<char *>(data + totalHeaderSize), header->packedSz());
      size_t sz = zstd_decompress(plainData.data(), plainSize, &data[totalHeaderSize], header->packedSz());
      IS_NOT_VROM_IF_TRUE(sz != header->fullSz);
      OBFUSCATE_ZSTD_DATA(const_cast<char *>(data + totalHeaderSize), header->packedSz());
    }
    else
    {
      z_stream zlibStream;
      zlibStream.zalloc = Z_NULL;
      zlibStream.zfree = Z_NULL;
      zlibStream.opaque = Z_NULL;
      zlibStream.avail_in = 0;
      zlibStream.next_in = Z_NULL;
      inflateInit(&zlibStream);
      zlibStream.avail_in = header->packedSz();
      zlibStream.next_in = (Bytef *)&data[totalHeaderSize];
      zlibStream.avail_out = plainSize;
      zlibStream.next_out = (Bytef *)plainData.data();
      int r = inflate(&zlibStream, Z_FINISH);
      unsigned availOutAfterInflate = zlibStream.avail_out;
      inflateEnd(&zlibStream);
      IS_NOT_VROM_IF_TRUE(r != Z_STREAM_END || availOutAfterInflate != 0);
    }
    if (checkerFactoryCb)
    {
      G_ASSERT(signatureSize > 0);
      VromfsDumpSections sections;
      sections.header = make_span_const((uint8_t *)reconstructedHeader, sizeof(reconstructedHeader));
      sections.headerExt = make_span_const((uint8_t *)&data[VROM_HEADER_SIZE], extHeaderSize);
      sections.body = make_span_const((uint8_t *)plainData.data(), plainSize);
      sections.md5 = make_span_const((uint8_t *)&data[totalHeaderSize + packedSize], MD5_HASH_SIZE);
      sections.signature = make_span_const((uint8_t *)&data[signatureOffset], signatureSize);
      auto checker = checkerFactoryCb();
      dag::ConstSpan<uint8_t> fileNameSpan = make_span_const((uint8_t *)fileName.str(), fileName.length());
      IS_NOT_VROM_IF_TRUE(!check_vromfs_dump_signature(*checker, sections, &fileNameSpan));
    }
    else
    {
      IS_NOT_VROM_IF_TRUE(!check_md5_hash(plainData.data(), plainSize, &data[totalHeaderSize + packedSize]));
    }
    return IDT_VROM_COMPRESSED;
  }

  // non-packed vrom
  const char *storedMd5Hash = &data[totalHeaderSize + plainSize];
  const char *dataToCheck = &data[totalHeaderSize];
  if (checkerFactoryCb)
  {
    G_ASSERT(signatureSize > 0);
    VromfsDumpSections sections;
    sections.header = make_span_const((uint8_t *)reconstructedHeader, sizeof(reconstructedHeader));
    sections.headerExt = make_span_const((uint8_t *)&data[VROM_HEADER_SIZE], extHeaderSize);
    sections.body = make_span_const((uint8_t *)&data[totalHeaderSize], plainSize);
    sections.md5 = make_span_const((uint8_t *)&data[totalHeaderSize + plainSize], MD5_HASH_SIZE);
    sections.signature = make_span_const((uint8_t *)&data[signatureOffset], signatureSize);
    auto checker = checkerFactoryCb();
    dag::ConstSpan<uint8_t> fileNameSpan = make_span_const((uint8_t *)fileName.str(), fileName.length());
    IS_NOT_VROM_IF_TRUE(!check_vromfs_dump_signature(*checker, sections, &fileNameSpan));
  }
  else
  {
    IS_NOT_VROM_IF_TRUE(!check_md5_hash(dataToCheck, plainSize, storedMd5Hash));
  }
  return IDT_VROM_PLAIN;
}


int VromfsCompression::getRequiredCompressionBufferLength(int plainDataLen, int level) const
{
  G_UNREFERENCED(level);
  // as we use zlib for both vrom and non-vrom data, the worst case will be
  // the same as zlib's one plus the 'z' letter
  return Compression::getInstanceById('z').getRequiredCompressionBufferLength(plainDataLen) + 1;
}


int VromfsCompression::getRequiredDecompressionBufferLength(const void *data_, int dataLen) const
{
  const char *data = (const char *)data_;
  if (!data)
    return -1;
  if (dataLen < 0)
    return -1;
  switch (getDataType(data, dataLen, CHECK_LAZY))
  {
    case IDT_VROM_PLAIN: return dataLen + 1; // the 'V' letter in front
    case IDT_VROM_COMPRESSED:
    {
      G_ASSERT_RETURN(dataLen >= VROM_HEADER_SIZE, -1);
      char reconstructedHeader[FULL_VROM_HEADER_SIZE];
      memcpy(&reconstructedHeader[1], data, VROM_HEADER_SIZE);
      reconstructedHeader[0] = 'V';
      const VirtualRomFsDataHdr *header = (const VirtualRomFsDataHdr *)reconstructedHeader;
      unsigned extHeaderSize = 0;
      if (header->label == VRFX_4C)
      {
        const VirtualRomFsExtHdr *extHdr = (const VirtualRomFsExtHdr *)&data[VROM_HEADER_SIZE];
        extHeaderSize = extHdr->size;
      }
      unsigned totalHeaderSize = VROM_HEADER_SIZE + extHeaderSize;
      unsigned signatureOffset = totalHeaderSize + header->packedSz() + MD5_HASH_SIZE;
      G_ASSERT_RETURN(dataLen >= signatureOffset, -1);
      int signatureSize = dataLen - signatureOffset;
      G_ASSERT_RETURN(!checkerFactoryCb || signatureSize > 0, -1);
      G_ASSERT_RETURN(!checkerFactoryCb || signatureSize <= REASONABLE_SIGNATURE_SIZE, -1);
      return (int)(FULL_VROM_HEADER_SIZE + extHeaderSize + header->fullSz + MD5_HASH_SIZE + signatureSize);
    }
    case IDT_DATA:
      if (data[0] == 'z')
        return Compression::getInstanceById('z').getRequiredDecompressionBufferLength(&data[1], dataLen - 1);
      return -1;
  }
  return -1;
}


const char *VromfsCompression::compress(const void *in_, int inLen, void *out_, int &outLen, int level) const
{
  int compressionLevel = clampLevel(level);
  if (!in_)
    return NULL;
  if (!out_)
    return NULL;
  int reqOutLen = getRequiredCompressionBufferLength(inLen, compressionLevel);
  if (outLen < reqOutLen)
    return NULL;
  const char *in = (const char *)in_;
  const VirtualRomFsDataHdr *inVromHeader = (const VirtualRomFsDataHdr *)in_;
  char *out = (char *)out_;
  char newVromHeaderStorage[FULL_VROM_HEADER_SIZE];
  VirtualRomFsDataHdr *outVromHeader = (VirtualRomFsDataHdr *)newVromHeaderStorage;
  InputDataType inputType = IDT_DATA;
  if (in[0] == 'V')
    inputType = getDataType(&in[1], inLen - 1, CHECK_STRICT);
  switch (inputType)
  {
    case IDT_VROM_PLAIN:
    {
      G_ASSERT(outLen >= VROM_HEADER_SIZE);
      G_ASSERT(out != NULL);
      unsigned extHeaderSize = 0;
      if (inVromHeader->label == VRFX_4C)
      {
        const VirtualRomFsExtHdr *extHdr = (const VirtualRomFsExtHdr *)&in[FULL_VROM_HEADER_SIZE];
        extHeaderSize = extHdr->size;
      }
      unsigned signatureOffset = FULL_VROM_HEADER_SIZE + extHeaderSize + inVromHeader->fullSz + MD5_HASH_SIZE;
      if (inLen < signatureOffset)
      {
        logwarn("%s: signature offset is out of file, %u > %d", __FUNCTION__, signatureOffset, inLen);
        return NULL;
      }
      int signatureSize = inLen - signatureOffset;
      G_ASSERT(!checkerFactoryCb || signatureSize > 0);
      G_ASSERT(!checkerFactoryCb || signatureSize <= REASONABLE_SIGNATURE_SIZE);
      const char *signaturePtr = &in[signatureOffset];

      // do vrom compression
      ConstrainedMemSaveCB cwr(out + VROM_HEADER_SIZE + extHeaderSize, outLen - VROM_HEADER_SIZE - extHeaderSize);
      InPlaceMemLoadCB crd(in + FULL_VROM_HEADER_SIZE + extHeaderSize, inVromHeader->fullSz);
      int sz = zstd_compress_data(cwr, crd, crd.getTargetDataSize(), 1 << 20, 11);
      if (sz <= 0 || outLen < sz + VROM_HEADER_SIZE + extHeaderSize + MD5_HASH_SIZE + signatureSize)
        return NULL;

      OBFUSCATE_ZSTD_DATA(out + VROM_HEADER_SIZE + extHeaderSize, sz);
      outLen = sz + VROM_HEADER_SIZE + extHeaderSize;

      G_ASSERT(outLen >= VROM_HEADER_SIZE + extHeaderSize);
      outVromHeader->label = inVromHeader->label;
      outVromHeader->target = inVromHeader->target;
      outVromHeader->fullSz = inVromHeader->fullSz;
      // 0x80000000: packed, 0x40000000: zstd compression
      outVromHeader->hw32 = (outLen - VROM_HEADER_SIZE - extHeaderSize) | 0xC0000000;
      // copy headers
      memcpy(out, &newVromHeaderStorage[1], VROM_HEADER_SIZE);
      if (extHeaderSize > 0)
        memcpy(&out[VROM_HEADER_SIZE], &in[FULL_VROM_HEADER_SIZE], extHeaderSize);
      // copy MD5 hash
      memcpy(&out[outLen], &in[FULL_VROM_HEADER_SIZE + extHeaderSize + inVromHeader->fullSz], MD5_HASH_SIZE);
      outLen += MD5_HASH_SIZE;
      // copy signature
      memcpy(&out[outLen], &in[signatureOffset], signatureSize);
      outLen += signatureSize;
      return out;
    }
    case IDT_VROM_COMPRESSED:
      // already compressed, return it as is
      outLen = inLen - 1;
      return &in[1];
    case IDT_DATA:
      // compress it with zlib
      G_ASSERT(outLen >= 1);
      G_ASSERT(out != NULL);
      out[0] = 'z';
      outLen--;
      // zlib never returns a pointer to in, so we can assume that the data is in out
      if (!Compression::getInstanceById('z').compress(in, inLen, &out[1], outLen))
      {
        return NULL;
      }
      outLen++;
      return out;
  }
  G_ASSERTF(0, "Should never get here (getDataType returned something unexpected?)");
  return NULL;
}


const char *VromfsCompression::decompress(const void *in_, int inLen, void *out_, int &outLen) const
{
  if (inLen <= 0 || !in_)
  {
    logwarn("%s: empty input", __FUNCTION__);
    return NULL;
  }
  const char *in = (const char *)in_;
  int reqOutLen = getRequiredDecompressionBufferLength(in, inLen);
  if (outLen < reqOutLen)
  {
    logwarn("%s: too small output buffer, %d < %d", __FUNCTION__, outLen, reqOutLen);
    return NULL;
  }
  char *out = (char *)out_;
  char reconstructedHeader[FULL_VROM_HEADER_SIZE];
  reconstructedHeader[0] = 'V';
  const VirtualRomFsDataHdr *inVromHeader = (const VirtualRomFsDataHdr *)reconstructedHeader;
  VirtualRomFsDataHdr *outVromHeader = (VirtualRomFsDataHdr *)out;
  switch (getDataType(in, inLen, CHECK_LAZY))
  {
    case IDT_VROM_PLAIN:
    {
      // return the plain vrom back
      G_ASSERT(outLen >= inLen + 1);
      G_ASSERT(out != NULL);
      out[0] = 'V';
      memcpy(&out[1], in, inLen);
      outLen = inLen + 1;
      return out;
    }
    case IDT_VROM_COMPRESSED:
      // do vrom decompression
      {
        G_ASSERT(inLen >= VROM_HEADER_SIZE);
        G_ASSERT(out != NULL);
        memcpy(&reconstructedHeader[1], in, VROM_HEADER_SIZE);
        unsigned extHeaderSize = 0;
        if (inVromHeader->label == VRFX_4C)
        {
          const VirtualRomFsExtHdr *extHdr = (const VirtualRomFsExtHdr *)&in[VROM_HEADER_SIZE];
          extHeaderSize = extHdr->size;
        }
        unsigned signatureOffset = VROM_HEADER_SIZE + extHeaderSize + inVromHeader->packedSz() + MD5_HASH_SIZE;
        if (inLen < signatureOffset)
        {
          logwarn("%s: signature offset is out of file, %u > %d", __FUNCTION__, signatureOffset, inLen);
          return NULL;
        }
        int signatureSize = inLen - signatureOffset;
        G_ASSERT(!checkerFactoryCb || signatureSize > 0);
        G_ASSERT(!checkerFactoryCb || signatureSize <= REASONABLE_SIGNATURE_SIZE);
        unsigned uncompressedLen = FULL_VROM_HEADER_SIZE + extHeaderSize + inVromHeader->fullSz + MD5_HASH_SIZE + signatureSize;
        if (uncompressedLen > MAX_PLAIN_DATA_SIZE)
        {
          logwarn("%s: insane uncompressedLen, %u > %d", __FUNCTION__, uncompressedLen, (int)MAX_PLAIN_DATA_SIZE);
          return NULL;
        }
        G_ASSERT(outLen >= uncompressedLen);
        bool isDecompressionOk = true;
        if (inVromHeader->zstdPacked())
        {
          DEOBFUSCATE_ZSTD_DATA(const_cast<char *>(in + VROM_HEADER_SIZE + extHeaderSize), inVromHeader->packedSz());
          int sz = zstd_decompress(out + FULL_VROM_HEADER_SIZE + extHeaderSize, inVromHeader->fullSz,
            in + VROM_HEADER_SIZE + extHeaderSize, inVromHeader->packedSz());
          if (sz == inVromHeader->fullSz)
          {
            outLen = FULL_VROM_HEADER_SIZE + extHeaderSize + sz;
          }
          else
          {
            logwarn("%s: (zstd) invalid size after decompression, %d != %u", __FUNCTION__, sz, inVromHeader->fullSz);
            isDecompressionOk = false;
          }
          OBFUSCATE_ZSTD_DATA(const_cast<char *>(in + VROM_HEADER_SIZE + extHeaderSize), inVromHeader->packedSz());
        }
        else
        {
          z_stream zlibStream;
          zlibStream.zalloc = Z_NULL;
          zlibStream.zfree = Z_NULL;
          zlibStream.opaque = Z_NULL;
          zlibStream.avail_in = 0;
          zlibStream.next_in = Z_NULL;
          inflateInit(&zlibStream);
          zlibStream.avail_in = inLen - VROM_HEADER_SIZE - extHeaderSize;
          zlibStream.next_in = (Bytef *)&in[VROM_HEADER_SIZE + extHeaderSize];
          zlibStream.avail_out = outLen - FULL_VROM_HEADER_SIZE - extHeaderSize;
          zlibStream.next_out = (Bytef *)&out[FULL_VROM_HEADER_SIZE + extHeaderSize];
          int r = inflate(&zlibStream, Z_FINISH);
          if (r == Z_STREAM_END)
          {
            outLen -= zlibStream.avail_out;
          }
          else
          {
            logwarn("%s: (zlib) inflate error, result: %d", __FUNCTION__, r);
            isDecompressionOk = false;
          }
          inflateEnd(&zlibStream);
        }
        const char *retVal = NULL;
        if (isDecompressionOk && outLen == inVromHeader->fullSz + FULL_VROM_HEADER_SIZE + extHeaderSize)
        {
          outVromHeader->label = inVromHeader->label;
          outVromHeader->target = inVromHeader->target;
          outVromHeader->fullSz = inVromHeader->fullSz;
          outVromHeader->hw32 = 0x80000000;
          if (extHeaderSize > 0)
            memcpy(&out[FULL_VROM_HEADER_SIZE], &in[VROM_HEADER_SIZE], extHeaderSize);
          memcpy(&out[outLen], &in[VROM_HEADER_SIZE + extHeaderSize + inVromHeader->packedSz()], MD5_HASH_SIZE);
          outLen += MD5_HASH_SIZE;
          memcpy(&out[outLen], &in[signatureOffset], signatureSize);
          outLen += signatureSize;
          VromfsCompression::InputDataType dataType = getDataType(&out[1], outLen - 1, CHECK_VERBOSE);
          if (dataType == IDT_VROM_PLAIN)
          {
            retVal = out;
          }
          else
          {
            logwarn("%s: decompressed vrom does not pass validation (%d)", __FUNCTION__, (int)dataType);
          }
        }
        return retVal;
      }
    case IDT_DATA:
      // decompress it with zlib
      G_ASSERT(inLen >= 1);
      if (in[0] != 'z')
      {
        logwarn("%s: is not a zlib packed blob", __FUNCTION__);
        return NULL;
      }
      return Compression::getInstanceById('z').decompress(&in[1], inLen - 1, out, outLen);
  }
  G_ASSERTF(0, "Should never get here (getDataType returned something unexpected?)");
  logwarn("%s: unexpected data type", __FUNCTION__);
  return NULL;
}


bool VromfsCompression::extractVersion(const char *data, int size, unsigned &version) const
{
  InputDataType dataType = getDataType(data, size, CHECK_LAZY);
  if (dataType != IDT_VROM_PLAIN && dataType != IDT_VROM_COMPRESSED)
    return false;

  G_ASSERT(size >= VROM_HEADER_SIZE);
  G_ASSERT(data != NULL);
  char reconstructedHeader[FULL_VROM_HEADER_SIZE];
  memcpy(&reconstructedHeader[1], data, VROM_HEADER_SIZE);
  reconstructedHeader[0] = 'V';
  const VirtualRomFsDataHdr *header = (const VirtualRomFsDataHdr *)reconstructedHeader;
  unsigned vrfs4C = header->label;
  if (header->label != VRFX_4C)
    return false;

  G_ASSERT(size >= VROM_HEADER_SIZE + sizeof(VirtualRomFsExtHdr));
  const VirtualRomFsExtHdr *extHdr = (const VirtualRomFsExtHdr *)&data[VROM_HEADER_SIZE];
  version = extHdr->version;
  return true;
}
