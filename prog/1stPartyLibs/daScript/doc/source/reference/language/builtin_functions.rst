.. _builtin_functions:


==================
Built-in Functions
==================

.. index::
    single: Built-in Functions
    pair: Global Symbols; Built-in Functions

Daslang built-in functions fall into two categories:

* **Intrinsic functions** — compiled directly into the AST as dedicated expression nodes
  (e.g. ``invoke``, ``assert``, ``debug``).  They are documented in this section.

* **Standard library functions** — defined in ``builtin.das`` and other standard modules
  that are always available. These include container operations (``push``, ``sort``,
  ``find_index``), iterator helpers (``each``, ``next``), clone, lock, and serialization
  functions. They are documented in the standard library reference
  (see :ref:`Built-in functions reference <stdlib_builtin>`).

This page covers both groups, organized by category.

---------------------
Invocation
---------------------

.. das:function:: invoke(block_or_function, arguments)

    Calls a block, lambda, or function pointer with the provided arguments:

    .. code-block:: das

        def apply(f : block<(x : int) : int>; value : int) : int {
            return invoke(f, value)
        }

    Blocks, lambdas, and function pointers can also be called using regular call syntax.
    The compiler will expand ``f(value)`` into ``invoke(f, value)``:

    .. code-block:: das

        def apply(f : block<(x : int) : int>; value : int) : int {
            return f(value)     // equivalent to invoke(f, value)
        }

    (see :ref:`Functions <functions>`, :ref:`Blocks <blocks>`, :ref:`Lambdas <lambdas>`).

---------------------
Assertions
---------------------

.. das:function:: assert(x, str)

    Triggers an application-defined assert if ``x`` is ``false``.
    ``assert`` may be removed in release builds, so the expression ``x``
    must have **no side effects** — the compiler will reject it otherwise:

    .. code-block:: das

        assert(index >= 0, "index must be non-negative")

.. das:function:: verify(x, str)

    Triggers an application-defined assert if ``x`` is ``false``.
    Unlike ``assert``, ``verify`` is **never removed** from release builds
    (it generates ``DAS_VERIFY`` in C++ rather than ``DAS_ASSERT``).
    Additionally, the expression ``x`` is allowed to have side effects:

    .. code-block:: das

        verify(initialize_system(), "initialization failed")

.. das:function:: static_assert(x, str)

    Causes a **compile-time** error if ``x`` is ``false``.
    ``x`` must be a compile-time constant.  ``static_assert`` expressions
    are removed from the compiled program:

    .. code-block:: das

        static_assert(typeinfo is_pod(type<Foo>), "Foo must be POD")

.. das:function:: concept_assert(x, str)

    Similar to ``static_assert``, but the error is reported
    **one level above** in the call stack.  This is useful for reporting
    type-contract violations in generic functions:

    .. code-block:: das

        def sort_array(var a : auto(TT)[]) {
            concept_assert(typeinfo is_numeric(type<TT>), "sort_array requires numeric type")
            sort(a)
        }

---------------------
Debug
---------------------

.. das:function:: debug(x, str)

    Prints the string ``str`` and the value of ``x`` (similar to ``print``), then
    **returns** ``x``.  This makes it suitable for debugging inside expressions:

    .. code-block:: das

        let mad = debug(x, "x") * debug(y, "y") + debug(z, "z")

.. das:function:: print(str)

    Outputs the string ``str`` to the standard output.  ``print`` only accepts
    strings — to print other types, use string interpolation:

    .. code-block:: das

        print("hello\n")       // ok
        print("{13}\n")        // ok, integer is interpolated into the string
        // print(13)           // error: print expects a string

---------------------
Panic
---------------------

.. das:function:: panic(str)

    Terminates execution with the given error message.
    Panic can be caught with a ``try``/``recover`` block, but unlike C++ exceptions,
    ``panic`` is intended for **fatal errors only**.  Recovery may have side effects,
    and not everything on the stack is guaranteed to recover properly:

    .. code-block:: das

        try {
            panic("something went wrong")
        } recover {
            print("recovered from panic\n")
        }

-----------------------
Memory & Type Utilities
-----------------------

.. das:function:: addr(x)

    Returns a pointer to the value ``x``.  This is an **unsafe** operation:

    .. code-block:: das

        unsafe {
            var a = 42
            var p = addr(a)     // p is int?
        }

