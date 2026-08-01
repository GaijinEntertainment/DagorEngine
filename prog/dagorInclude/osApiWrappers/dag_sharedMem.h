//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

#include <supp/dag_define_KRNLIMP.h>

//! creates global shared (mapped) view of named file
KRNLIMP void *create_global_map_shared_mem(const char *shared_mem_fname, void *base_addr, size_t sz, intptr_t &out_fd);

//! opens global shared (mapped) view of existing named file
KRNLIMP void *open_global_map_shared_mem(const char *shared_mem_fname, void *base_addr, size_t sz, intptr_t &out_fd);

//! destroys shared (mapped) view of named file
KRNLIMP void close_global_map_shared_mem(intptr_t fd, void *addr, size_t sz);

//! removes temporary storage used for shared mem (may be no-op on some platforms)
KRNLIMP void unlink_global_shared_mem(const char *shared_mem_fname);


//! marks pages in shared mem region as read-only/read-write
KRNLIMP void mark_global_shared_mem_readonly(void *addr, size_t sz, bool read_only);

#include <supp/dag_undef_KRNLIMP.h>


#ifdef __cplusplus
#include <osApiWrappers/dag_globalMutex.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <generic/dag_span.h>
#include <debug/dag_log.h>
#include <string.h>

class GlobalSharedMemStorage
{
  struct ScopedLock
  {
    ScopedLock(void *gm, void *lm)
    {
      if (gm)
        global_mutex_enter(globalMutex = gm);
      else if (lm)
        enter_critical_section_raw(localMutex = lm);
    }
    ~ScopedLock() { release(); }

    void release()
    {
      if (globalMutex)
      {
        global_mutex_leave(globalMutex);
        globalMutex = nullptr;
      }
      else if (localMutex)
      {
        leave_critical_section(localMutex);
        localMutex = nullptr;
      }
    }

  private:
    void *globalMutex = nullptr, *localMutex = nullptr;
  };

public:
  struct Data
  {
    struct Rec
    {
      int64_t offs;
      int32_t sz;       //< size of the current occupant's payload
      int32_t capacity; //< size reserved at this offs when the record was first allocated; never shrinks on reuse
      uint32_t tag;
      uint16_t refCount;
      volatile uint16_t ready;
      char name[256 - sizeof(int64_t) - sizeof(int32_t) * 3 - sizeof(uint16_t) * 2];

      bool cmpEq(const char *nm, int32_t _tag) const { return tag == _tag && strcmp(nm, name) == 0; }
    };

    int64_t baseAddr;
    int64_t memSz;
    int32_t refCount;
    int32_t maxRecNum;
    int64_t memUsed;
    int32_t recUsed;
    int32_t _resv[7];
    Rec rec[1]; //< variable size array

    dag::Span<Rec> usedRec() { return dag::Span<Rec>(rec, recUsed); }
    dag::Span<Rec> allRec() { return dag::Span<Rec>(rec, maxRecNum); }
    int index(const Rec &r) const { return int(&r - rec); }

    void mark_rec_ro() { mark_global_shared_mem_readonly(this, calcSizeof(maxRecNum), true); }
    void mark_rec_rw() { mark_global_shared_mem_readonly(this, calcSizeof(maxRecNum), false); }

    static int calcSizeof(int rec_num) { return ((sizeof(Data) + sizeof(Rec) * (rec_num - 1)) + 0xFFF) & ~0xFFF; }
  };

public:
  GlobalSharedMemStorage() = default;

  //! create new/connect to global (inter-process) shared memory storage
  GlobalSharedMemStorage(const char *shared_name, size_t sz, int max_rec_num)
  {
    memset(sharedName, 0, sizeof(sharedName));
    init(shared_name, sz, max_rec_num);
  }
  //! create local (inside process only) shared memory storage
  GlobalSharedMemStorage(int max_rec_num)
  {
    memset(sharedName, 0, sizeof(sharedName));
    initLocal(max_rec_num);
  }
  ~GlobalSharedMemStorage() { term(); }

