#if _TARGET_PC_WIN | _TARGET_SCARLETT
#define OODLE_IMPORT_LIB 1
#endif
#include <util/dag_globDef.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_oodleIo.h>
#include <oodle2.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_span.h>

#if OODLE2_VERSION_MAJOR >= 8
#define GET_COMPR_BUF_SZ_NEEDED(C, SZ) OodleLZ_GetCompressedBufferSizeNeeded(C, SZ)
#define GET_DEC_BUF_SZ(C, SZ, CORR)    OodleLZ_GetDecodeBufferSize(C, SZ, CORR)
#else
#define GET_COMPR_BUF_SZ_NEEDED(C, SZ) OodleLZ_GetCompressedBufferSizeNeeded(SZ)
#define GET_DEC_BUF_SZ(C, SZ, CORR)    OodleLZ_GetDecodeBufferSize(SZ, CORR)
#endif
#if OODLE2_VERSION_MAJOR < 9
typedef SINTa OO_SINTa;
typedef rrbool OO_BOOL;
#endif

enum
{
  DICT_SZ = 2 << 20,
  DECBUF_SZ = 1 << 20
};

size_t oodle_compress(void *dst, size_t maxDstSize, const void *src, size_t srcSize, int compressionLevel)
{
  SmallTab<char, TmpmemAlloc> cstor;
  OodleLZ_Compressor compressor = compressionLevel / 10 ? OodleLZ_Compressor_Leviathan : OodleLZ_Compressor_Kraken;
  OO_SINTa comp_buf_size = GET_COMPR_BUF_SZ_NEEDED(compressor, srcSize);
  if (comp_buf_size > maxDstSize)
    clear_and_resize(cstor, comp_buf_size);

  OodleLZ_CompressionLevel level = OodleLZ_CompressionLevel(OodleLZ_CompressionLevel_None + (compressionLevel % 10));
  OodleLZ_CompressOptions options = *OodleLZ_CompressOptions_GetDefault(compressor, level);
  if (srcSize > DICT_SZ + DECBUF_SZ)
    options.dictionarySize = DICT_SZ;
  OO_SINTa comp_len = OodleLZ_Compress(compressor, src, srcSize, cstor.size() ? cstor.data() : dst, level, &options);
  if (comp_len > maxDstSize)
    return 0;
  if (cstor.size())
    memcpy(dst, cstor.data(), comp_len);
  return comp_len;
}
size_t oodle_decompress(void *dst, size_t maxOriginalSize, const void *src, size_t compressedSize)
{
  return OodleLZ_Decompress(src, compressedSize, dst, maxOriginalSize, OodleLZ_FuzzSafe_Yes);
}

int oodle_compress_data(IGenSave &dest, IGenLoad &src, int sz, int compressionLevel)
{
  SmallTab<uint8_t, TmpmemAlloc> srcBuf, dstBuf;
  clear_and_resize(srcBuf, sz);
  clear_and_resize(dstBuf,
    GET_COMPR_BUF_SZ_NEEDED(compressionLevel / 10 ? OodleLZ_Compressor_Leviathan : OodleLZ_Compressor_Kraken, sz));
  src.read(srcBuf.data(), srcBuf.size());
  sz = oodle_compress(dstBuf.data(), dstBuf.size(), srcBuf.data(), srcBuf.size(), compressionLevel);
  if (!sz)
    return 0;
  dest.write(dstBuf.data(), sz);
  return sz;
}

int oodle_decompress_data(IGenSave &dest, IGenLoad &src, int compr_sz, int orig_sz)
{
  SmallTab<uint8_t, TmpmemAlloc> srcBuf, dstBuf;
  clear_and_resize(srcBuf, compr_sz);
  clear_and_resize(dstBuf, orig_sz);
  src.read(srcBuf.data(), srcBuf.size());
  orig_sz = oodle_decompress(dstBuf.data(), dstBuf.size(), srcBuf.data(), srcBuf.size());
  if (!orig_sz)
    return 0;
  dest.write(dstBuf.data(), orig_sz);
  return orig_sz;
}

static inline OodleLZDecoder *oodleDec(void *strm) { return reinterpret_cast<OodleLZDecoder *>(strm); }
inline bool OodleLoadCB::partialDecBuf() const { return decDataSize > DICT_SZ + DECBUF_SZ; }

void OodleLoadCB::open(IGenLoad &in_crd, int enc_size, int dec_size)
{
  del_it(encBuf);
  del_it(decBuf);
  G_ASSERT(!loadCb && "already opened?");
  G_ASSERT(enc_size > 0);
  loadCb = &in_crd;
  encDataLeft = enc_size;
  decDataSize = dec_size;
  encBufRdPos = encBufWrPos = 0;
  decBufPos = 0;
  rdPos = decDataOffset = 0;
  isFinished = false;
  strm = NULL;

  if (partialDecBuf())
    decBufSize = DICT_SZ + DECBUF_SZ;
  else
    decBufSize = GET_DEC_BUF_SZ(OodleLZ_Compressor_Invalid, decDataSize, false);
  encBufSize = OODLELZ_BLOCK_MAX_COMPLEN + (16 << 10);
  if (encBufSize > encDataLeft)
    encBufSize = encDataLeft;
}
void OodleLoadCB::close()
{
  if (strm && !isFinished && !encDataLeft && encBufRdPos >= encBufWrPos)
    ceaseReading();

  G_ASSERT(isFinished || !strm);
  if (strm)
    OodleLZDecoder_Destroy(oodleDec(strm));
  loadCb = NULL;
  encDataLeft = 0;
  isFinished = false;
  strm = NULL;
  del_it(encBuf);
  del_it(decBuf);
}


