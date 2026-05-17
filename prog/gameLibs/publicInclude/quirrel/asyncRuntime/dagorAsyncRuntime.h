//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqasync.h>

class SqModules;


namespace sqasync_dagor
{

// Per-VM async runtime: owns the sqasync binding, the "async" SqModules
// native module, and the MT-safe timer queue backing `async.delay`. Expired
// timers are posted to sqasync's inbox (not invoked synchronously by pump).
//
// Lifetime: shutdown() should run before sq_close so timer callbacks release
// VM refs via the still-live inbox; the scope itself must outlive sq_close
// because the `async.delay` closure holds a pointer to it. One scope per
// VM lifecycle -- do not reuse across sq_open / sq_close.
class AsyncRuntimeScope
{
public:
  // Installs sqasync::bind + registers the "async" SqModules native module.
  // Do NOT also call SqModules::registerAsyncLib() on the same VM.
  explicit AsyncRuntimeScope(SqModules *module_mgr);
  ~AsyncRuntimeScope();

  AsyncRuntimeScope(const AsyncRuntimeScope &) = delete;
  AsyncRuntimeScope &operator=(const AsyncRuntimeScope &) = delete;

  // Drain pending timers and disable async.delay / postDelayed. Must be
  // called from the VM thread, BEFORE sq_close. Idempotent.
  void shutdown();

  // Must be called from the VM thread. Returns the number of sqasync
  // callbacks invoked (not the number of timers posted).
  SQInteger pump(SQInteger max_steps = 1024);

  // Schedule fn(user) on the VM thread after delay_sec seconds (<= 0 fires
  // at next pump). Safe to call from any thread. Returns SQ_ERROR if the
  // scope has been shut down; on failure the caller owns `user` and must
  // free it (mirrors sqasync::post_on_vm_thread).
  SQRESULT postDelayed(double delay_sec, sqasync::VmCallback fn, void *user);

  bool hasPendingTimers() const;

private:
  struct Impl;
  Impl *impl;
};

} // namespace sqasync_dagor