.. das:function:: intptr(p)

    Converts a pointer (raw or smart) to a ``uint64`` integer value representing
    its address:

    .. code-block:: das

        let address = intptr(some_ptr)

.. das:function:: typeinfo trait(expression)

    Provides compile-time type information about an expression or a ``type<T>`` argument.
    Used extensively in generic programming:

    .. code-block:: das

        typeinfo sizeof(type<float3>)       // 12
        typeinfo typename(type<int>)        // "int"
        typeinfo is_pod(type<int>)          // true
        typeinfo has_field<x>(myStruct)     // true if myStruct has field x

    (see :ref:`Generic Programming <generic_programming>` for a full list of typeinfo traits).

---------------------
Array Operations
---------------------

The following operations are defined in ``builtin.das`` and are always available.

^^^^^^^^^^^^^^^^
Resize & Reserve
^^^^^^^^^^^^^^^^

.. das:function:: resize(var arr : array<T>; new_size : int)

    Resizes the array to ``new_size`` elements.  New elements are zero-initialized.

.. das:function:: resize_and_init(var arr : array<T>; new_size : int)

    Resizes the array and default-initializes all new elements using the element type's
    default constructor.

.. das:function:: resize_no_init(var arr : array<T>; new_size : int)

    Resizes without initializing new elements. Only valid for POD/raw element types.

.. das:function:: reserve(var arr : array<T>; capacity : int)

    Pre-allocates memory for at least ``capacity`` elements without changing the array length.

^^^^^^^^^^^^^^^^
Push & Emplace
^^^^^^^^^^^^^^^^

.. das:function:: push(var arr : array<T>; value : T [; at : int])

    Inserts a **copy** of ``value`` into the array at index ``at`` (or at the end
    if ``at`` is omitted).  Also accepts another ``array<T>`` or a fixed-size ``T[]``
    to push all elements at once:

    .. code-block:: das

        var a : array<int>
        push(a, 1)
        push(a, 2, 0)         // insert 2 at beginning
        push(a, fixed_array(3, 4, 5))   // push three elements

.. das:function:: push_clone(var arr : array<T>; value [; at : int])

    Clones ``value`` (deep copy) and inserts the clone into the array.

.. das:function:: emplace(var arr : array<T>; var value : T& [; at : int])

    **Move-inserts** ``value`` into the array, zeroing the source.  Preferred for
    types that own resources (e.g., other arrays, tables, smart pointers):

    .. code-block:: das

        var inner : array<int>
        push(inner, 1)
        var outer : array<array<int>>
        emplace(outer, inner)   // inner is now empty

.. das:function:: emplace_new(var arr : array<smart_ptr<T>>; var value : smart_ptr<T>)

    Move-inserts a smart pointer into the back of the array.

^^^^^^^^^^^^^^^^
Remove & Erase
^^^^^^^^^^^^^^^^

.. das:function:: erase(var arr : array<T>; at : int [; count : int])

    Removes the element at index ``at`` (or ``count`` elements starting at ``at``).

.. das:function:: erase_if(var arr : array<T>; blk : block<(element : T) : bool>)

    Removes all elements for which ``blk`` returns ``true``:

    .. code-block:: das

        erase_if(arr) $(x) { return x < 0 }

.. das:function:: remove_value(var arr : array<T>; value : T) : bool

    Removes the first occurrence of ``value``.  Returns ``true`` if an element was removed.

.. das:function:: pop(var arr : array<T>)

    Removes the last element of the array.

^^^^^^^^^^^^^^^^
Access & Search
^^^^^^^^^^^^^^^^

.. das:function:: back(var arr : array<T>) : T&

    Returns a reference to the last element.  Panics if the array is empty.

.. das:function:: empty(arr : array<T>) : bool

    Returns ``true`` if the array has no elements.

.. das:function:: length(arr : T[]) : int

    Returns the compile-time dimension of a fixed-size array.

.. das:function:: find_index(arr; value : T) : int

    Returns the index of the first element equal to ``value``, or ``-1`` if not found.
    Works on dynamic arrays, fixed-size arrays, and iterators.

.. das:function:: find_index_if(arr; blk : block<(element : T) : bool>) : int

    Returns the index of the first element satisfying ``blk``, or ``-1`` if not found.

.. das:function:: has_value(arr; value) : bool

    Returns ``true`` if any element equals ``value``.  Works on any iterable.

