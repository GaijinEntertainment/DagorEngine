# src/builtin - the job queue

Companion to `ARCHITECTURE.md`; this document carries one section, `ARCHITECTURE_JOBQUE.md#jobque-team-panic`,
cited by its anchor.

### A job's panic is the caller's {#jobque-team-panic}

Every job body runs under the same catch (`runWithCatch`), whichever way it was dispatched, and its
panic reaches the caller as the caller's own panic, `JOB EXCEPTION: <what> at <where>` - recoverable
by a `try/recover` around the dispatch for a graceful shutdown, and an exit code of 1 where nothing
catches it. The re-raise always happens where the caller's frame holds nothing a longjmp would skip:
with exceptions off a panic is a longjmp that runs no destructor, so the clones, the guards and the
recorder live in an inner scope and the message crosses it in a stack buffer.

A team dispatch - `parallel_for` in team mode, its indexed form and the multi-stage chain - runs its
chunks on worker clones and on the caller's own slot; the first panic is recorded under a mutex, the
failed context's exception is cleared, since the same context - a fork, or the caller itself - serves
the op's next chunk on its slot and a stop flag left by the interrupted lambda would cut that chunk
short, the team runs to completion, the clones are cleaned up, and the caller re-raises. A panic on
the caller's slot never longjmps out of the team callback mid-run: that left the workers waiting on a
completion that never came, and the stage chain's dispatch mutex held.

A fifo job (`new_job`, `parallel_for` off team mode) panics on a worker with no caller frame under it:
the worker records the first panic and clears its context, notifies and releases every wait group the
job's lambda captured and had not released itself - the panic skipped the job's own
`notify_and_release`, and a join on that group would otherwise never return - and the job's siblings
run on. The caller re-raises when its enclosing `with_job_status` block ends - which is where
`parallel_for` returns - or when the `with_job_que` block ends, before the drain's own complaint. A
`join` never raises: it may run inside a block whose guard a longjmp would skip. A detached
`new_thread` has no join point to report at, so its panic reports on the thread and ends the process
there, as before.
