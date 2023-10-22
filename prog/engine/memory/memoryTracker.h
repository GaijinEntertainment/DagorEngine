#pragma once

#include <generic/dag_carray.h>
#include <math/dag_mathBase.h>
#include <math/dag_adjpow2.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_fileIoErr.h>
#include <osApiWrappers/dag_vromfs.h>
#include <stdio.h>
#include <string.h>
#include <osApiWrappers/dag_files.h>
#include <startup/dag_globalSettings.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/vector_set.h>
#include <generic/dag_qsort.h>
#include <string.h>

#define MTR_MAX_UNWIND_DEPTH 50
// check dagmem.cpp real_init_memory()
#if MEM_DEBUGALLOC > 0
#define MTR_MAX_CPU_HEAPS 9
#else
#define MTR_MAX_CPU_HEAPS 8
#endif

#if _TARGET_C1 | _TARGET_C2

#else
#define MTR_MAX_HEAPS MTR_MAX_CPU_HEAPS
#endif

#define BLK_POW2_LOW      4U
#define BLK_POW2_HIGH     24U
#define BLK_POW2_ELEMENTS (BLK_POW2_HIGH - BLK_POW2_LOW)

#define MTR_VERSION 0x3300

typedef uint32_t bitmap_t;
#define BITMAP_BITS    (sizeof(bitmap_t) * 8)
#define TRACE_BIN_SIZE 1024

//- configure hash bins -
#define HB_SZ_64K 1024
#define HB_SZ_4K  128
#define HB_SZ_256 64
#define HB_SZ_64  16
#define HB_SZ_8   1

#define HB_00 HB_SZ_64K
#define HB_01 HB_SZ_64K
#define HB_02 HB_SZ_64K
#define HB_03 HB_SZ_64K
#define HB_04 HB_SZ_64K
#define HB_05 HB_SZ_64K
#define HB_06 HB_SZ_4K
#define HB_07 HB_SZ_4K
#define HB_08 HB_SZ_4K
#define HB_09 HB_SZ_256
#define HB_10 HB_SZ_256
#define HB_11 HB_SZ_64
#define HB_12 HB_SZ_64
#define HB_13 HB_SZ_64
#define HB_14 HB_SZ_64
#define HB_15 HB_SZ_64
#define HB_16 HB_SZ_8
#define HB_17 HB_SZ_8
#define HB_18 HB_SZ_8
#define HB_19 HB_SZ_8
//----------------------

// reference label in gamelibs, to force linking
extern bool mtrk_console_handler(const char *argv[], int argc);

namespace memorytracker
{
static const uint16_t hash_bin_size[BLK_POW2_ELEMENTS] = {HB_00, HB_01, HB_02, HB_03, HB_04, HB_05, HB_06, HB_07, HB_08, HB_09, HB_10,
  HB_11, HB_12, HB_13, HB_14, HB_15, HB_16, HB_17, HB_18, HB_19};

static const uint16_t hash_bin_offset[BLK_POW2_ELEMENTS] = {
  0,
  HB_00,
  HB_00 + HB_01,
  HB_00 + HB_01 + HB_02,
  HB_00 + HB_01 + HB_02 + HB_03,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08 + HB_09,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08 + HB_09 + HB_10,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08 + HB_09 + HB_10 + HB_11,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08 + HB_09 + HB_10 + HB_11 + HB_12,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08 + HB_09 + HB_10 + HB_11 + HB_12 + HB_13,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08 + HB_09 + HB_10 + HB_11 + HB_12 + HB_13 + HB_14,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08 + HB_09 + HB_10 + HB_11 + HB_12 + HB_13 + HB_14 + HB_15,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08 + HB_09 + HB_10 + HB_11 + HB_12 + HB_13 + HB_14 + HB_15 +
    HB_16,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08 + HB_09 + HB_10 + HB_11 + HB_12 + HB_13 + HB_14 + HB_15 +
    HB_16 + HB_17,
  HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08 + HB_09 + HB_10 + HB_11 + HB_12 + HB_13 + HB_14 + HB_15 +
    HB_16 + HB_17 + HB_18,
};

#define HB_SIZE_TOTAL                                                                                                              \
  (HB_00 + HB_01 + HB_02 + HB_03 + HB_04 + HB_05 + HB_06 + HB_07 + HB_08 + HB_09 + HB_10 + HB_11 + HB_12 + HB_13 + HB_14 + HB_15 + \
    HB_16 + HB_17 + HB_18 + HB_19)


#if (_TARGET_C1 || _TARGET_C2) && (DAGOR_DBGLEVEL > 0)








































#endif

struct MemBlock
{
  void *addr;
  // void *backtrace;
  uint32_t size;
  uint32_t nextIndex; // next index with same hash
  // only used with MTR_COLLECT_BACKTRACE
  uint32_t bktraceID; // is offset in trace heap (first element is size:8|next:24)
  uint32_t timestamp; // store time

  static constexpr int STOP_INDEX = 0; // block #0 is preallocated
};

struct StackTrace
{
  uint32_t nextIndex;
  uint32_t refcount;
  uintptr_t baseAddr;
};

struct BlockHeader
{
  uint32_t count;
  size_t allocated;
};

struct HeapHeader
{
  IMemAlloc *heapID; // only used as ID
  size_t allocated;
  size_t count;
  char name[16]; // name is copied here during the dump
  BlockHeader blockHeaders[BLK_POW2_ELEMENTS];

