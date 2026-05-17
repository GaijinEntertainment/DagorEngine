//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqasync.h>
#include <squirrel.h>


namespace sqasync_helpers
{

// RAII handle around a freshly-created Promise instance.
// Replaces the per-binding push-class / sq_call / sq_remove / sq_addref dance
// and the per-feature `bool dead` flag for VMs that may be torn down before
// an in-flight async op completes.
class PromiseHolder
{
public:
  // On runtime-not-bound or OOM, leaves instance() null and the VM's last
  // error set; resolve/reject become no-ops.
  explicit PromiseHolder(HSQUIRRELVM v) : vmHandle(v)
  {
    sq_resetobject(&promise);
    if (sqasync::create_promise(v, &promise) != SQ_OK)
      sq_resetobject(&promise);
  }

  ~PromiseHolder()
  {
    if (!sq_isnull(promise))
      sq_release(vmHandle, &promise);
  }

  PromiseHolder(const PromiseHolder &) = delete;
  PromiseHolder &operator=(const PromiseHolder &) = delete;
  PromiseHolder(PromiseHolder &&) = delete;
  PromiseHolder &operator=(PromiseHolder &&) = delete;

  // No-op if already settled or after onVmShutdown().
  void resolve(const HSQOBJECT &value)
  {
    if (sq_isnull(promise))
      return;
    sqasync::resolve_promise(vmHandle, promise, value);
    sq_release(vmHandle, &promise);
    sq_resetobject(&promise);
  }

  void reject(const HSQOBJECT &reason)
  {
    if (sq_isnull(promise))
      return;
    sqasync::reject_promise(vmHandle, promise, reason);
    sq_release(vmHandle, &promise);
    sq_resetobject(&promise);
  }

  void resolveNull()
  {
    HSQOBJECT n;
    sq_resetobject(&n);
    resolve(n);
  }

  // Called by the embedder before sq_close so any later resolve/reject from
  // a still-in-flight async op short-circuits without touching freed memory.
  void onVmShutdown()
  {
    if (sq_isnull(promise))
      return;
    sq_release(vmHandle, &promise);
    sq_resetobject(&promise);
  }

  // sq_isnull() iff construction failed or the holder has settled / been disowned.
  const HSQOBJECT &instance() const { return promise; }
  HSQUIRRELVM vm() const { return vmHandle; }

private:
  HSQUIRRELVM vmHandle = nullptr;
  HSQOBJECT promise{};
};

} // namespace sqasync_helpers
