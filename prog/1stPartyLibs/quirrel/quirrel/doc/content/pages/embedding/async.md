---
title: The async runtime
group: Embedding
order: 112
summary: Installing the async runtime, and pumping it once per frame.
---

Script-side [async and await](page:language/async) is a small cooperative
scheduler. Its C entry points are in `sqasync.h`. The scheduler does not drive
itself, so a host has three duties. If the host skips the third, every async
function starts and never finishes.

1. **Bind it**, once per shared state, before any `async` function runs:
   `sqasync::bind(v)`.
2. **Publish the Future class** to script, through
   `SqModules::registerAsyncLib` or `sqasync::sqasync_register`.
3. **Pump it**, once per host frame: `sqasync::pump(v)`.

## Pumping

`pump` drains the inbox into the step queue and runs both. It returns the number
of callbacks it invoked. Its step cap bounds how long one call can take when a
callback posts another callback, so a busy frame cannot starve rendering.

A zero return does not mean nothing happened. A pump with an empty queue may
still report deferred unhandled async faults through the VM error handler before
it returns. This is where a rejected Future that nobody awaited surfaces.

`sqasync::has_pending` reports whether the runtime has work it can advance on
its own. It is the stop condition for a drain loop, but only together with the
host's other signals. A task parked on a Future that nothing will settle is not
pending, because it cannot advance without outside input. Drain until
`has_pending` is false, and no further, so that shutdown terminates and does not
spin.

## Threading

The unit of affinity is the VM, not the process. Each VM owns a step queue that
only its own thread touches, and an inbox protected by a mutex that any thread
may push into. The thread that calls `pump(v)` drains both. Different VMs may be
pumped on different threads.

This split is why there are two posting primitives:

- `sqasync::post_on_vm_thread` is lock free and must be called from the thread
  that pumps that VM.
- `sqasync::post_from_any_thread` takes the inbox lock and can be called from
  any thread. A worker thread uses it to hand a finished result back.

A call to the first from the wrong thread is a data race, not an error return.
Be sure which thread a callback runs on before you choose.

## Settling a Future from C

`sqasync::future_create` makes a Future and writes an addref'd handle out. The
host holds the handle while the work is in flight. `future_resolve` and
`future_throw` settle the Future, and the task that was awaiting resumes on the
next pump. Release the handle once the Future is settled.

## Shutting down

Call `sqasync::stop_tasks(v)` during the VM teardown, before any later pump.
After this call no task step runs, so script does not see host state that is
partly gone. Callbacks still run, so native code can drain its posts.

The full list, with the header's own notes, is on
[Async runtime](page:capi/async).