  bool isGpuHeap() { return name[0] == '*'; }
};

struct HeapName
{
  IMemAlloc *heapID;
  char name[16];
};

struct DumpHeader
{
  uint16_t version; // 0x33xn
  uint8_t ptrSize;  // 4/8
  uint8_t pad;
  int32_t numHeaps;
  int32_t numBlocks;
  int32_t numBtElems;
  int32_t maxBlocks;
  int32_t maxBtElems;
  uint32_t firstBitIdx;
  uint32_t savedBitIdx;

  DumpHeader() :
    version(MTR_VERSION),
    ptrSize(sizeof(void *)),
    pad(0),
    numHeaps(0),
    numBlocks(0),
    numBtElems(0),
    maxBlocks(0),
    maxBtElems(0),
    firstBitIdx(0),
    savedBitIdx(0)
  {}
};

// follows:
// bitmap

struct MemoryTrackerNull
{
  void registerHeap(IMemAlloc * /*heap*/, const char * /*name*/){};
  void init(int /*num_memory_blocks*/, int /*back_trace_elements*/, int /*dump_size_percent*/) {}
  void addBlock(IMemAlloc * /*heap*/, void * /*addr*/, size_t /*size*/, bool user_heap = false) { (void)user_heap; }
  bool removeBlock(IMemAlloc * /*heap*/, void * /*addr*/, size_t user_size = 0, bool anywhere = false)
  {
    (void)user_size;
    (void)anywhere;
    return false;
  }
  bool isAddrValid(void * /*addr*/, size_t /*size*/);
  void saveDump(const char * /*filename*/) {}
  void printStats();
};

struct MemoryTracker
{
  carray<IMemAlloc *, MTR_MAX_HEAPS> heapIDs; // for fast ID search
  carray<HeapHeader, MTR_MAX_HEAPS> heaps;
  carray<HeapName, MTR_MAX_HEAPS> heapNames; // track heap names at startup

  bool requireInit;
  uint8_t dumpSizePercent;
  uint8_t dummy;
  int numHeaps;
  int maxBlocks;
  int maxBtElems;
  int currentBtIndex;
  int savedBtIndex;
  size_t dumpSize;
  size_t dumpSizePacked;

  uint32_t firstNonFullBitmapChunk;

  bitmap_t *bitmap;
  MemBlock *blocks;
  uintptr_t *btElems;
  uint8_t *dumpBase;

  CritSecStorage mtlock;

  uintptr_t stackUnwind[MTR_MAX_UNWIND_DEPTH];

  // hashbin contains blockidx
  uint32_t hashbins[MTR_MAX_HEAPS][HB_SIZE_TOTAL];
  uint32_t tracebins[TRACE_BIN_SIZE];

  // class is initialized explicitly before this constructor
  MemoryTracker() {} //-V730
  ~MemoryTracker()
  {
    free(btElems);
    free(bitmap);
    free(blocks);
    if (!requireInit)
      destroy_critical_section(mtlock);
  }

  void lock() { enter_critical_section(mtlock, "mtr"); }
  void unlock() { leave_critical_section(mtlock); }

  void clearAll()
  {
    numHeaps = 0;
    firstNonFullBitmapChunk = 0;

    for (int heapIdx = 0; heapIdx < MTR_MAX_HEAPS; heapIdx++)
    {
      heapIDs[heapIdx] = 0;

      HeapHeader &hh = heaps[heapIdx];
      hh.heapID = NULL;
      hh.allocated = 0;
      hh.count = 0;

      for (size_t i = 0; i < BLK_POW2_ELEMENTS; i++)
      {
        BlockHeader &bh = hh.blockHeaders[i];
        bh.allocated = 0;
        bh.count = 0;
      }

      for (size_t i = 0; i < HB_SIZE_TOTAL; i++)
        hashbins[heapIdx][i] = MemBlock::STOP_INDEX;

      for (size_t i = 0; i < TRACE_BIN_SIZE; i++)
        tracebins[i] = MemBlock::STOP_INDEX;
    }

    bitmap[0] = 1; // mark block #0
  }

  static uint32_t addr2hash(void *p)
  {
    uintptr_t v = (uintptr_t)p;
    return (((v >> 16) ^ v) >> 8) ^ v;
  }

  static uint32_t addr2hash_offset(void *addr, uint32_t size_idx)
  {
    uint32_t hbSize = hash_bin_size[size_idx];
    uint32_t hashIdx = addr2hash(addr) & (hbSize - 1);
    return hash_bin_offset[size_idx] + hashIdx;
  }

  // getters for external code
  uint32_t get_hash_bin_size(uint32_t bin_no) { return hash_bin_size[bin_no]; }
  uint32_t get_hash_bin_offset(uint32_t bin_no) { return hash_bin_offset[bin_no]; }

  uint32_t get_first_one(bitmap_t v)
  {
    if (v == 0)
      return BITMAP_BITS - 1;
    return get_log2i(~v);

    // return get_log2i((v & (1U<<(BITMAP_BITS-1))) ? ~v : v);
  }