  //! create new/connect to global (inter-process) shared memory storage
  Data *init(const char *shared_name, size_t sz, int max_rec_num)
  {
    mutex = global_mutex_create(shared_name);
    if (!mutex)
    {
      logmessage(_MAKE4C('SHMM'), "can't create mutex '%s'", shared_name);
      return NULL;
    }

    memset(sharedName, 0, sizeof(sharedName));
    strncpy(sharedName, shared_name, sizeof(sharedName) - 1);

    if (sz < Data::calcSizeof(max_rec_num))
      sz = Data::calcSizeof(max_rec_num) + 4096;
    sz = (sz + 0xFFFF) & ~0xFFFF; // 64K align

    global_mutex_enter(mutex);
    data = (Data *)open_global_map_shared_mem(sharedName, NULL, sz, fd); // open in any location
    int64_t base_addr = data ? data->baseAddr : 0;
    if (data && base_addr != (int64_t)(uintptr_t)data)
    {
      base_addr = data->baseAddr;
      close_global_map_shared_mem(fd, data, sz);

      data = (Data *)open_global_map_shared_mem(sharedName, (void *)base_addr, sz, fd); // try to open in fixed location
      if (!data)
        data = (Data *)open_global_map_shared_mem(sharedName, NULL, sz, fd); // open in any location (again)
      if (!data)
      {
      init_fail:
        logmessage(_MAKE4C('SHMM'), "failed to map shared mem: name=%s, sz=%lluK", sharedName, ((uint64_t)sz) >> 10);
        close_global_map_shared_mem(fd, NULL, 0);
        global_mutex_leave_destroy(mutex, sharedName);
        mutex = NULL;
        return NULL;
      }
    }
    if (!data) // create new one
    {
      data = (Data *)create_global_map_shared_mem(sharedName, NULL, sz, fd);
      if (!data)
        goto init_fail;
      memset(data, 0, Data::calcSizeof(max_rec_num));
      data->baseAddr = (int64_t)(uintptr_t)data;
      data->memSz = sz;
      data->refCount = 1;
      data->maxRecNum = max_rec_num;
      data->recUsed = 0;
      data->memUsed = Data::calcSizeof(data->maxRecNum);
      logmessage(_MAKE4C('SHMM'), "created shared mem: %s, 0x%llX, %lluK", sharedName, (uint64_t)(uintptr_t)data,
        ((uint64_t)data->memSz) >> 10);
    }
    else
    {
      data->refCount++;
      logmessage(_MAKE4C('SHMM'), "opened shared mem: %s, 0x%llX, base 0x%llX, %lluK", sharedName, (uint64_t)(uintptr_t)data,
        (uint64_t)data->baseAddr, ((uint64_t)data->memSz) >> 10);
    }
    mark_global_shared_mem_readonly(data, (size_t)data->memUsed, true);
    global_mutex_leave(mutex);
    return data;
  }

  void term()
  {
    if (!mutex)
      return termLocal();
    global_mutex_enter(mutex);
    bool need_unlink = false;
    if (data)
    {
      data->mark_rec_rw();
      data->refCount--;
      if (!data->refCount)
        need_unlink = true;
      dumpContentsInternal("terminating");
      close_global_map_shared_mem(fd, data, (size_t)data->memSz);
    }
    if (need_unlink)
      unlink_global_shared_mem(sharedName);
    global_mutex_leave_destroy(mutex, sharedName);
    fd = -1;
    data = NULL;
    mutex = NULL;
    memset(sharedName, 0, sizeof(sharedName));
  }


  //! create local (inside process only) shared memory storage
  Data *initLocal(int max_rec_num)
  {
    localMem = midmem;
    localMutex = new (localMem) WinCritSec;
    int sz = Data::calcSizeof(max_rec_num);

    ScopedLock lock(nullptr, localMutex);
    strncpy(sharedName, "-local-", sizeof(sharedName) - 1);
    data = (Data *)localMem->allocAligned(sz, 4096);
    memset(data, 0, sz);
    data->baseAddr = (int64_t)(uintptr_t)data;
    data->memSz = 0;
    data->refCount = 1;
    data->maxRecNum = max_rec_num;
    data->recUsed = 0;
    data->memUsed = 0;
    logmessage(_MAKE4C('SHMM'), "created shared mem: %s, 0x%llX, %u", sharedName, (uint64_t)(uintptr_t)data, sz);
    data->mark_rec_ro();
    return data;
  }