inline void OodleLoadCB::preloadEncData(int size)
{
  static constexpr int RD_ALIGN_MASK = (16 << 10) - 1;
  size += RD_ALIGN_MASK + 1;
  if (encBufWrPos - encBufRdPos >= size)
    return;
  if (encBufRdPos && encBufRdPos + size > encBufSize)
  {
    memmove(encBuf, encBuf + encBufRdPos, encBufWrPos - encBufRdPos);
    encBufWrPos -= encBufRdPos;
    encBufRdPos = 0;
  }

  int rd = (size - (encBufWrPos - encBufRdPos) + RD_ALIGN_MASK) & ~RD_ALIGN_MASK;
  if (rd > encBufSize - encBufWrPos)
    rd = encBufSize - encBufWrPos;
  if (rd > encDataLeft)
    rd = encDataLeft;
  rd = loadCb->tryRead(encBuf + encBufWrPos, rd);
  encBufWrPos += rd;
  encDataLeft -= rd;
}
inline int OodleLoadCB::decodeData(int size)
{
  if (!size || rdPos >= decDataSize)
    return 0;
  if (rdPos + size <= decDataOffset + decBufPos)
    return size;
  if (rdPos + size > decDataSize)
    size = decDataSize - rdPos;
  if (partialDecBuf())
  {
    if (rdPos >= decDataOffset + DECBUF_SZ && decBufPos == decBufSize)
    {
      memmove(decBuf, decBuf + DECBUF_SZ, DICT_SZ);
      decDataOffset += DECBUF_SZ;
      decBufPos = DICT_SZ;
    }

    if (rdPos + size > decDataOffset + decBufSize)
      size = decDataOffset + decBufSize - rdPos;
  }

  G_ASSERT(!isFinished);

  if (!strm)
  {
    strm = OodleLZDecoder_Create(OodleLZ_Compressor_Invalid, decDataSize, NULL, 0);
    encBuf = (char *)memalloc(encBufSize, tmpmem);
    decBuf = (char *)memalloc(decBufSize, tmpmem);
  }

  preloadEncData(0);
  while (decDataOffset + decBufPos < rdPos + size)
  {
    OodleLZ_DecodeSome_Out out;

    OO_BOOL ok = OodleLZDecoder_DecodeSome(oodleDec(strm), &out, decBuf, decBufPos, decDataSize, decBufSize - decBufPos,
      encBuf + encBufRdPos, encBufWrPos - encBufRdPos);

    if (!ok)
    {
      int pos = loadCb->tell();
      G_UNUSED(pos);
      logerr("Oodle dec error: file(%s) at ~0x%08X...0x%08X, last decoded data size=0x%08X\n"
             "dec=(%p, 0x%08X, 0x%08X) enc(%p, 0x%08X, 0x%08X) decoded=%d consumed=%d encDataLeft=%d",
        loadCb->getTargetName(), pos - (encBufWrPos - encBufRdPos), pos, decDataOffset + decBufPos - rdPos, decBuf, decBufPos,
        decBufSize - decBufPos, encBuf, encBufRdPos, encBufWrPos - encBufRdPos, out.decodedCount, out.compBufUsed, encDataLeft);
      return 0;
    }

    decBufPos += out.decodedCount;
    encBufRdPos += out.compBufUsed;

    if (encDataLeft && (decDataOffset + decBufPos < rdPos + size))
      preloadEncData(out.curQuantumCompLen);
  }
  return size;
}

int OodleLoadCB::tryRead(void *ptr, int size)
{
  int read_sz = 0;
  while (read_sz < size)
  {
    int rd_sz = decodeData(size - read_sz);
    if (!rd_sz)
      break;
    memcpy(ptr, decBuf + rdPos - decDataOffset, rd_sz);
    rdPos += rd_sz;
    ptr = (char *)ptr + rd_sz;
    read_sz += rd_sz;
  }
  return read_sz;
}

void OodleLoadCB::read(void *ptr, int size)
{
  if (tryRead(ptr, size) != size)
  {
    isFinished = true;
    DAGOR_THROW(LoadException("Oodle read error", -1));
  }
}
void OodleLoadCB::seekrel(int ofs)
{
  if (ofs < 0)
    issueFatal();
  else
    while (ofs > 0)
    {
      int rd_sz = decodeData(ofs);
      rdPos += rd_sz;
      ofs -= rd_sz;
    }
}
bool OodleLoadCB::ceaseReading()
{
  if (isFinished || !strm)
    return true;

  loadCb->seekrel(encDataLeft);
  OodleLZDecoder_Destroy(oodleDec(strm));
  strm = NULL;
  isFinished = true;
  return true;
}

#define EXPORT_PULL dll_pull_iosys_oodleIo
#include <supp/exportPull.h>