.. das:function:: subarray(arr; r : range) : array<T>

    Returns a new array containing elements in the specified range.

^^^^^^^^^^^^^^^^
Sorting
^^^^^^^^^^^^^^^^

.. das:function:: sort(var arr)

    Sorts the array in ascending order using the default ``<`` operator:

    .. code-block:: das

        var a <- array(3, 1, 2)
        sort(a)     // a is now [1, 2, 3]

.. das:function:: sort(var arr; cmp : block<(x, y : T) : bool>)

    Sorts the array using a custom comparator.  The comparator should return
    ``true`` if ``x`` should come before ``y``:

    .. code-block:: das

        sort(arr) $(a, b) { return a > b }  // descending order

^^^^^^^^^^^^^^^^
Swap
^^^^^^^^^^^^^^^^

.. das:function:: swap(var a, b : T&)

    Swaps two values using move semantics through a temporary.

---------------------
Table Operations
---------------------

^^^^^^^^^^^^^^^^
Lookup
^^^^^^^^^^^^^^^^

.. das:function:: key_exists(tab : table<K;V>; key : K) : bool

    Returns ``true`` if ``key`` exists in the table.

.. das:function:: get(tab : table<K;V>; key : K; blk : block<(value : V&)>)

    Looks up ``key`` in the table.  If found, the table is locked and ``blk``
    is invoked with a reference to the value.  Returns ``true`` if the key was found:

    .. code-block:: das

        get(tab, "key") $(value) {
            print("found: {value}\n")
        }

.. das:function:: get_value(var tab : table<K;V>; key : K) : V

    Returns a **copy** of the value at ``key``.  Only works for copyable types.

.. das:function:: clone_value(var tab : table<K; smart_ptr<V>>; key : K) : smart_ptr<V>

    Clones the smart pointer value at ``key``.

^^^^^^^^^^^^^^^^
Insert & Emplace
^^^^^^^^^^^^^^^^

.. das:function:: insert(var tab : table<K;V>; key : K; value : V)

    Inserts a key-value pair.  For key-only tables (sets), only the key is needed:

    .. code-block:: das

        var seen : table<string>
        insert(seen, "hello")

.. das:function:: insert_clone(var tab : table<K;V>; key : K; value : V)

    Clones ``value`` and inserts the clone into the table.

.. das:function:: emplace(var tab : table<K;V>; key : K; var value : V&)

    Move-inserts ``value`` into the table at ``key``.

.. das:function:: insert_default(var tab : table<K;V>; key : K [; value])

    Inserts ``value`` (or a default-constructed value) only if ``key`` does not already exist.

.. das:function:: emplace_default(var tab : table<K;V>; key : K)

    Emplaces a default value only if ``key`` does not already exist.

^^^^^^^^^^^^^^^^
Remove & Clear
^^^^^^^^^^^^^^^^

.. das:function:: erase(var tab : table<K;V>; key : K) : bool

    Removes the entry at ``key``.  Returns ``true`` if the key was found and removed.

.. das:function:: clear(var tab : table<K;V>)

    Removes all entries from the table.

^^^^^^^^^^^^^^^^
Iteration
^^^^^^^^^^^^^^^^

.. das:function:: keys(tab : table<K;V>) : iterator<K>

    Returns an iterator over all keys in the table.

.. das:function:: values(tab : table<K;V>) : iterator<V&>

    Returns an iterator over all values in the table (mutable or const depending on
    the table).

.. das:function:: empty(tab : table<K;V>) : bool

    Returns ``true`` if the table has no entries.

---------------------
Iterator Operations
---------------------

.. das:function:: each(iterable)

    Creates an iterator from a range, array, fixed-size array, string, or lambda:

    .. code-block:: das

        for (x in each(my_range)) {
            print("{x}\n")
        }

    Overloads exist for ``range``, ``urange``, ``range64``, ``urange64``, ``string``,
    ``T[]``, ``array<T>``, and ``lambda``.

.. das:function:: each_enum(tt : T)

    Creates an iterator over all values of an enumeration type.

    .. deprecated:: Use the built-in enumeration iteration instead.

.. das:function:: next(var it : iterator<T>; var value : T&) : bool

    Advances the iterator and stores the current value in ``value``.
    Returns ``false`` when the iterator is exhausted.

.. das:function:: nothing(var it : iterator<T>) : iterator<T>

    Returns an empty (nil) iterator.

