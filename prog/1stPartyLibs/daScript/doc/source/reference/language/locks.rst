.. _locks:


=====
Locks
=====

.. index:: single: locks

There are several locking mechanisms available in daScript. They are designed to ensure runtime safety of the code.

----------------
Context locks
----------------

`Context` can be locked and unlocked via `lock` and `unlock` functions from the C++ side.
When locked `Context` can not be restarted. `tryRestartAndLock` restarts context if its not locked, and then locks it regardless.
The main reason to lock context is when data on the heap is accessed externally. Heap collection is safe to do on a locked context.

------------------------------
Array and Table locks
------------------------------

`Array` or `Table` can be locked and unlocked explicitly. When locked, they can't be modified.
Calling `resize`, `reserve`, `push`, `emplace`, `erase`, etc on the locked `Array`` will cause `panic`.
Accessing locked `Table` elements via [] operation would cause `panic`.

`Arrays` are locked when iterated over. This is done to prevent modification of the array while it is being iterated over.
`keys` and `values` iterators lock `Table` as well. `Tables` are also locked during the `find*` operations.

------------------------------
Array and Table lock checking
------------------------------

`Array` and `Table` lock checking occurs on the data structures, which internally contain other `Arrays` or `Tables`.

Consider the following example::

    var a : array < array<int> >
    ...
    for b in a[0]
        a |> resize(100500)

The `resize` operation on the `a` array will cause `panic` because `a[0]` is locked during the iteration.
This test, however, can only happen in runtime. The compiler generates custom `resize` code, which verifies locks::

    def private builtin`resize ( var Arr:array<array<int> aka numT> explicit; newSize:int const )
        _builtin_verify_locks(Arr)
        __builtin_array_resize(Arr,newSize,24,__context__)

The `_builtin_verify_locks` iterates over provided data, and for each `Array` or `Table` makes sure it does not lock.
If its locked `panic` occurs.

Custom operations will only be generated, if the underlying type needs lock checks.

Here are the list of operations, which perform lock check on the data structures::
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
    * a[b] for the `Table`

Lock checking can be explicitly disabled
    * for the `Array` or the `Table` by using `set_verify_array_locks` and `set_verify_table_locks` functions.
    * for a structure type with the [skip_field_lock_check] structure annotation
    * for the entire function with the [skip_lock_check] function annotation
    * for the entire context with the `options skip_lock_checks`
    * for the entire context with the `set_verify_context_locks` function