  int findHeapIndex(IMemAlloc *heap) const
  {
    int heapIdx;
    for (heapIdx = 0; (heapIdx < numHeaps) && (heapIDs[heapIdx] != heap); heapIdx++)
      ;
    return heapIdx;
  }

  uint32_t indexBySize(size_t size) const
  {
    if (size == 0)
      return BLK_POW2_LOW;
    G_ASSERT(size <= 0xFFFFFFFF);
    uint32_t pow2 = get_log2i(uint32_t(size));
    return min(max(pow2, BLK_POW2_LOW), BLK_POW2_HIGH - 1) - BLK_POW2_LOW;
  }

  uintptr_t packBtHdr(size_t stack_depth, uintptr_t next) { return (stack_depth << 24) | next; }
  size_t unpackBtHdrDepth(uintptr_t bt_entry) { return (bt_entry >> 24) & 0xff; }
  size_t unpackBtHdrNext(uintptr_t bt_entry) { return bt_entry & 0xffffff; }

  // returns 0 if no space`
  uint32_t addTrace()
  {
    // get back trace hash
    uintptr_t hash = 0;
    size_t stackDepth = 0;
    while (stackDepth < MTR_MAX_UNWIND_DEPTH && stackUnwind[stackDepth] != ~((uintptr_t)0))
      hash = (hash >> 1) ^ stackUnwind[stackDepth++];
    hash = ((((hash >> 12) ^ hash) >> 4) ^ hash) & TRACE_BIN_SIZE - 1;

    uint32_t btIdx;
    for (btIdx = tracebins[hash]; btIdx != MemBlock::STOP_INDEX; btIdx = unpackBtHdrNext(btElems[btIdx]))
    {
      if (unpackBtHdrDepth(btElems[btIdx]) != stackDepth)
        continue;

      size_t i = 0;
      for (; i < stackDepth; i++)
        if (btElems[btIdx + 1 + i] != stackUnwind[i])
          break;
      if (i != stackDepth)
        continue;
      return btIdx;
    };

    // add if have space
    if ((currentBtIndex + stackDepth + 1) < maxBtElems)
    {
      memcpy(&btElems[currentBtIndex + 1], stackUnwind, stackDepth * sizeof(uintptr_t));

      btElems[currentBtIndex] = packBtHdr(stackDepth, tracebins[hash]);
      tracebins[hash] = (uint32_t)currentBtIndex;

      currentBtIndex += stackDepth + 1;
      return currentBtIndex - stackDepth - 1;
    }

    // no space
    return 0;
  }
#if INSIDE_ENGINE
  // called after malloc
  void addBlock(IMemAlloc *heap, void *addr, size_t size, bool user_heap = false)
  {
    if (addr == 0)
      return;

    if (requireInit)
      delayedInit();


#if MTR_COLLECT_BACKTRACE
    uint32_t bktraceID = 0;
#if (_TARGET_C1 || _TARGET_C2) && (DAGOR_DBGLEVEL > 0)

#else
    ::stackhlp_fill_stack((void **)stackUnwind, MTR_MAX_UNWIND_DEPTH, 2);
#endif
#endif
    lock();
#if MTR_COLLECT_BACKTRACE
    bktraceID = addTrace();
#endif

    // find or add heap
    int heapIdx = findHeapIndex(heap);
    if (heapIdx == numHeaps)
    {
      numHeaps++;
      if (numHeaps >= MTR_MAX_HEAPS)
        fatal("memory tracker run out of heaps");
      heapIDs[heapIdx] = heap;
      heaps[heapIdx].heapID = heap;
    }

    // allocate bit in bitmap
    size_t bitmapIdx;
    for (bitmapIdx = firstNonFullBitmapChunk; (1 < maxBlocks / BITMAP_BITS) && (~bitmap[bitmapIdx] == 0); bitmapIdx++)
      ;
    if (bitmapIdx >= maxBlocks / BITMAP_BITS)
      fatal("memory tracker run out of blocks");
    firstNonFullBitmapChunk = bitmapIdx;

    bitmap_t bmp = bitmap[bitmapIdx];
    uint32_t bit = get_first_one(bmp);
    uint32_t blockIdx = bitmapIdx * BITMAP_BITS + bit;
    bitmap[bitmapIdx] |= 1U << bit;

    size_t usable_sz = user_heap ? size : sys_malloc_usable_size(addr);
    // G_ASSERT(usable_sz);
    uint32_t sIdx = indexBySize(usable_sz); // size?

    // add block info
    HeapHeader &heapHdr = heaps[heapIdx];
    heapHdr.allocated += size;
    heapHdr.count++;

    BlockHeader &blockHdr = heapHdr.blockHeaders[sIdx];
    blockHdr.allocated += size;
    blockHdr.count++;

    MemBlock &mblk = blocks[blockIdx];
    mblk.addr = addr;
    mblk.size = size;
#if MTR_COLLECT_BACKTRACE
    mblk.bktraceID = bktraceID;
#endif
    mblk.timestamp = dagor_frame_no();

    // link
    uint32_t hbOffset = addr2hash_offset(addr, sIdx);
    mblk.nextIndex = hashbins[heapIdx][hbOffset];
    hashbins[heapIdx][hbOffset] = blockIdx;

    // debug("alloc %p %p [%d] => %d %p", heap, addr, size, sIdx, &blocks[blockIdx]);
    //      if (bktraceID > 0x1000)
    //          prepareFullDump();

    unlock();
  }

