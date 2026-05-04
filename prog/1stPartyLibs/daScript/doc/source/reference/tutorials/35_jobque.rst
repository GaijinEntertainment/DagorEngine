.. _tutorial_jobque:

==========================================
Job Queue (jobque)
==========================================

.. index::
    single: Tutorial; Job Queue
    single: Tutorial; jobque
    single: Tutorial; Multithreading
    single: Tutorial; Channels
    single: Tutorial; Parallel For
    single: Tutorial; LockBox

This tutorial covers the ``jobque_boost`` module — daslang's built-in
multi-threading primitives.  The module provides a work-stealing thread pool,
typed channels for inter-thread communication, job statuses and wait groups
for synchronization, lock boxes for shared mutable state, and high-level
helpers like ``parallel_for``.

.. code-block:: das

    require daslib/jobque_boost

Core concepts
=============

* **Job queue** — a work-stealing thread pool initialized by ``with_job_que``.
  All multi-threading operations must be performed inside this scope.
* **Job** — a unit of work dispatched to the thread pool via ``new_job``.
  Each job runs in a *cloned* context — it does not share memory with the
  caller.
* **Channel** — a thread-safe FIFO queue for passing typed struct data between
  jobs.  Created with ``with_channel(count)`` where ``count`` is the expected
  number of ``notify_and_release`` calls before the channel closes.
* **JobStatus** — a counter that tracks asynchronous completion.
  ``notify_and_release`` decrements it, ``join`` blocks until it reaches zero.
* **Wait group** — ``with_wait_group`` wraps a ``JobStatus`` with automatic
  ``join`` on scope exit.  ``done`` is an alias for ``notify_and_release``.
* **LockBox** — a mutex-protected single-value container.  ``set`` stores a
  value, ``get`` reads it inside a block.
* **Thread** — ``new_thread`` spawns a dedicated OS thread, outside the
  thread pool.

Channel and lock-box data **must be a struct** (or handled type) — primitives
cannot be heap-allocated.  Wrap them in a struct when needed.

Starting the job queue
======================

``with_job_que`` initializes the thread pool and runs the block.  All
job-queue operations must happen inside this scope:

.. code-block:: das

    with_job_que() {
        print("Job queue is active\n")
    }
    // output: Job queue is active

Spawning jobs
=============

``new_job`` dispatches work to the thread pool.  Each job runs in a cloned
context.  Use channels to communicate results back:

.. code-block:: das

    with_job_que() {
        with_channel(1) $(ch) {
            new_job() @() {
                ch |> push_clone(IntVal(v = 42))
                ch |> notify_and_release
            }
            ch |> for_each_clone() $(val : IntVal#) {
                print("Received from job: {val.v}\n")
            }
        }
    }
    // output: Received from job: 42

Channels
========

A ``Channel`` is a thread-safe FIFO queue.  Create one with
``with_channel(expected_count)`` where the count matches the number of
``notify_and_release`` calls that will be made.  ``push_clone`` sends data
and ``for_each_clone`` receives it, blocking until the channel is drained:

.. code-block:: das

    with_job_que() {
        with_channel(2) $(ch) {
            new_job() @() {
                ch |> push_clone(StringVal(s = "hello"))
                ch |> notify_and_release
            }
            new_job() @() {
                ch |> push_clone(StringVal(s = "world"))
                ch |> notify_and_release
            }
            var messages : array<string>
            ch |> for_each_clone() $(msg : StringVal#) {
                messages |> push(clone_string(msg.s))
            }
            sort(messages)
            for (m in messages) {
                print("  {m}\n")
            }
        }
    }
    // output:
    //   hello
    //   world

Channels work with any struct type — not just simple wrappers:

.. code-block:: das

    struct WorkResult {
        index : int
        value : int
    }

    with_channel(1) $(ch) {
        new_job() @() {
            for (i in range(3)) {
                ch |> push_clone(WorkResult(index = i, value = i * i))
            }
            ch |> notify_and_release
        }
        ch |> for_each_clone() $(r : WorkResult#) {
            print("  [{r.index}] = {r.value}\n")
        }
    }
    // output:
    //   [0] = 0
    //   [1] = 1
    //   [2] = 4

Multiple producers
==================

When multiple jobs push to the same channel, set the channel count to the
number of producers.  ``for_each_clone`` blocks until all have called
``notify_and_release``:

