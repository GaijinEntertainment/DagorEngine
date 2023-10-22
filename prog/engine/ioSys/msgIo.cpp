#include <ioSys/dag_msgIo.h>


//
// Mmo message i/o dispatcher
//
ThreadSafeMsgIo::ThreadSafeMsgIo(int buf_sz)
{
  bufferSize = buf_sz;
  buffer = memalloc(buf_sz, midmem);
  availWrSize = bufferSize;
  wrPos = 0;
  rdPos = 0;
  msgCount = 0;

  rdMsgCount = 0;

  cwr.setCircularBuffer(buffer, bufferSize);
  crd.setCircularBuffer(buffer, bufferSize);
  crd.setLimits(rdPos, rdPos);
  cwr.setLimits(wrPos, wrPos);
}
ThreadSafeMsgIo::~ThreadSafeMsgIo()
{
  cwr.setCircularBuffer(NULL, 0);
  crd.setCircularBuffer(NULL, 0);
  if (buffer)
    memfree(buffer, midmem);
}

IGenLoad *ThreadSafeMsgIo::startRead(int &msg_count)
{
  if (rdMsgCount)
    return NULL;

  cc.lock();
  msg_count = msgCount;
  if (!msg_count)
  {
    cc.unlock();
    return NULL;
  }
  rdMsgCount = msg_count;
  crd.setLimits(rdPos, wrPos);
  cc.unlock();
  return &crd;
}
void ThreadSafeMsgIo::endRead()
{
  while (crd.getBlockLevel())
    crd.endBlock();

  cc.lock();
  msgCount -= rdMsgCount;
  rdMsgCount = 0;
  interlocked_add(availWrSize, crd.getLimSize());
  rdPos = crd.getEndPos();
  crd.setLimits(rdPos, rdPos);
  cc.unlock();
}

IGenSave *ThreadSafeMsgIo::startWrite()
{
  if (cwr.getEndPos() != wrPos)
    return NULL;

  cc.lock();
  cwr.setLimits(wrPos, (rdPos - 8 + bufferSize) % bufferSize);
  interlocked_add(availWrSize, -cwr.getLimSize());
  cc.unlock();
  return &cwr;
}
void ThreadSafeMsgIo::endWrite()
{
  while (cwr.getBlockLevel())
    cwr.endBlock();

  cc.lock();
  if (cwr.getRootBlockCount())
  {
    msgCount += cwr.getRootBlockCount();
    wrPos = (wrPos + cwr.tell()) % bufferSize;
  }
  interlocked_add(availWrSize, cwr.getLimSize() - cwr.tell());
  cwr.setLimits(wrPos, wrPos);
  cc.unlock();
}


// simple circular buffer writer with block support (no mem allocations)
SimpleBlockSave::SimpleBlockSave()
{
  buffer = NULL;
  bufferSize = 0;
  limSize = 0;
  blkUsed = 0;
  memset(blkOfs, 0, sizeof(blkOfs));
  limStart = 0;
  limEnd = 0;
  curPos = 0;
  rootBlkNum = 0;
}

void SimpleBlockSave::setCircularBuffer(void *mem, int len)
{
  buffer = (char *)mem;
  bufferSize = len;
}
void SimpleBlockSave::setLimits(int start_pos, int end_pos)
{
  limStart = start_pos;
  limEnd = end_pos;
  curPos = limStart;
  limSize = limEnd - limStart;
  if (limSize < 0)
    limSize += bufferSize;
  rootBlkNum = 0;
}

void SimpleBlockSave::beginBlock()
{
  if ((unsigned)blkUsed >= BLOCK_MAX)
    DAGOR_THROW(SaveException("too much nested blocks", tell()));
  G_ANALYSIS_ASSUME((unsigned)blkUsed < BLOCK_MAX);
  writeIntP<2>(0);
  blkOfs[blkUsed] = tell();
  blkUsed++;
}
void SimpleBlockSave::endBlock(unsigned block_flags)
{
  G_UNUSED(block_flags);
  G_ASSERTF(block_flags == 0, "block_flags=%08X", block_flags); // 2 bits max
  if (blkUsed <= 0)
    DAGOR_THROW(SaveException("block not begun", tell()));
  G_ANALYSIS_ASSUME((unsigned)blkUsed > 0);
  int o = tell();
  G_ASSERTF(o >= blkOfs[blkUsed - 1] && !((o - blkOfs[blkUsed - 1]) & 0xFFFF0000), "o=%08X blkOfs[]=%08X len=%08X", o,
    blkOfs[blkUsed - 1], o - blkOfs[blkUsed - 1]);
  seekto(blkOfs[blkUsed - 1] - sizeof(short));
  writeIntP<2>(o - blkOfs[blkUsed - 1]);
  seekto(o);
  blkUsed--;

  if (blkUsed == 0)
    rootBlkNum++;
}
int SimpleBlockSave::getBlockLevel() { return blkUsed; }