  // called before free, true if block has been found in heap
  bool removeBlock(IMemAlloc *heap, void *addr, size_t user_size = 0, bool anywhere = false)
  {
    if (addr == 0 || requireInit)
      return false;

    // debug("free %p %p", heap, addr);
    lock();

    // we know chunk size, and can find it quickly
    size_t usable_sz = user_size ? user_size : sys_malloc_usable_size(addr);
    // G_ASSERT(usable_sz);
    uint32_t sIdx = indexBySize(usable_sz);

    uint32_t hbOffset = addr2hash_offset(addr, sIdx);

    uint32_t blockIdx = MemBlock::STOP_INDEX;
    uint32_t prevIdx = blockIdx;
    int heapIdx = anywhere ? 0 : findHeapIndex(heap);

    do
    {
      if (heapIdx >= numHeaps)
        break;

      for (blockIdx = hashbins[heapIdx][hbOffset], prevIdx = MemBlock::STOP_INDEX;
           blockIdx != MemBlock::STOP_INDEX && (blocks[blockIdx].addr != addr);
           prevIdx = blockIdx, blockIdx = blocks[blockIdx].nextIndex)
        ;

      if (blockIdx != MemBlock::STOP_INDEX)
        break;

      heapIdx++;
    } while (anywhere);

    G_ASSERT(heapIdx < numHeaps);
    G_ASSERT(blockIdx != MemBlock::STOP_INDEX); // zero elements?

    if (heapIdx >= numHeaps || blockIdx == MemBlock::STOP_INDEX)
    {
      unlock();
      return false;
    }

    // remove block info
    MemBlock &mblk = blocks[blockIdx];

    HeapHeader &heapHdr = heaps[heapIdx];
    heapHdr.allocated -= mblk.size;
    heapHdr.count--;

    BlockHeader &blockHdr = heapHdr.blockHeaders[sIdx];
    blockHdr.allocated -= mblk.size;
    blockHdr.count--;

    // unlink
    uint32_t nextIndex = mblk.nextIndex;
    if (prevIdx != MemBlock::STOP_INDEX)
      blocks[prevIdx].nextIndex = nextIndex;
    else
      hashbins[heapIdx][hbOffset] = nextIndex;

    mblk.nextIndex = MemBlock::STOP_INDEX;
    mblk.addr = 0;
    mblk.size = 0;

    // remove from bitmap
    size_t bitmapIdx = blockIdx / BITMAP_BITS;
    uint32_t bit = blockIdx % BITMAP_BITS;
    bitmap[bitmapIdx] &= ~(1U << bit);

    if (firstNonFullBitmapChunk > bitmapIdx)
      firstNonFullBitmapChunk = bitmapIdx;

    unlock();
    return true;
  }

  // very slow - iterates across all array
  bool isAddrValid(void *addr, size_t size)
  {
    if (addr == 0 || requireInit)
      return false;

    char *endPtr = (char *)addr + size;
    // debug("check %p %p", heap, addr);
    lock();

    int findHeapIdx = -1;
    uint32_t findBlockIdx = MemBlock::STOP_INDEX;

    for (int heapIdx = 0; heapIdx < numHeaps; heapIdx++)
    {
      if (this->heaps[heapIdx].count > 0)
      {
        for (int binNo = 0; binNo < BLK_POW2_ELEMENTS; binNo++)
        {
          uint32_t hbSize = this->get_hash_bin_size(binNo);
          for (int binElemNo = 0; binElemNo < hbSize; binElemNo++)
          {
            uint32_t offset = this->get_hash_bin_offset(binNo) + binElemNo;
            uint32_t blockIdx = this->hashbins[heapIdx][offset]; //-V557
            while (blockIdx != MemBlock::STOP_INDEX)
            {
              if (this->blocks[blockIdx].addr <= addr && ((char *)this->blocks[blockIdx].addr + this->blocks[blockIdx].size) > endPtr)
              {
                findHeapIdx = heapIdx;
                findBlockIdx = blockIdx;
                break;
              }
              blockIdx = this->blocks[blockIdx].nextIndex;
            }
            if (findHeapIdx >= 0)
              break;
          }
          if (findHeapIdx >= 0)
            break;
        }
      }
      if (findHeapIdx >= 0)
        break;
    }

    if (findHeapIdx >= 0)
    {
      // MemBlock &mb = this->blocks[findBlockIdx];
      // debug("xmb %p -> %p, %x", addr, mb.addr, mb.size);
    }

    unlock();

    if (findHeapIdx < 0)
    {
      logerr("xmb addr not found %p", addr);
      return false;
    }

    return true;
  }

#endif

  void registerHeap(IMemAlloc *heap, const char *name)
  {
    for (int heapIdx = 0; heapIdx < MTR_MAX_HEAPS; heapIdx++)
      if (heapNames[heapIdx].heapID == heap)
        return;

    for (int heapIdx = 0; heapIdx < MTR_MAX_HEAPS; heapIdx++)
      if (heapNames[heapIdx].heapID == NULL)
      {
        HeapName &hn = heapNames[heapIdx];
        hn.heapID = heap;
        strncpy(hn.name, name, sizeof(hn.name) - 1);
        return;
      }
  }