.. code-block:: das

    let num_producers = 3
    with_channel(num_producers) $(ch) {
        for (p in range(num_producers)) {
            new_job() @() {
                ch |> push_clone(IntVal(v = p * 100))
                ch |> notify_and_release
            }
        }
        var results : array<int>
        ch |> for_each_clone() $(val : IntVal#) {
            results |> push(val.v)
        }
        sort(results)
        print("producers: {results}\n")
    }
    // output: producers: [0, 100, 200]

JobStatus
=========

A ``JobStatus`` tracks completion of asynchronous work.
``with_job_status(count)`` creates a status expecting ``count`` notifications.
Each call to ``notify_and_release`` decrements the counter.  ``join`` blocks
until all notifications arrive:

.. code-block:: das

    with_job_que() {
        with_job_status(3) $(status) {
            for (i in range(3)) {
                new_job() @() {
                    status |> notify_and_release
                }
            }
            status |> join
        }
        print("All 3 jobs finished\n")
    }
    // output: All 3 jobs finished

Wait groups
===========

``with_wait_group`` is a convenience wrapper — it combines
``with_job_status`` and ``join``.  ``done`` is an alias for
``notify_and_release``:

.. code-block:: das

    with_job_que() {
        with_wait_group(3) $(wg) {
            for (i in range(3)) {
                new_job() @() {
                    wg |> done
                }
            }
        }
        print("all jobs complete\n")
    }
    // output: all jobs complete

LockBox
=======

A ``LockBox`` holds a single struct value protected by a mutex.  ``set``
stores a value and ``get`` reads it inside a block.  These operations must
run inside a job context (``new_job`` or ``new_thread``):

.. code-block:: das

    struct BoxCounter {
        value : int
    }

    with_job_que() {
        with_lock_box() $(box) {
            with_channel(1) $(ch) {
                new_job() @() {
                    box |> set(BoxCounter(value = 42))
                    box |> get() $(c : BoxCounter#) {
                        ch |> push_clone(IntVal(v = c.value))
                    }
                    box |> release
                    ch |> notify_and_release
                }
                ch |> for_each_clone() $(val : IntVal#) {
                    print("lockbox value: {val.v}\n")
                }
            }
        }
    }
    // output: lockbox value: 42

parallel_for
============

``parallel_for`` splits a range ``[begin..end)`` into chunks and runs them on
the job queue.  A macro automatically wraps the block body in ``new_job`` — you
write sequential code inside:

.. code-block:: das

    with_job_que() {
        let num_jobs = 3
        with_channel(num_jobs) $(ch) {
            parallel_for(0, 10, num_jobs) $(job_begin, job_end, wg) {
                for (i in range(job_begin, job_end)) {
                    ch |> push_clone(IntVal(v = i * i))
                }
                ch |> notify_and_release
                wg |> done
            }
            var results : array<int>
            ch |> for_each_clone() $(val : IntVal#) {
                results |> push(val.v)
            }
            sort(results)
            print("squares: {results}\n")
        }
    }
    // output: squares: [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]

The block receives three parameters:

* ``job_begin``, ``job_end`` — the index sub-range for this chunk
* ``wg`` — a ``JobStatus?`` that must be signaled via ``done(wg)`` when the
  chunk finishes

When using a channel inside ``parallel_for``, set the channel count to
``num_jobs`` since each chunk calls ``notify_and_release`` once.

new_thread
==========

``new_thread`` creates a dedicated OS thread outside the thread pool.  Use it
for long-lived work that should not block the job queue:

.. code-block:: das

    with_job_que() {
        with_channel(1) $(ch) {
            new_thread() @() {
                ch |> push_clone(StringVal(s = "from thread"))
                ch |> notify_and_release
            }
            ch |> for_each_clone() $(msg : StringVal#) {
                print("{msg.s}\n")
            }
        }
    }
    // output: from thread

.. seealso::

   Full source: :download:`tutorials/language/35_jobque.das <../../../../tutorials/language/35_jobque.das>`

   Previous tutorial: :ref:`tutorial_decs`

   Next tutorial: :ref:`tutorial_pointers`

   :ref:`Lambda <lambdas>` — lambda language reference.

   :ref:`Blocks <blocks>` — block language reference.
