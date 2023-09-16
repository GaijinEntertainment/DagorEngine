#include <ioSys/dag_lzmaIo.h>
#include <arc/lzma-9.20/LzmaDec.h>

static void *SzAlloc(void *, size_t size) { return memalloc(size, tmpmem); }
static void SzFree(void *, void *address) { memfree(address, tmpmem); }
static ISzAlloc g_Alloc = {SzAlloc, SzFree};

static CLzmaDec *lzmaDec(void *strm) { return reinterpret_cast<CLzmaDec *>(strm); }


void LzmaLoadCB::open(IGenLoad &in_crd, int in_size)
{
  G_STATIC_ASSERT(SIZE_OF_LZMA_DEC >= sizeof(CLzmaDec));
  G_ASSERT(!loadCb && "already opened?");
  G_ASSERT(in_size > 0);
  loadCb = &in_crd;
  inBufLeft = in_size;
  isStarted = false;
  isFinished = false;
  rdBufAvail = rdBufPos = 0;
  memset(rdBuf, 0, sizeof(rdBuf));

  LzmaDec_Construct(lzmaDec(strm));
}
void LzmaLoadCB::close()
{
  if (isStarted && !isFinished && !inBufLeft && rdBufPos >= rdBufAvail)
    ceaseReading();

  G_ASSERT(isFinished || !isStarted);
  if (isStarted)
    LzmaDec_Free(lzmaDec(strm), &g_Alloc);
  loadCb = NULL;
  inBufLeft = 0;
  isStarted = false;
  isFinished = false;
}


inline int LzmaLoadCB::tryReadImpl(void *ptr, int _size)
{
  if (!_size)
    return 0;
  size_t size = _size;

  G_ASSERT(!isFinished);

  if (!isStarted)
  {
    unsigned char header[LZMA_PROPS_SIZE];
    loadCb->read(header, sizeof(header));
    inBufLeft -= sizeof(header);
    SRes err = LzmaDec_Allocate(lzmaDec(strm), header, LZMA_PROPS_SIZE, &g_Alloc);
    LzmaDec_Init(lzmaDec(strm));

    if (err != SZ_OK)
    {
      fatal("7zip error %d in %s\nsource: '%s'\n", err, "LzmaDec_Allocate", getTargetName());
#if _TARGET_PC
      RETURN_X_AFTER_FATAL(0);
#endif
    }
    isStarted = true;
  }

  Byte *out_p = (Byte *)ptr;
  while (size > 0)
  {
    if (rdBufPos >= rdBufAvail)
    {
      rdBufPos = 0;
      rdBufAvail = loadCb->tryRead(rdBuf, int(inBufLeft > sizeof(rdBuf) ? sizeof(rdBuf) : inBufLeft));
      inBufLeft -= rdBufAvail;
    }
    SizeT outProcessed = size, inProcessed = rdBufAvail - rdBufPos;
    ELzmaStatus status = LZMA_STATUS_NOT_SPECIFIED;
    SRes res = LzmaDec_DecodeToBuf(lzmaDec(strm), out_p, &outProcessed, rdBuf + rdBufPos, &inProcessed, LZMA_FINISH_ANY, &status);
    if (res != SZ_OK)
    {
      outProcessed = inProcessed = 0;
      fatal("7zip error %d status %d in %s\nsource: '%s'\n", res, status, "LzmaDec_DecodeToBuf", getTargetName());
#if _TARGET_PC
      RETURN_X_AFTER_FATAL(0);
#endif
    }
    rdBufPos += inProcessed;
    out_p += outProcessed;
    size -= outProcessed;

    if (!inBufLeft && rdBufPos >= rdBufAvail)
      break;
  }

  return out_p - (Byte *)ptr;
}

int LzmaLoadCB::tryRead(void *ptr, int size) { return LzmaLoadCB::tryReadImpl(ptr, size); }

void LzmaLoadCB::read(void *ptr, int size)
{
  int rd_sz = LzmaLoadCB::tryReadImpl(ptr, size);
  while (rd_sz && rd_sz < size)
  {
    ptr = (char *)ptr + rd_sz;
    size -= rd_sz;
    rd_sz = LzmaLoadCB::tryReadImpl(ptr, size);
  }

  if (rd_sz != size)
  {
    isFinished = true;
    DAGOR_THROW(LoadException("7zip read error", -1));
  }
}
void LzmaLoadCB::seekrel(int ofs)
{
  if (ofs < 0)
    issueFatal();
  else
    while (ofs > 0)
    {
      char buf[4096];
      int sz = ofs > sizeof(buf) ? sizeof(buf) : ofs;
      read(buf, sz);
      ofs -= sz;
    }
}
bool LzmaLoadCB::ceaseReading()
{
  if (isFinished || !isStarted)
    return true;

  loadCb->seekrel(int(inBufLeft > 0x70000000 ? rdBufPos - rdBufAvail : inBufLeft));
  rdBufAvail = rdBufPos = 0;
  LzmaDec_Free(lzmaDec(strm), &g_Alloc);

  isFinished = true;
  isStarted = false;
  return true;
}

void BufferedLzmaLoadCB::read(void *ptr, int size)
{
  int rd = tryRead(ptr, size);
  if (rd != size)
  {
    logerr("BufferedLzmaLoadCB::read(%p, %d)=%d totalOut=%d curPos=%d", ptr, size, rd, totalOut, curPos);
    DAGOR_THROW(LoadException("7zip read error", -1));
  }
}
int BufferedLzmaLoadCB::tryRead(void *_ptr, int size)
{
  char *ptr = (char *)_ptr;
  if (curPos + size <= totalOut)
  {
    memcpy(ptr, outBuf + curPos, size);
    curPos += size;
    return size;
  }

  if (totalOut - curPos && totalOut - curPos < size)
  {
    memcpy(ptr, outBuf + curPos, totalOut - curPos);
    ptr += totalOut - curPos;
    size -= totalOut - curPos;
  }

  if (size > OUT_BUF_SZ / 2)
  {
    while (size > 0)
    {
      totalOut = LzmaLoadCB::tryReadImpl(outBuf, size >= OUT_BUF_SZ ? OUT_BUF_SZ : size);
      memcpy(ptr, outBuf, totalOut);
      ptr += totalOut;
      size -= totalOut;
      if (!totalOut)
        break;
    }

    totalOut = 0;
    curPos = 0;
  }
  else
  {
    totalOut = LzmaLoadCB::tryReadImpl(outBuf, OUT_BUF_SZ);
    G_ASSERT(totalOut >= size);
    memcpy(ptr, outBuf, size);
    ptr += size;
    curPos = size;
  }
  return ptr - (char *)_ptr;
}

#define EXPORT_PULL dll_pull_iosys_lzmaDecIo
#include <supp/exportPull.h>
