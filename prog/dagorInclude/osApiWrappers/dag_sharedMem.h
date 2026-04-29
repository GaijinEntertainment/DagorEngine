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
      int64_t sz;
      uint32_t tag;
      uint16_t refCount;
      volatile uint16_t ready;
      char name[256 - sizeof(int64_t) * 2 - sizeof(int32_t) * 2];

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
          if (r.refCount != 0)
          {
            logerr("data->rec[%d].refCount=%d tag=%c%c%c%c ptr=%p,%dK", &r - data->rec, r.refCount, _DUMP4C(r.tag), ptr, r.sz >> 10);
            continue;
          }
          mark_global_shared_mem_readonly(ptr, r.sz, false);
          localMem->freeAligned(ptr);
          r.offs = r.sz = 0;
          r.tag = 0;
        }
      data->recUsed = 0;
      localMem->freeAligned(data);
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
  bool doesPtrBelong(void *p) const
  {
    if (!data)
      return false;
    if (localMem)
    {
      for (auto &r : data->usedRec())
        if (r.offs && intptr_t(p) >= r.offs && intptr_t(p) < r.offs + r.sz)
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
      {
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
        if (!r.ready)
          return nullptr;
        ScopedLock lock2(mutex, localMutex);
        data->mark_rec_rw();
        r.refCount++;
        data->mark_rec_ro();
        return offsToPtr(r.offs);
      }
    return NULL;
  }
  void *allocPtr(const char *ptr_fname, uint32_t tag, size_t sz)
  {
    if (!data)
      return NULL;
    ScopedLock lock(mutex, localMutex);
    for (auto &r : data->usedRec())
      if (r.cmpEq(ptr_fname, tag))
      {
        if (!r.ready)
          return nullptr;
        data->mark_rec_rw();
        r.refCount++;
        data->mark_rec_ro();
        return offsToPtr(r.offs);
      }

    size_t orig_sz = sz;
    sz = (sz + 0xFFF) & ~0xFFF; // 4K align for pages

    if (localMem)
    {
      for (auto &r : data->allRec())
        if (!r.offs)
        {
          sz = (sz + 0xFFF) & ~0xFFF;
          auto ptr = (int64_t)(uintptr_t)localMem->allocAligned(sz, 4096);
          if (!ptr)
            break;
          data->mark_rec_rw();
          r.offs = ptr;
          strncpy(r.name, ptr_fname, sizeof(r.name) - 1);
          r.tag = tag;
          r.sz = sz;
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

    if (data->recUsed < data->maxRecNum && data->memUsed + sz <= data->memSz)
    {
      data->mark_rec_rw();
      Data::Rec &r = data->rec[data->recUsed];
      strncpy(r.name, ptr_fname, sizeof(r.name) - 1);
      r.tag = tag;
      r.offs = data->memUsed;
      r.sz = orig_sz;
      r.refCount = 1;
      data->memUsed += sz;
      data->recUsed++;
      data->mark_rec_ro();
      return offsToPtr(r.offs);
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
          logmessage(_MAKE4C('SHMM'), "releasing ptr=%p,%dK tag=%c%c%c%c with refCount=0", ptr, r.sz >> 10, _DUMP4C(r.tag));
          mark_global_shared_mem_readonly(ptr, r.sz, false);
          localMem->freeAligned(ptr);
          data->memUsed -= (r.sz + 0xFFF) & ~0xFFF;
          r.offs = r.sz = 0;
          r.tag = 0;
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
  Data *data = nullptr;
  intptr_t fd = -1;
  void *mutex = nullptr;
  IMemAlloc *localMem = nullptr;
  WinCritSec *localMutex = nullptr;
  char sharedName[256 - sizeof(void *) * 5] = {0};

  int64_t ptrToOffs(const void *p) const { return (int64_t)(uintptr_t)p - (localMem ? 0 : (int64_t)(uintptr_t)data); }
  void *offsToPtr(int64_t offs) const { return (void *)uintptr_t(int64_t(localMem ? 0 : uintptr_t(data)) + offs); }

  void dumpContentsInternal(const char *label)
  {
    logmessage(_MAKE4C('SHMM'), "[%s] dumping contents of %s shared mem: {%s} (%p, %uK, refs=%d), %dK used in %d/%d records", label,
      localMem ? "local" : "global", sharedName, data, (data->memSz ? data->memSz : Data::calcSizeof(data->maxRecNum)) >> 10,
      data->refCount, data->memUsed >> 10, data->recUsed, data->maxRecNum);
    for (auto &r : data->usedRec())
      if (r.offs)
        debug("  rec[%2d] tag=%c%c%c%c ptr=(%p, %6dK) refCount=%d name={%s}", data->index(r), _DUMP4C(r.tag), offsToPtr(r.offs),
          r.sz >> 10, r.refCount, r.name);
  }
};
#endif
