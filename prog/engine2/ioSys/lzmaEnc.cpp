#include <ioSys/dag_lzmaIo.h>
#include <arc/lzma-9.20/LzmaEnc.h>

static void *SzAlloc(void *, size_t size) { return memalloc(size, tmpmem); }
static void SzFree(void *, void *address) { memfree(address, tmpmem); }
static ISzAlloc g_Alloc = {SzAlloc, SzFree};


struct DagSeqInStream : public ISeqInStream
{
  DagSeqInStream(IGenLoad *_crd, int sz)
  {
    crd = _crd;
    Read = readCrd;
    dataSz = sz;
  }
  static SRes readCrd(void *p, void *buf, size_t *size)
  {
    int &dataSz = reinterpret_cast<DagSeqInStream *>(p)->dataSz;
    *size = reinterpret_cast<DagSeqInStream *>(p)->crd->tryRead(buf, int((*size > dataSz) ? dataSz : *size));
    dataSz -= int(*size);
    return SZ_OK;
  }
  IGenLoad *crd;
  int dataSz;
};
struct DagSeqOutStream : public ISeqOutStream
{
  DagSeqOutStream(IGenSave *_cwr)
  {
    cwr = _cwr;
    Write = writeCwr;
    sumSz = 0;
  }
  static size_t writeCwr(void *p, const void *buf, size_t size)
  {
    reinterpret_cast<DagSeqOutStream *>(p)->cwr->write(buf, int(size));
    reinterpret_cast<DagSeqOutStream *>(p)->sumSz += int(size);
    return size;
  }
  IGenSave *cwr;
  int sumSz;
};

int lzma_compress_data(IGenSave &dest, int compression_level, IGenLoad &src, int sz, int dict_sz)
{
  CLzmaEncHandle enc = LzmaEnc_Create(&g_Alloc);

  CLzmaEncProps props;
  LzmaEncProps_Init(&props);
  props.level = compression_level;
  props.dictSize = dict_sz;

  SRes res = LzmaEnc_SetProps(enc, &props);

  if (res != SZ_OK)
  {
    LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
    logerr("7zip error %d in %s\nsource: '%s'\n", res, "LzmaEnc_SetProps", dest.getTargetName());
    return -1;
  }

  Byte header[LZMA_PROPS_SIZE];
  size_t headerSize = LZMA_PROPS_SIZE;

  res = LzmaEnc_WriteProperties(enc, header, &headerSize);
  if (res != SZ_OK)
  {
    LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
    enc = NULL;
    logerr("7zip error %d in %s\nsource: '%s'\n", res, "LzmaEnc_WriteProperties", dest.getTargetName());
    return -1;
  }

  int st_ofs = dest.tell();
  dest.write(header, int(headerSize));
  DagSeqInStream inStrm(&src, sz);
  DagSeqOutStream outStrm(&dest);

  res = LzmaEnc_Encode(enc, &outStrm, &inStrm, NULL, &g_Alloc, &g_Alloc);
  LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);
  if (res != SZ_OK)
    logerr("7zip error %d in %s\nsource: '%s'\n", res, "LzmaEnc_Encode", dest.getTargetName());

  return dest.tell() - st_ofs;
}


#define EXPORT_PULL dll_pull_iosys_lzmaEnc
#include <supp/exportPull.h>