  // called after init() and before constructor
  void delayedInit()
  {
    G_ASSERT(maxBlocks && !bitmap && !blocks);
    // addBlock could be called from malloc if overloaded
    bitmap = (bitmap_t *)malloc(maxBlocks / BITMAP_BITS * sizeof(bitmap_t)); //-V575
    blocks = (MemBlock *)malloc(maxBlocks * sizeof(MemBlock));               //-V575
    btElems = nullptr;
#if MTR_COLLECT_BACKTRACE
    btElems = (uintptr_t *)malloc(maxBtElems * sizeof(uintptr_t)); //-V575
    // TODO: store higher 32bit
    btElems[0] = 0; //-V522
#endif
    currentBtIndex = 1; // 0 is used as invalid/stop value
    savedBtIndex = currentBtIndex;

    size_t totalSize = maxBlocks * sizeof(MemBlock) + maxBlocks / BITMAP_BITS * sizeof(bitmap_t) + maxBtElems * sizeof(uintptr_t);
    dumpSize = totalSize * dumpSizePercent / 100;
    dumpBase = (uint8_t *)malloc(dumpSize);
    dumpSizePacked = 0;

    memset(bitmap, 0, maxBlocks / BITMAP_BITS * sizeof(bitmap_t)); //-V575
    memset(blocks, 0, maxBlocks * sizeof(MemBlock));
    clearAll();

    create_critical_section(mtlock, "dlmalloc");

#if MEMUSAGE_COUNT == 3
    // reference gamelibs/MemoryTracker as it gets deleted by linker
    dummy = (uint8_t)((uintptr_t)mtrk_console_handler);
#endif
    requireInit = false;
  }

  // called before constructor.
  // set limits, as tracker do not realloc it's work memory
  // num_memory_blocks - max expected memory blocks
  // back_trace_elements - addresses in callstack backtrace buffer (max 2^24-1)
  //
  void init(int num_memory_blocks, int back_trace_elements, int dump_size_percent)
  {
    requireInit = true;
    maxBlocks = POW2_ALIGN(num_memory_blocks, BITMAP_BITS);
    maxBtElems = POW2_ALIGN(back_trace_elements, 32); // 4k aligned
    dumpSizePercent = max(dump_size_percent, 10);
    G_ASSERT(maxBtElems < (1 << 24));
    G_ASSERT(maxBlocks);

    for (int heapIdx = 0; heapIdx < MTR_MAX_HEAPS; heapIdx++)
    {
      heapNames[heapIdx].heapID = NULL;
      heapNames[heapIdx].name[0] = 0;
    }
  }

  bool write(file_ptr_t file, const void *ptr, int len) { return len == df_write(file, ptr, len); }

  struct PackState
  {
    uint8_t *cur;
    uint8_t *end;
    uint8_t *beg;
    bool failed;

    void init(void *mem, size_t size)
    {
      beg = (uint8_t *)mem;
      end = beg + size;
      cur = beg;
      failed = false;
    }

    PackState(void *mem, size_t size) { init(mem, size); }
    uint8_t *rewind(uint8_t *ptr)
    {
      cur = ptr;
      return cur;
    }
    bool haveSpace(size_t size) { return (end - cur) >= size; }
    uint8_t *skip(size_t size)
    {
      cur += size;
      return cur - size;
    }
    uint8_t *align(size_t size)
    {
      cur = POW2_ALIGN_PTR(cur, size, uint8_t);
      return cur;
    }
    uint8_t *append(void *data, size_t size)
    {
      if (cur + size < end)
      {
        memcpy(cur, data, size);
        cur += size;
        return cur - size;
      }
      failed = true;
      return cur;
    }

    void pack(void *data, size_t size)
    {
      uint32_t *packedSizePtr = (uint32_t *)cur;
      cur += sizeof(uint32_t);
#if MTR_PACK_DATA == 0
      if (cur + size < end)
      {
        memcpy(cur, data, size);
        cur += size;
      }
      else
        failed = true;
#else
      if (cur + size < end)
      {
        uint8_t *dst = cur;
        uint8_t *src = (uint8_t *)data;

        size_t si = 0;
        size_t di = 0;
        while (si < size)
        {
          size_t ci = di;
          size_t szfill = 0;
          di++;

          while (si < size && (di - ci) < 16)
          {
            uint8_t b = src[si];
            dst[di++] = b;
            si++;

            if (si < size && b == src[si])
            {
              size_t oi = si;
              for (; si < size; si++)
                if (b != src[si])
                  break;

              szfill = si - oi;
              if (szfill >= 2)
                break;
            }
          }

          uint32_t szfillb = min(szfill, 14);
          szfill -= szfillb;
          dst[ci] = ((di - ci - 1) << 4) | szfillb; // control byte [copy][fill]

          if (szfill < 64)
          {
            while (szfill != 0)
            {
              uint32_t szfillb = min(szfill, 14);
              dst[di++] = szfillb;
              szfill -= szfillb;
            };
          }
          else
          {
            while (szfill != 0)
            {
              uint32_t szfillb = min(szfill, 65535);
              dst[di + 0] = 0x0f;
              dst[di + 1] = szfillb & 0xff;
              dst[di + 2] = szfillb >> 8;
              di += 3;
              szfill -= szfillb;
            };
          }
        }
        cur += di;
      }
      else
        failed = true;
#endif
      *packedSizePtr = (uintptr_t)cur - (uintptr_t)packedSizePtr;
    }