.. das:function:: iter_range(container) : range

    Returns ``range(0, length(container))`` — useful for index-based iteration.

---------------------
Conversion Functions
---------------------

.. das:function:: to_array(source) : array<T>

    Converts a fixed-size array or iterator to a dynamic ``array<T>``:

    .. code-block:: das

        let fixed = fixed_array(1, 2, 3)
        var dynamic <- to_array(fixed)

.. das:function:: to_array_move(var source) : array<T>

    Moves elements from the source into a new dynamic array.

.. das:function:: to_table(source) : table<K;V>

    Converts a fixed-size array of tuples to a ``table<K;V>``, or a fixed-size array
    of keys to a key-only table (set):

    .. code-block:: das

        var tab <- to_table(fixed_array(("one", 1), ("two", 2)))

.. das:function:: to_table_move(var source) : table<K;V>

    Moves elements from the source into a new table.

---------------------
Clone
---------------------

.. das:function:: clone(src) : T

    Creates a deep copy of any value. For arrays and tables, all elements are cloned
    recursively:

    .. code-block:: das

        var a <- [1, 2, 3]
        var b := a          // equivalent to: clone(b, a)

.. das:function:: clone_dim(var dst; src)

    Clones a fixed-size array into another of the same dimension.

(see :ref:`Clone <clone>` for full cloning rules).

---------------------
Lock Operations
---------------------

.. das:function:: lock(container; blk)

    Locks an array or table, invokes ``blk`` with a temporary handled reference,
    then unlocks.  While locked, the container cannot be resized or modified
    structurally:

    .. code-block:: das

        lock(my_table) $(t) {
            for (key in keys(t)) {
                print("{key}\n")
            }
        }

.. das:function:: lock_forever(var tab : table<K;V>) : table#

    Locks the table permanently and returns a handled (temporary) reference.
    This is useful for read-only lookup tables.

.. das:function:: lock_data(var arr : array<T>; blk : block<(var p : T?#; s : int)>)

    Locks the array and provides raw pointer access to the underlying data along with
    its length.

---------------------
Serialization
---------------------

.. das:function:: binary_save(obj; blk : block<(data : array<uint8>#)>)

    Serializes a reference-type object to a binary representation and invokes ``blk``
    with the resulting byte array.

.. das:function:: binary_load(var obj; data : array<uint8>)

    Deserializes a reference-type object from binary data.

---------------------
Smart Pointer
---------------------

.. das:function:: get_ptr(src : smart_ptr<T>) : T?

    Extracts a raw pointer from a smart pointer.

.. das:function:: get_const_ptr(src : smart_ptr<T>) : T? const

    Extracts a const raw pointer from a smart pointer.

.. das:function:: add_ptr_ref(src : smart_ptr<T>) : smart_ptr<T>

    Increments the reference count and returns a new smart pointer.

---------------------
Memory Mapping
---------------------

.. das:function:: map_to_array(data : void?; len : int; blk)

    Maps raw memory to a temporary mutable array view.  This is an **unsafe** operation.

.. das:function:: map_to_ro_array(data : void?; len : int; blk)

    Maps raw memory to a temporary read-only array view.  This is an **unsafe** operation.

---------------------
Vector Construction
---------------------

Helper functions for constructing vector types from individual components:

.. code-block:: das

    let v2 = float2(1.0, 2.0)
    let v3 = float3(1.0, 2.0, 3.0)
    let v4 = float4(1.0, 2.0, 3.0, 4.0)

    let i2 = int2(1, 2)
    let i3 = int3(1, 2, 3)
    let i4 = int4(1, 2, 3, 4)

    let u2 = uint2(1u, 2u)
    let u3 = uint3(1u, 2u, 3u)
    let u4 = uint4(1u, 2u, 3u, 4u)

These accept any numeric arguments and convert them to the appropriate element type.

---------------------
Move Helpers
---------------------

.. das:function:: copy_to_local(a) : T

    Copies a value into a local variable (removes const qualifier).

.. das:function:: move_to_local(var a : T&) : T

    Moves a value out of a reference into a local variable.

.. das:function:: move_to_ref(var dst : T&; var src : T)

    Moves (or copies for non-ref types) ``src`` into ``dst``.

---------------------
Miscellaneous
---------------------

.. das:function:: get_command_line_arguments() : array<string>

    Returns the command-line arguments passed to the program.
