#pragma once

#include "riGen/riGenExtra.h"


namespace rendinst
{

struct ScopedRIExtraWriteTryLock
{
  ScopedRIExtraWriteTryLock(bool do_not_wait = false)
  {
    if (do_not_wait)
      locked = rendinst::ccExtra.tryLockWrite();
    else
    {
      rendinst::ccExtra.lockWrite();
      locked = true;
    }
  }
  ~ScopedRIExtraWriteTryLock()
  {
    if (isLocked())
      rendinst::ccExtra.unlockWrite();
  }
  bool isLocked() { return locked; }

protected:
  bool locked = false;
};

} // namespace rendinst