void SimpleBlockSave::write(const void *ptr, int size)
{
  int sz = SimpleBlockSave::tell() + size;
  if (sz > limSize)
    DAGOR_THROW(SaveException("cant write full packet", tell()));

  if (curPos < limStart || curPos + size < bufferSize)
  {
    memcpy(buffer + curPos, ptr, size);
    curPos += size;
  }
  else
  {
    char *_ptr = ((char *)ptr) + bufferSize - curPos;
    memcpy(buffer + curPos, ptr, bufferSize - curPos);
    size -= bufferSize - curPos;
    memcpy(buffer, _ptr, size);
    curPos = size;
  }
}
int SimpleBlockSave::tell()
{
  if (curPos >= limStart)
    return curPos - limStart;
  return curPos + bufferSize - limStart;
}
void SimpleBlockSave::seekto(int abs_ofs)
{
  if (abs_ofs < 0 || abs_ofs > limSize)
    DAGOR_THROW(SaveException("seek out of range", tell()));

  if (abs_ofs + limStart < bufferSize)
    curPos = abs_ofs + limStart;
  else
    curPos = abs_ofs + limStart - bufferSize;
}
void SimpleBlockSave::seektoend(int) { DAGOR_THROW(SaveException("seek to end not supported", tell())); }


// simple circular buffer reader with block support (no mem allocations)
SimpleBlockLoad::SimpleBlockLoad()
{
  buffer = NULL;
  bufferSize = 0;
  limSize = 0;
  blkUsed = 0;
  memset(blkOfs, 0, sizeof(blkOfs));
  memset(blkLen, 0, sizeof(blkLen));
  limStart = 0;
  limEnd = 0;
  curPos = 0;
}

void SimpleBlockLoad::setCircularBuffer(const void *mem, int len)
{
  buffer = (const char *)mem;
  bufferSize = len;
}
void SimpleBlockLoad::setLimits(int start_pos, int end_pos)
{
  limStart = start_pos;
  limEnd = end_pos;
  curPos = limStart;
  limSize = limEnd - limStart;
  if (limSize < 0)
    limSize += bufferSize;
}

int SimpleBlockLoad::beginBlock(unsigned *out_block_flags)
{
  if (blkUsed >= BLOCK_MAX)
    DAGOR_THROW(LoadException("too much nested blocks", tell()));
  G_ANALYSIS_ASSUME((unsigned)blkUsed < BLOCK_MAX);

  int l = readIntP<2>();
  if (out_block_flags)
    *out_block_flags = 0;

  blkOfs[blkUsed] = tell();
  blkLen[blkUsed] = l;
  blkUsed++;

  return l;
}
void SimpleBlockLoad::endBlock()
{
  if (blkUsed <= 0)
    DAGOR_THROW(LoadException("endBlock without beginBlock", tell()));
  G_ANALYSIS_ASSUME(blkUsed > 0);
  seekto(blkOfs[blkUsed - 1] + blkLen[blkUsed - 1]);
  blkUsed--;
}
int SimpleBlockLoad::getBlockLength()
{
  if (blkUsed <= 0)
    DAGOR_THROW(LoadException("block not begun", tell()));
  G_ANALYSIS_ASSUME(blkUsed > 0);
  return blkLen[blkUsed - 1];
}
int SimpleBlockLoad::getBlockRest()
{
  if (blkUsed <= 0)
    DAGOR_THROW(LoadException("block not begun", tell()));
  G_ANALYSIS_ASSUME(blkUsed > 0);
  return blkOfs[blkUsed - 1] + blkLen[blkUsed - 1] - tell();
}
int SimpleBlockLoad::getBlockLevel() { return blkUsed; }

void SimpleBlockLoad::read(void *ptr, int size)
{
  if (SimpleBlockLoad::tryRead(ptr, size) != size)
    DAGOR_THROW(LoadException("read error", tell()));
}
int SimpleBlockLoad::tryRead(void *ptr, int size)
{
  int sz = SimpleBlockLoad::tell() + size;
  if (sz > limSize)
    size -= sz - limSize;

  if (size <= 0)
    return 0;

  if (curPos < limStart || curPos + size < bufferSize)
  {
    memcpy(ptr, buffer + curPos, size);
    curPos += size;
    return size;
  }
  else
  {
    char *_ptr = ((char *)ptr) + bufferSize - curPos;
    sz = size;
    memcpy(ptr, buffer + curPos, bufferSize - curPos);
    size -= bufferSize - curPos;
    memcpy(_ptr, buffer, size);
    curPos = size;
    return sz;
  }
}
int SimpleBlockLoad::tell()
{
  if (curPos >= limStart)
    return curPos - limStart;
  return curPos + bufferSize - limStart;
}
void SimpleBlockLoad::seekto(int abs_ofs)
{
  if (abs_ofs < 0 || abs_ofs > limSize)
    DAGOR_THROW(LoadException("seek out of range", tell()));

  if (abs_ofs + limStart < bufferSize)
    curPos = abs_ofs + limStart;
  else
    curPos = abs_ofs + limStart - bufferSize;
}
void SimpleBlockLoad::seekrel(int rel_ofs) { SimpleBlockLoad::seekto(SimpleBlockLoad::tell() + rel_ofs); }

#define EXPORT_PULL dll_pull_iosys_msgIo
#include <supp/exportPull.h>