  void termLocal()
  {
    auto *mem = localMem;
    if (!mem)
      return;

    {
      ScopedLock lock(nullptr, localMutex);
      if (data->refCount != 1)
        return logerr("data->refCount=%d", data->refCount);
      data->mark_rec_rw();
      for (auto &r : data->usedRec())
        if (r.offs)
        {
          void *ptr = offsToPtr(r.offs);
          // a still-claimed record leaks by design: consumers must release before term - once the
          // storage is gone, no flag state can tell their pointers apart from private allocations
          if (r.refCount != 0)
          {
            logerr("data->rec[%d].refCount=%d tag=%c%c%c%c ptr=%p,%dK", &r - data->rec, r.refCount, _DUMP4C(r.tag), ptr, r.sz >> 10);
            continue;
          }
          mark_global_shared_mem_readonly(ptr, r.capacity, false);
          localMem->freeAligned(ptr);
          r.offs = r.sz = r.capacity = 0;
          r.tag = 0;
        }
      data->recUsed = 0;
      localMem->freeAligned(data);
      data = nullptr; // like term(): a consumer probing doesPtrBelong after teardown must see a dead storage, not freed memory
      localMem = nullptr;
    }
    memdelete(localMutex, mem);
    localMutex = nullptr;
  }

  void addRefLocal()
  {
    if (!localMem)
      return;
    ScopedLock lock(mutex, localMutex);
    data->mark_rec_rw();
    data->refCount++;
    data->mark_rec_ro();
  }

  void delRefLocal()
  {
    if (!localMem)
      return;
    ScopedLock lock(mutex, localMutex);
    data->mark_rec_rw();
    data->refCount--;
    data->mark_rec_ro();
  }

  const char *getSharedName() const { return sharedName; }
  int getRefCount() const { return data ? data->refCount : 0; }
  //! returns true if this process maps view of shared mem to the same address as it was mapped in creator process
  bool hasStableBaseAddr() const { return data && baseAddrDelta() == 0; }
  //! returns offset to be added to a pointer baked at the creator process to get proper local address; 0 for stable address
  int64_t baseAddrDelta() const { return data ? (int64_t)(uintptr_t)data - data->baseAddr : 0; }
  bool doesPtrBelong(void *p) const
  {
    if (!data)
      return false;
    if (localMem)
    {
      for (auto &r : data->usedRec())
        if (r.offs && intptr_t(p) >= r.offs && intptr_t(p) < r.offs + r.capacity)
          return true;
      return false;
    }
    int64_t offs = (int64_t)(uintptr_t)p - (int64_t)(uintptr_t)data;
    return offs >= Data::calcSizeof(data->maxRecNum) && offs < data->memSz;
  }
  size_t getMemUsed() const { return data ? data->memUsed : 0; }
  size_t getMemSize() const { return data ? data->memSz : 0; }
  int getRecUsed() const { return data ? data->recUsed : 0; }
  const Data::Rec &getRec(int i) const { return data->rec[i]; }

  size_t getPtrSize(void *p) const
  {
    if (!data)
      return 0;
    int64_t offs = ptrToOffs(p);
    ScopedLock lock(mutex, localMutex);
    for (auto &r : data->usedRec())
      if (r.offs == offs)
        return r.sz;
    return 0;
  }


