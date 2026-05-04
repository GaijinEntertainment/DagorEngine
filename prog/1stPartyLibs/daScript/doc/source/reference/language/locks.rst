.. _locks:


=====
Locks
=====

.. index:: single: locks

There are several locking mechanisms available in Daslang, designed to ensure the runtime safety of the code.

----------------
Context locks
----------------

A ``Context`` can be locked and unlocked via the ``lock`` and ``unlock`` functions from the C++ side.
When locked, a ``Context`` cannot be restarted. ``tryRestartAndLock`` restarts the context if it is not locked, and then locks it regardless.
The main reason to lock a context is when data on the heap is accessed externally. Heap collection is safe to perform on a locked context.

------------------------------
Array and Table locks
------------------------------

An ``Array`` or ``Table`` can be locked and unlocked explicitly. When locked, they cannot be modified.
Calling ``resize``, ``reserve``, ``push``, ``emplace``, ``erase``, etc. on a locked ``Array`` will cause a ``panic``.
Accessing a locked ``Table``'s elements via the ``[]`` operator will also cause a ``panic``.

Arrays are locked when iterated over, preventing modification during iteration.
The ``keys`` and ``values`` iterators lock a ``Table`` as well. Tables are also locked during ``find*`` operations.

The following operations perform lock checks on data structures:

.. code-block:: text

    * a <- b
    * return <- a
    * resize
    * reserve
    * push
    * push_clone
    * emplace
    * pop
    * erase
    * clear
    * insert
    * for the ``Table``

.. seealso::

    :ref:`Arrays <arrays>` and :ref:`Tables <tables>` for the container types that support locking,
    :ref:`Iterators <iterators>` for iteration patterns that lock containers,
    :ref:`Contexts <contexts>` for context-level locking.