    template <typename T>
    inline void pack(T obj)
    {
      pack(&obj, sizeof(obj));
    }

    void flush() {}
  };

  struct UnpackState
  {
    uint8_t *cur;
    uint8_t *end;
    uint8_t *beg;
    bool failed;

    void init(void *mem, size_t size)
    {
      beg = (uint8_t *)mem;
      end = beg + size;
      cur = beg;
      failed = false;
    }

    UnpackState(void *mem, size_t size) { init(mem, size); }

    uint8_t *skip(size_t size)
    {
      cur += size;
      return cur - size;
    }
    uint8_t *align(size_t size)
    {
      cur = POW2_ALIGN_PTR(cur, size, uint8_t);
      return cur;
    }
    uint8_t *pop(void *data, size_t size)
    {
      if (cur + size < end)
      {
        memcpy(data, cur, size);
        cur += size;
        return cur - size;
      }
      failed = true;
      return cur;
    }

    void unpack(void *data, size_t size)
    {
      uint32_t *packedSizePtr = (uint32_t *)cur;
      cur += sizeof(uint32_t);
#if MTR_PACK_DATA == 0
      if (cur + size < end)
      {
        memcpy(data, cur, size);
        cur += size;
      }
      else
        failed = true;
#else
      // TODO: unpack code
#endif
      *packedSizePtr = (uintptr_t)cur - (uintptr_t)packedSizePtr;
    }

    template <typename T>
    inline void unpack(T obj)
    {
      unpack(&obj, sizeof(obj));
    }
  };

  void updateHeapNames()
  {
    for (int i = 0; i < numHeaps; i++)
    {
      HeapHeader &hh = heaps[i];
      // find and copy heap name
      memset(hh.name, 0, sizeof(hh.name));
      for (int heapIdx = 0; heapIdx < MTR_MAX_HEAPS; heapIdx++)
        if (heapNames[heapIdx].heapID == hh.heapID)
        {
          memcpy(hh.name, heapNames[heapIdx].name, sizeof(hh.name));
          break;
        }
    }
  }

  void prepareFullDump()
  {
    lock();

    PackState ps(dumpBase, dumpSize);

    // preprocess heaps
    updateHeapNames();

    uint32_t nblocks = 0;
    for (int i = 0; i < numHeaps; i++)
      nblocks += heaps[i].count;

    uint32_t savedBitIdxs = (maxBlocks / BITMAP_BITS) - firstNonFullBitmapChunk;

    DumpHeader hdr;
    hdr.version = MTR_VERSION;
    hdr.ptrSize = sizeof(uintptr_t);
    hdr.pad = 0;
    hdr.numHeaps = numHeaps;
    hdr.numBlocks = nblocks;
    hdr.maxBlocks = maxBlocks;
    hdr.firstBitIdx = firstNonFullBitmapChunk;
    hdr.savedBitIdx = savedBitIdxs;
    hdr.maxBtElems = maxBtElems;
    hdr.numBtElems = currentBtIndex;

    ps.pack(hdr);

    for (int i = 0; i < numHeaps; i++)
      ps.pack(heaps[i]);

    for (int i = 0; i < numHeaps; i++)
      ps.pack(hashbins[i]);

    ps.pack(tracebins, sizeof(tracebins));

    // pack actual data
    ps.pack(&bitmap[firstNonFullBitmapChunk], sizeof(bitmap_t) * savedBitIdxs);
    ps.pack(blocks, sizeof(MemBlock) * maxBlocks);
#if MTR_COLLECT_BACKTRACE
    ps.pack(btElems, sizeof(uintptr_t) * currentBtIndex);
#endif
    ps.flush();
    dumpSizePacked = ps.cur - ps.beg;

    if (ps.failed)
    {
      debug("memory tracker dump failed (used %d/%dKB)", int(dumpSizePacked) >> 10, int(dumpSize) >> 10);
      dumpSizePacked = 0;
    }

    unlock();
  }

  void saveDump(const char *filename)
  {
    file_ptr_t ofile = df_open(filename, DF_APPEND);
    if (ofile)
    {
      write(ofile, dumpBase, dumpSizePacked);
      df_close(ofile);
    }
  }

  void unpackDump(void *ptr, size_t size)
  {
    lock();

    UnpackState ups(ptr, size);

    DumpHeader hdr;
    ups.unpack(hdr);

    G_ASSERT(hdr.version == MTR_VERSION);
    G_ASSERT(hdr.ptrSize == sizeof(uintptr_t));
    numHeaps = hdr.numHeaps;
    maxBlocks = hdr.maxBlocks;
    firstNonFullBitmapChunk = hdr.firstBitIdx;
    uint32_t savedBitIdxs = hdr.savedBitIdx;
    maxBtElems = hdr.maxBtElems;
    currentBtIndex = hdr.numBtElems;

    for (int i = 0; i < numHeaps; i++)
      ups.unpack(heaps[i]);

    for (int i = 0; i < numHeaps; i++)
      ups.unpack(hashbins[i]);

    ups.unpack(tracebins, sizeof(tracebins));

    ups.unpack(&bitmap[firstNonFullBitmapChunk], sizeof(bitmap_t) * savedBitIdxs);
    ups.unpack(blocks, sizeof(MemBlock) * maxBlocks);
#if MTR_COLLECT_BACKTRACE
    ups.unpack(btElems, sizeof(uintptr_t) * currentBtIndex);
#endif
    updateHeapNames();

    unlock();
  }