  void *findPtr(const char *ptr_fname, uint32_t tag)
  {
    if (!data)
      return NULL;
    ScopedLock lock(mutex, localMutex);
    for (auto &r : data->usedRec())
      if (r.cmpEq(ptr_fname, tag))
        return claimAndWaitReady(r, ptr_fname, tag, lock);
    return NULL;
  }
  //! one-call publish-or-attach: a fresh record to fill (is_new=true: fill, mark read-only, then
  //! markPtrDataReady), or the existing same-size record claimed and waited ready like findPtr
  //! (is_new=false: attach, do not write); null on a full storage, a different-size record or a
  //! leader that never marked ready
  void *findOrAlloc(const char *ptr_fname, uint32_t tag, size_t sz, bool &is_new)
  {
    void *already = nullptr;
    void *ret = allocPtr(ptr_fname, tag, sz, &already);
    is_new = ret != nullptr;
    return ret ? ret : already;
  }
  //! allocates a fresh record to publish into; an existing name+tag record is never handed out
  //! as a fresh slot (it may be read-only and already served to readers - the loser of a publish
  //! race must fall back, not rewrite the winner's pages). With out_already_allocated, an
  //! existing record of the same declared size is instead claimed and waited ready like findPtr
  //! and returned through it, so alloc-or-attach is one call; without it (the default), an
  //! existing record just returns null and attaching stays findPtr's job.
  void *allocPtr(const char *ptr_fname, uint32_t tag, size_t sz, void **out_already_allocated = nullptr)
  {
    if (out_already_allocated)
      *out_already_allocated = nullptr;
    if (!data)
      return NULL;
    ScopedLock lock(mutex, localMutex);
    for (auto &r : data->usedRec())
      if (r.cmpEq(ptr_fname, tag))
      {
        // a record of a different size is not the same content: never serve it under this name
        if (!out_already_allocated || (int64_t)r.sz != (int64_t)sz)
          return nullptr;
        *out_already_allocated = claimAndWaitReady(r, ptr_fname, tag, lock);
        return nullptr;
      }

    size_t orig_sz = sz;
    sz = (sz + 0xFFF) & ~0xFFF; // 4K align for pages

    if (localMem)
    {
      for (auto &r : data->allRec())
        if (!r.offs)
        {
          auto ptr = (int64_t)(uintptr_t)localMem->allocAligned(sz, 4096);
          if (!ptr)
            break;
          data->mark_rec_rw();
          r.offs = ptr;
          strncpy(r.name, ptr_fname, sizeof(r.name) - 1);
          r.tag = tag;
          r.sz = orig_sz; // the declared payload size, as in the global flavor; capacity keeps the padded one
          r.capacity = sz;
          r.refCount = 1;
          data->memUsed += sz;
          data->recUsed = max<unsigned>(data->recUsed, data->index(r) + 1);
          data->mark_rec_ro();
          return offsToPtr(r.offs);
        }
      logmessage(_MAKE4C('SHMM'), "failed to alloc ptr in shared mem: mem(rec=%d/%d), ptr(sz=%llu)", data->recUsed, data->maxRecNum,
        (uint64_t)sz);
      return NULL;
    }

    // Global storage has no free(): capacity is reserved once and never released, so reuse a refCount==0
    // record whose capacity fits rather than always growing memUsed/recUsed.
    // Best-fit, not first-fit: capacity never shrinks, so first-fit can permanently waste a big slot on a small payload.
    //
    // Skip a poor-fitting candidate when a fresh append is affordable without it: a candidate's own slack
    // (capacity - sz) counts toward that affordability check too, since passing on it costs nothing but taking
    // it wastes that slack forever.
    // Record-slot headroom is checked separately, since reuse never consumes a slot but a fresh append always does.
    constexpr int64_t low_occupation_pct = 25;
    constexpr int64_t ample_headroom_pct = 30;
    bool recHeadroomAmple = (int64_t)(data->maxRecNum - data->recUsed) * 100 > (int64_t)data->maxRecNum * ample_headroom_pct;
    int best = -1;
    for (int i = 0, ie = data->recUsed; i < ie; i++)
    {
      Data::Rec &r = data->rec[i];
      if (r.refCount != 0 || (int64_t)sz > r.capacity)
        continue;
      if (recHeadroomAmple && (int64_t)sz * 100 < r.capacity * low_occupation_pct)
      {
        int64_t affordableIfSkipped = (data->memSz - data->memUsed) + (r.capacity - (int64_t)sz);
        if (affordableIfSkipped * 100 > data->memSz * ample_headroom_pct)
          continue; // poor fit, and passing on it still leaves us comfortably able to append fresh
      }
      if (best < 0 || r.capacity < data->rec[best].capacity)
        best = i;
    }
    if (best >= 0)
    {
      Data::Rec &r = data->rec[best];
      data->mark_rec_rw();
      strncpy(r.name, ptr_fname, sizeof(r.name) - 1);
      r.tag = tag;
      r.sz = orig_sz;
      r.refCount = 1;
      r.ready = 0; // must clear: this slot's previous occupant left it set
      data->mark_rec_ro();
      return pointerToWritableArea(r);
    }

    // Trim the trailing run of refCount==0 records once utilization is high, or as a last resort right before
    // this would otherwise fail.
    // Safe regardless of trigger: nothing can be watching a refCount==0 record (see findPtr()'s ref-claim-before-unlock
    // above), and nothing needs to move since these are already the highest-offset records.
    constexpr int64_t high_utilization_pct = 80;
    bool nearFull = data->memUsed * 100 > data->memSz * high_utilization_pct ||
                    (int64_t)data->recUsed * 100 > (int64_t)data->maxRecNum * high_utilization_pct;
    bool wouldFail = data->recUsed >= data->maxRecNum || data->memUsed + (int64_t)sz > data->memSz;
    if ((nearFull || wouldFail) && data->recUsed > 0 && data->rec[data->recUsed - 1].refCount == 0)
    {
      int trimmedRecs = 0;
      int64_t trimmedBytes = 0;
      data->mark_rec_rw();
      while (data->recUsed > 0 && data->rec[data->recUsed - 1].refCount == 0)
      {
        Data::Rec &last = data->rec[data->recUsed - 1];
        trimmedBytes += last.capacity;
        data->memUsed -= last.capacity;
        last.offs = last.sz = last.capacity = 0;
        last.tag = 0;
        last.ready = 0; // a freed slot is fully cleared: its next occupant must not look ready mid-fill
        data->recUsed--;
        trimmedRecs++;
      }
      data->mark_rec_ro();
      logmessage(_MAKE4C('SHMM'), "trimmed %d trailing unused rec(s), %dK (trigger: %s), now mem %lluK/%lluK, rec=%d/%d", //
        trimmedRecs, (int)(trimmedBytes >> 10), nearFull ? "near full" : "would fail", ((uint64_t)data->memUsed) >> 10,
        ((uint64_t)data->memSz) >> 10, data->recUsed, data->maxRecNum);
    }

    if (data->recUsed < data->maxRecNum && data->memUsed + sz <= data->memSz)
    {
      data->mark_rec_rw();
      Data::Rec &r = data->rec[data->recUsed];
      strncpy(r.name, ptr_fname, sizeof(r.name) - 1);
      r.tag = tag;
      r.offs = data->memUsed;
      r.sz = orig_sz;
      r.capacity = sz;
      r.refCount = 1;
      data->memUsed += sz;
      data->recUsed++;
      data->mark_rec_ro();
      return pointerToWritableArea(r);
    }

    logmessage(_MAKE4C('SHMM'), "failed to alloc ptr in shared mem: mem(rec=%d/%d,sz=%lluK), ptr(sz=%llu)", data->recUsed,
      data->maxRecNum, ((uint64_t)data->memSz) >> 10, (uint64_t)sz);
    return NULL;
  }
  void markPtrDataReady(void *p)
  {
    if (!data)
      return;
    int64_t offs = ptrToOffs(p);
    ScopedLock lock(mutex, localMutex);
    for (auto &r : data->usedRec())
      if (r.offs == offs)
      {
        data->mark_rec_rw();
        r.ready = 1;
        data->mark_rec_ro();
        break;
      }
  }
  void releasePtr(uint32_t tag, void *p)
  {
    if (!data)
      return;
    int64_t offs = ptrToOffs(p);
    ScopedLock lock(mutex, localMutex);
    for (auto &r : data->usedRec())
      if (r.offs == offs)
      {
        if (r.tag != tag)
          return logerr("rec[%d].tag=%c%c%c%c != %c%c%c%c ptr=%p", data->index(r), _DUMP4C(r.tag), _DUMP4C(tag), p);
        if (r.refCount <= 0)
          return logerr("rec[%d].tag=%c%c%c%c .refCount=%d ptr=%p", data->index(r), _DUMP4C(r.tag), r.refCount, p);

        data->mark_rec_rw();
        r.refCount--;
        if (localMem && !r.refCount)
        {
          void *ptr = offsToPtr(r.offs);
          logmessage(_MAKE4C('SHMM'), "releasing ptr=%p,%dK tag=%c%c%c%c with refCount=0", ptr, r.capacity >> 10, _DUMP4C(r.tag));
          mark_global_shared_mem_readonly(ptr, r.capacity, false);
          localMem->freeAligned(ptr);
          data->memUsed -= r.capacity;
          r.offs = r.sz = r.capacity = 0;
          r.tag = 0;
          r.ready = 0; // a freed slot is fully cleared: its next occupant must not look ready mid-fill
          if (data->index(r) + 1 == data->recUsed)
            while (data->recUsed >= 1 && !data->rec[data->recUsed - 1].offs)
              data->recUsed--;
        }
        data->mark_rec_ro();
        break;
      }
  }

