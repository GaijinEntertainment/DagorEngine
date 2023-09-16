//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>

#include <supp/dag_define_COREIMP.h>

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

#include <supp/dag_undef_COREIMP.h>


#ifdef __cplusplus
#include <osApiWrappers/dag_globalMutex.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_log.h>
#include <string.h>

class GlobalSharedMemStorage
{
public:
  struct Data
  {
    struct Rec
    {
      int64_t offs;
      int64_t sz;
      uint32_t tag;
      volatile uint32_t ready;
      char name[256 - sizeof(int64_t) * 2 - sizeof(int32_t) * 2];

      bool cmpEq(const char *nm, int32_t _tag) const { return tag == _tag && strcmp(nm, name) == 0; }
      void *getPtr(void *base) { return (void *)(uintptr_t)((int64_t)(uintptr_t)base + offs); }
    };

    int64_t baseAddr;
    int64_t memSz;
    int32_t refCount;
    int32_t maxRecNum;
    int64_t memUsed;
    int32_t recUsed;
    int32_t _resv[7];
    Rec rec[1]; //< variable size array

    static int calcSizeof(int rec_num) { return ((sizeof(Data) + sizeof(Rec) * (rec_num - 1)) + 0xFFF) & ~0xFFF; }
  };

public:
  GlobalSharedMemStorage() : data(NULL), fd(-1), mutex(NULL) { memset(sharedName, 0, sizeof(sharedName)); }

  GlobalSharedMemStorage(const char *shared_name, size_t sz, int max_rec_num) : data(NULL), fd(-1), mutex(NULL)
  {
    memset(sharedName, 0, sizeof(sharedName));
    init(shared_name, sz, max_rec_num);
  }
  ~GlobalSharedMemStorage() { term(); }

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
      return;
    global_mutex_enter(mutex);
    bool need_unlink = false;
    if (data)
    {
      mark_global_shared_mem_readonly(data, Data::calcSizeof(data->maxRecNum), false);
      data->refCount--;
      if (!data->refCount)
        need_unlink = true;
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

  const char *getSharedName() const { return sharedName; }
  int getRefCount() const { return data ? data->refCount : 0; }
  bool doesPtrBelong(void *p) const
  {
    if (!data)
      return false;
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
    int64_t offs = (int64_t)(uintptr_t)p - (int64_t)(uintptr_t)data;
    global_mutex_enter(mutex);
    for (int i = 0; i < data->recUsed; i++)
      if (data->rec[i].offs == offs)
      {
        global_mutex_leave(mutex);
        return data->rec[i].sz;
      }
    global_mutex_leave(mutex);
    return 0;
  }


  void *findPtr(const char *ptr_fname, uint32_t tag)
  {
    if (!data)
      return NULL;
    global_mutex_enter(mutex);
    for (int i = 0; i < data->recUsed; i++)
      if (data->rec[i].cmpEq(ptr_fname, tag))
      {
        global_mutex_leave(mutex);
        for (int c = 0; c < 1000; c++)
          if (data->rec[i].ready)
            break;
          else
          {
            sleep_msec(10);
            if (c > 10 && (c % 100) == 0)
              logmessage(_MAKE4C('SHMM'), "waiting for %s, tag=%u data ready (%d sec)...", ptr_fname, tag, c / 100);
          }
        return data->rec[i].ready ? data->rec[i].getPtr(data) : NULL;
      }
    global_mutex_leave(mutex);
    return NULL;
  }
  void *allocPtr(const char *ptr_fname, uint32_t tag, size_t sz)
  {
    if (!data)
      return NULL;
    global_mutex_enter(mutex);
    for (int i = 0; i < data->recUsed; i++)
      if (data->rec[i].cmpEq(ptr_fname, tag))
      {
        global_mutex_leave(mutex);
        return data->rec[i].ready ? data->rec[i].getPtr(data) : NULL;
      }

    size_t orig_sz = sz;
    sz = (sz + 0xFFF) & ~0xFFF; // 4K align for pages
    if (data->recUsed < data->maxRecNum && data->memUsed + sz <= data->memSz)
    {
      mark_global_shared_mem_readonly(data, Data::calcSizeof(data->maxRecNum), false);
      Data::Rec &r = data->rec[data->recUsed];
      strncpy(r.name, ptr_fname, sizeof(r.name) - 1);
      r.tag = tag;
      r.offs = data->memUsed;
      r.sz = orig_sz;
      data->memUsed += sz;
      data->recUsed++;
      mark_global_shared_mem_readonly(data, Data::calcSizeof(data->maxRecNum), true);
      global_mutex_leave(mutex);
      return r.getPtr(data);
    }

    logmessage(_MAKE4C('SHMM'), "failed to alloc ptr in shared mem: mem(rec=%d/%d,sz=%lluK), ptr(sz=%llu)", data->recUsed,
      data->maxRecNum, ((uint64_t)data->memSz) >> 10, (uint64_t)sz);
    global_mutex_leave(mutex);
    return NULL;
  }
  void markPtrDataReady(void *p)
  {
    if (!data)
      return;
    int64_t offs = (int64_t)(uintptr_t)p - (int64_t)(uintptr_t)data;
    global_mutex_enter(mutex);
    for (int i = 0; i < data->recUsed; i++)
      if (data->rec[i].offs == offs)
      {
        mark_global_shared_mem_readonly(data, Data::calcSizeof(data->maxRecNum), false);
        data->rec[i].ready = 1;
        mark_global_shared_mem_readonly(data, Data::calcSizeof(data->maxRecNum), true);
        break;
      }
    global_mutex_leave(mutex);
  }

protected:
  Data *data;
  intptr_t fd;
  void *mutex;
  char sharedName[512 - sizeof(void *) * 3 - 8];
};
#endif