  void printStats()
  {
    updateHeapNames();

    uint32_t nblocks = 0;
    size_t allocated = 0;
    for (int i = 0; i < numHeaps; i++)
    {
      HeapHeader &hh = heaps[i];
      if (hh.isGpuHeap())
        continue;
      nblocks += hh.count;
      allocated += hh.allocated;
      debug("\nheap %s alloc=%d KB in %d blocks", hh.name, hh.allocated >> 10, hh.count);

      for (size_t i = 0; i < BLK_POW2_ELEMENTS; i++)
      {
        BlockHeader &bh = hh.blockHeaders[i];
        if (bh.allocated > 1024 && bh.count > 0)
          debug("bin %8d alloc=%d KB in %d blocks", 1 << (i + BLK_POW2_LOW), bh.allocated >> 10, bh.count);
      }
    }
    debug("Summary for CPU heaps: %d KB in %d blocks, frame %u", allocated >> 10, nblocks, dagor_frame_no());
#if _TARGET_C1 | _TARGET_C2




















#endif
  }

  struct HeapInfoCollect
  {
    int index = 0;
    eastl::vector_set<int> backtraceSet;
  };

  struct PrintMemBlockInfo
  {
    uint32_t index;
    uint32_t size;
  };

  static void showBlockLine(uint32_t size, void *ptr, uint32_t count)
  {
    const int maxTexLen = 16;
    char text[maxTexLen + 1];
    text[0] = 0;

    if (ptr != nullptr)
    {
      uint8_t *pb = (uint8_t *)ptr;
      bool isText = true;
      int textIdx = 0;
      for (textIdx = 0; textIdx < 16; textIdx++)
      {
        text[textIdx] = pb[textIdx];
        if (!isalnum(pb[textIdx]) && !ispunct(pb[textIdx]))
          isText = false;
      }
      if (!isText)
        textIdx = 0;
      text[textIdx] = 0;
    }

    if (ptr == nullptr)
    {
      if (count > 1)
        debug("blk %010d | %p x %d", size, ptr, count);
      else if (count == 1)
        debug("blk %010d | %p", size, ptr);
    }
    else
    {
      uint32_t *pw = (uint32_t *)ptr;
      if (count > 1)
        debug("blk %010d | %p x %d |  %08x %08x %08x %08x | %s ", size, ptr, count, pw[0], pw[1], pw[2], pw[3], text);
      else if (count == 1)
        debug("blk %010d | %p |  %08x %08x %08x %08x | %s ", size, ptr, pw[0], pw[1], pw[2], pw[3], text);
      G_UNUSED(pw);
    }
    G_UNUSED(size);
  }

