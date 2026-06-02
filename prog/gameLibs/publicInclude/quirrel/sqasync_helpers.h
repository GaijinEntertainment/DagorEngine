//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqasync.h>
#include <squirrel.h>


namespace sqasync_helpers
{

// RAII handle around a freshly-created Future instance.
// Replaces the per-binding push-class / sq_call / sq_remove / sq_addref dance
// and the per-feature `bool dead` flag for VMs that may be torn down before
// an in-flight async op completes.
class FutureHolder
{
public:
  // On runtime-not-bound or OOM, leaves instance() null and the VM's last
  // error set; resolve / throwFault become no-ops.
  explicit FutureHolder(HSQUIRRELVM v) : vmHandle(v)
  {
    sq_resetobject(&future);
    if (sqasync::future_create(v, &future) != SQ_OK)
      sq_resetobject(&future);
  }

  ~FutureHolder()
  {
    if (!sq_isnull(future))
      sq_release(vmHandle, &future);
  }

  FutureHolder(const FutureHolder &) = delete;
  FutureHolder &operator=(const FutureHolder &) = delete;
  FutureHolder(FutureHolder &&) = delete;
  FutureHolder &operator=(FutureHolder &&) = delete;

  // No-op if already settled or after onVmShutdown().
  void resolve(const HSQOBJECT &value)
  {
    if (sq_isnull(future))
      return;
    sqasync::future_resolve(vmHandle, future, value);
    sq_release(vmHandle, &future);
    sq_resetobject(&future);
  }

  // Worker-thread-bug channel. Documented failures must travel as values
  // through resolve(); reserve throwFault() for invariant violations the
  // binding's contract does not document.
  void throwFault(const HSQOBJECT &value)
  {
    if (sq_isnull(future))
      return;
    sqasync::future_throw(vmHandle, future, value);
    sq_release(vmHandle, &future);
    sq_resetobject(&future);
  }

  void resolveNull()
  {
    HSQOBJECT n;
    sq_resetobject(&n);
    resolve(n);
  }

  // Called by the embedder before sq_close so any later resolve / throwFault
  // from a still-in-flight async op short-circuits without touching freed memory.
  void onVmShutdown()
  {
    if (sq_isnull(future))
      return;
    sq_release(vmHandle, &future);
    sq_resetobject(&future);
  }

  // sq_isnull() iff construction failed or the holder has settled / been disowned.
  const HSQOBJECT &instance() const { return future; }
  HSQUIRRELVM vm() const { return vmHandle; }

private:
  HSQUIRRELVM vmHandle = nullptr;
  HSQOBJECT future{};
};

} // namespace sqasync_helpers