  void dumpContents(const char *label)
  {
    if (!data)
      return;
    ScopedLock lock(mutex, localMutex);
    dumpContentsInternal(label);
  }

protected:
  // written under the storage lock, read unsynchronized by the accessors: construction, teardown
  // and consumers share the main thread
  Data *data = nullptr;
  intptr_t fd = -1;
  void *mutex = nullptr;
  IMemAlloc *localMem = nullptr;
  WinCritSec *localMutex = nullptr;
  char sharedName[256 - sizeof(void *) * 5] = {0};

  int64_t ptrToOffs(const void *p) const { return (int64_t)(uintptr_t)p - (localMem ? 0 : (int64_t)(uintptr_t)data); }
  void *offsToPtr(int64_t offs) const { return (void *)uintptr_t(int64_t(localMem ? 0 : uintptr_t(data)) + offs); }

  //! claims r (the ref stops allocPtr from reusing the slot), releases the lock, then waits for
  //! the publisher to mark the record ready; null (claim dropped) if it never becomes ready
  void *claimAndWaitReady(Data::Rec &r, const char *ptr_fname, uint32_t tag, ScopedLock &lock)
  {
    data->mark_rec_rw();
    r.refCount++;
    data->mark_rec_ro();
    void *ptr = offsToPtr(r.offs);
    lock.release();

    for (int c = 0; c < 1000; c++)
      if (r.ready)
        break;
      else
      {
        sleep_msec(10);
        if (c > 10 && (c % 100) == 0)
          logmessage(_MAKE4C('SHMM'), "waiting for %s, tag=%u data ready (%d sec)...", ptr_fname, tag, c / 100);
      }
    if (r.ready)
      return ptr;

    releasePtr(tag, ptr); // never became ready (leader failed) - drop our claim, nothing to return
    return nullptr;
  }

  void *pointerToWritableArea(const Data::Rec &r)
  {
    void *ptr = offsToPtr(r.offs);
    mark_global_shared_mem_readonly(ptr, r.capacity, false);
    return ptr;
  }

  void dumpContentsInternal(const char *label)
  {
    logmessage(_MAKE4C('SHMM'), "[%s] dumping contents of %s shared mem: {%s} (%p, %uK, refs=%d), %dK used in %d/%d records", label,
      localMem ? "local" : "global", sharedName, data, (data->memSz ? data->memSz : Data::calcSizeof(data->maxRecNum)) >> 10,
      data->refCount, data->memUsed >> 10, data->recUsed, data->maxRecNum);
    for (auto &r : data->usedRec())
      if (r.offs)
        debug("  rec[%2d] tag=%c%c%c%c ptr=(%p, %6dK/%6dK cap) refCount=%d name={%s}", data->index(r), _DUMP4C(r.tag),
          offsToPtr(r.offs), r.sz >> 10, r.capacity >> 10, r.refCount, r.name);
  }
};
#endif