  // min_alloc_kb  - minimal amount for specific backtrace to appear in log
  // show_lines - maximum block lines to show
  void printDetailedStats(int min_size, int max_size, int min_alloc_kb, int show_lines, uint32_t min_frame)
  {
    int min_alloc = min_alloc_kb << 10;

    eastl::vector_set<int> backtraceSet;
    backtraceSet.reserve(maxBtElems / MTR_MAX_UNWIND_DEPTH);

    lock();

    updateHeapNames();

    MemoryTracker::PackState ps(dumpBase, dumpSize);

    debug("Show blocks >= %d && <= %d for frame %u:\n", min_size, max_size, dagor_frame_no());
    uint32_t nblocks = 0;
    for (int heapIdx = 0; heapIdx < numHeaps; heapIdx++)
    {
      backtraceSet.clear();

      HeapHeader &hh = heaps[heapIdx];
      nblocks += hh.count;
      debug("heap %s alloc=%d KB in %d blocks. ", hh.name, hh.allocated >> 10, hh.count);

      uint32_t totalMatched = 0;
      // collect back traces for specific heap
      for (size_t binNo = 0; binNo < BLK_POW2_ELEMENTS; binNo++)
      {
        uint32_t hbSize = get_hash_bin_size(binNo);
        for (int binElemNo = 0; binElemNo < hbSize; binElemNo++)
        {
          uint32_t offset = get_hash_bin_offset(binNo) + binElemNo;
          uint32_t blockIdx = hashbins[heapIdx][offset]; //-V557
          while (blockIdx != MemBlock::STOP_INDEX)
          {
            // check consistency. enable if needed
            // G_ASSERT(bitmap[blockIdx / BITMAP_BITS] & (1U<<(blockIdx % BITMAP_BITS)));

            MemBlock &mb = blocks[blockIdx];
            if (mb.size >= min_size && mb.size <= max_size && mb.timestamp >= min_frame)
            {
              backtraceSet.insert(mb.bktraceID);
              totalMatched++;
            }
            blockIdx = blocks[blockIdx].nextIndex;
          }
        }
      }

      debug("total matching blocks %u ", totalMatched);

      size_t btTextBufsz = 10000;
      char *btTextBuf = (char *)ps.skip(btTextBufsz); // enough buffer to dump call stack

      uint8_t *workBuf = ps.align(sizeof(uint64_t));
      for (auto btIdx : backtraceSet)
      {
        ps.rewind(workBuf); // reuse work buffer

        int btDepth = unpackBtHdrDepth(btElems[btIdx]);
        void **btStart = (void **)ps.append((void *)&btElems[btIdx + 1], sizeof(void *) * btDepth);

        int shownCnt = 0;
        int totalCnt = 0;
        size_t shownSize = 0;
        size_t totalSize = 0;

        PrintMemBlockInfo *pmbInfo = (PrintMemBlockInfo *)ps.align(sizeof(uint64_t));
        ;

        // matching blocks
        // TODO: this ie very SLOW; Optimize
        for (size_t binNo = 0; binNo < BLK_POW2_ELEMENTS; binNo++)
        {
          uint32_t hbSize = get_hash_bin_size(binNo);
          for (int binElemNo = 0; binElemNo < hbSize; binElemNo++)
          {
            uint32_t offset = get_hash_bin_offset(binNo) + binElemNo;
            uint32_t blockIdx = hashbins[heapIdx][offset]; //-V557
            while (blockIdx != MemBlock::STOP_INDEX)
            {
              // check consistency. enable if needed
              // G_ASSERT(bitmap[blockIdx / BITMAP_BITS] & (1U<<(blockIdx % BITMAP_BITS)));

              MemBlock &mb = blocks[blockIdx];
              if (mb.bktraceID == btIdx)
              {
                if (mb.size >= min_size && mb.size <= max_size && mb.timestamp >= min_frame)
                {
                  if (!ps.haveSpace(sizeof(PrintMemBlockInfo)))
                    break;
                  if (shownCnt > 40000) // qsort is crashing with larger arrays
                    break;
                  PrintMemBlockInfo pmb;
                  pmb.index = blockIdx;
                  pmb.size = mb.size;
                  ps.append(&pmb, sizeof(pmb));
                  shownCnt++;
                  shownSize += mb.size;
                }
                totalCnt++;
                totalSize += mb.size;
              }

              blockIdx = mb.nextIndex;
            }
          }
        }

        if (shownSize < min_alloc)
          continue;

        struct SortCmp
        {
          static int compare(const PrintMemBlockInfo &a, const PrintMemBlockInfo &b) { return b.size - a.size; }
        };

        // debug("qsort %p %d", pmbInfo, shownCnt);

        Qsort<PrintMemBlockInfo, SortCmp> srt;
        srt.sort(pmbInfo, shownCnt);

        const char *btTextBufx = stackhlp_get_call_stack(btTextBuf, btTextBufsz, btStart, btDepth);

        // shown - blocks that matches size
        // total - all blocks from this backtrace
        debug("\n\n%u KB (%d blocks) / %u KB (%d blocks) (matched/total) ", uint32_t(shownSize >> 10), shownCnt,
          uint32_t(totalSize >> 10), totalCnt);
        debug("Blocks for stack trace %d\n%s:\n", btIdx, btTextBufx);
        G_UNUSED(btTextBufx);

        int shown = 0;
        int lastCnt = 0;
        void *lastAddr = 0;
        uint32_t lastSize = ~0;
        for (int i = 0; i < shownCnt; i++)
        {
          if (shown >= show_lines)
            break;
          int blockIdx = pmbInfo[i].index;
          MemBlock &mb = blocks[blockIdx];
          if (mb.size != lastSize)
          {
            showBlockLine(lastSize, lastAddr, lastCnt);
            lastCnt = 1;
            lastSize = mb.size;
            lastAddr = mb.addr;
            shown++;
          }
          else
          {
            lastCnt++;
          }
        }

        showBlockLine(lastSize, lastAddr, lastCnt);
      }
    }
    debug("--------");

    unlock();
  }
};

MemoryTracker *get_memtracker_access();

} // namespace memorytracker

using namespace memorytracker;

#if INSIDE_ENGINE
namespace dagor_memory_tracker
{
void register_heap(IMemAlloc *heap, const char *name)
{
  if (get_memtracker_access())
    get_memtracker_access()->registerHeap(heap, name);
}

void add_block(IMemAlloc *heap, void *addr, size_t size, bool user_heap)
{
  if (get_memtracker_access())
    get_memtracker_access()->addBlock(heap, addr, size, user_heap);
}

bool remove_block(IMemAlloc *heap, void *addr, size_t user_size, bool anywhere)
{
  if (!get_memtracker_access())
    return true;
  return get_memtracker_access()->removeBlock(heap, addr, user_size, anywhere);
}

bool is_addr_valid(void *addr, size_t size)
{
  if (!get_memtracker_access())
    return true;
  return get_memtracker_access()->isAddrValid(addr, size);
}

void save_full_dump(const char *filename)
{
  if (get_memtracker_access())
    get_memtracker_access()->saveDump(filename);
}

void print_full_stats()
{
  if (get_memtracker_access())
    get_memtracker_access()->printStats();
}

#if DAGOR_DBGLEVEL > 0
void assert_addr(void *addr, size_t size) { G_ASSERT(is_addr_valid(addr, size)); }
#else
void assert_addr(void *, size_t) {}
#endif
} // namespace dagor_memory_tracker
#endif
