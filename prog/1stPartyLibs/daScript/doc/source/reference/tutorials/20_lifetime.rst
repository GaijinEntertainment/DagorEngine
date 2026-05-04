.. _tutorial_lifetime:

========================
Lifetime and Cleanup
========================

.. index::
    single: Tutorial; Lifetime
    single: Tutorial; Finalizer
    single: Tutorial; Delete
    single: Tutorial; Inscope

This tutorial covers daslang's memory model, explicit cleanup with
``delete``, automatic cleanup with ``var inscope``, custom finalizers,
and ``finally`` blocks.

Memory model
=============

daslang does **not** automatically clean up local variables when they go
out of scope. There is no implicit destructor call. Containers (arrays,
tables) will have their memory reclaimed eventually, but finalizers are
not called automatically.

For deterministic cleanup (closing files, releasing locks), use:

- ``delete`` — explicit immediate cleanup
- ``var inscope`` — automatic cleanup at end of scope

For simple programs, you often don't need either — the process reclaims
all memory on exit. But for long-running applications or resource
management, use the tools below.

Explicit delete
===============

``delete`` zeroes a variable and calls its finalizer. For containers it
frees all elements::

  var data <- [1, 2, 3]
  delete data           // data is now empty

Custom finalizers
=================

Define a ``finalize`` function for custom cleanup logic::

  struct Resource {
      name : string
      id : int
  }

  def finalize(var r : Resource) {
      print("cleanup: {r.name}\n")
  }

  var res = Resource(name="DB", id=1)
  delete res            // prints "cleanup: DB"

var inscope
===========

``var inscope`` automatically calls ``delete`` at the end of the enclosing
block — like RAII in C++ or ``defer`` in Go. This is the recommended
approach for deterministic cleanup::

  if (true) {
      var inscope r1 = acquire("File", 2)
      var inscope r2 = acquire("Socket", 3)
      // ... use resources ...
      // delete r2, then delete r1, called automatically (reverse order)
  }

Use ``inscope`` for file handles, connections, locks, or any resource
that needs prompt release.

finally blocks
==============

``finally`` runs cleanup code when a block exits::

  for (i in range(3)) {
      counter += 1
  } finally {
      print("done, counter={counter}\n")
  }

Heap pointers
=============

For class instances created with ``new``, ``delete`` requires ``unsafe``::

  var p = new MyClass()
  // ... use p ...
  unsafe { delete p }

Or use ``var inscope`` for automatic cleanup::

  unsafe { // needs 'unsafe', because deleting classes is unsafe
    var inscope p = new MyClass()
  }
  // deleted automatically at end of scope

When to use what
================

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Approach
     - When to use
   * - Do nothing
     - Short-lived programs where process exit reclaims memory
   * - ``var inscope``
     - Deterministic cleanup (files, sockets, locks)
   * - ``delete``
     - Immediate memory reclaim in long-running code
   * - Custom ``finalize``
     - Cleanup logic (logging, counters, etc.)

.. note::

   For heap pointers (from ``new``), ``delete`` requires ``unsafe``.
   For local variables, ``delete`` is safe.

.. seealso::

   :ref:`Finalizers <finalizers>`, :ref:`Statements <statements>` in the
   language reference.

   Full source: :download:`tutorials/language/20_lifetime.das <../../../../tutorials/language/20_lifetime.das>`

   Next tutorial: :ref:`tutorial_error_handling`
