//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <osApiWrappers/dag_critSec.h>
template <class T>
class TabWithLock : public Tab<T>
{
protected:
  CritSecStorage critSec;
  friend class AutoLock;

public:
  class AutoLock
  {
  private:
    void *pCritSec;
    AutoLock(const AutoLock &) = delete;
    AutoLock &operator=(const AutoLock &) = delete;
    friend class TabWithLock;

  public:
    AutoLock(TabWithLock &tab) : pCritSec(tab.critSec) { ::enter_critical_section(pCritSec); }
    ~AutoLock() { ::leave_critical_section(pCritSec); }
  };
  TabWithLock(IMemAlloc *m = tmpmem_ptr()) : Tab<T>(m) { ::create_critical_section(&critSec); }
  TabWithLock(const TabWithLock &) = delete; // CS is non copyable
  ~TabWithLock() { ::destroy_critical_section(critSec); }
  void lock() { ::enter_critical_section(critSec); }
  void unlock() { ::leave_critical_section(critSec); }
};
