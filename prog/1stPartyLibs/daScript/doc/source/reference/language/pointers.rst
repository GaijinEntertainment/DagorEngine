.. _pointers:

========
Pointers
========

.. index::
    single: Pointers

Daslang provides nullable pointer types for heap-allocated data, optional
references, and low-level memory access.  Pointer operations split into
two categories: **safe** operations that work without ``unsafe``, and
**unsafe** operations that require an ``unsafe`` block.

.. _pointer_types:

--------------
Pointer types
--------------

=============  =====================================================
Type           Description
=============  =====================================================
``T?``         Nullable pointer to type ``T``
``T? const``   Const pointer — cannot modify the pointed-to value
``T?#``        Temporary pointer (from ``safe_addr``) — cannot escape scope
``void?``      Untyped pointer — must ``reinterpret`` to use
=============  =====================================================

Pointer types are declared by appending ``?`` to any type:

.. code-block:: das

    var p : int?               // pointer to int — null by default
    var ps : Point?            // pointer to struct
    var vp : void?             // void pointer

All pointers default to ``null`` when uninitialized.

.. _pointer_creation:

-----------------
Creating pointers
-----------------

new
^^^

``new`` allocates on the heap and returns ``T?``:

.. code-block:: das

    var p = new Point(x = 3.0, y = 4.0)   // p is Point?
    var q = new Point()                    // default field values

Heap pointers must be released with ``delete`` (see :ref:`pointer_delete`)
or declared with ``var inscope`` for automatic cleanup:

.. code-block:: das

    var inscope pt = new Point(x = 1.0, y = 2.0)
    // pt is automatically deleted at scope exit

addr
^^^^

``addr(x)`` returns a pointer to an existing variable.  **Requires unsafe.**

::

    var x = 42
    unsafe {
        var p = addr(x)   // p is int?
        *p = 100           // modifies x
    }

The pointer is valid only while the variable is alive — using it after
the variable goes out of scope is undefined behavior.

safe_addr
^^^^^^^^^

``safe_addr`` from ``daslib/safe_addr`` returns a temporary pointer (``T?#``)
without requiring ``unsafe``.  The compiler validates that the argument is a
local or global variable (not a field of a temporary):

.. code-block:: das

    require daslib/safe_addr
    var a = 13
    var p = safe_addr(a)   // p is int?# (temporary pointer)
    print("{*p}\n")

Temporary pointers cannot be stored in containers or returned from functions.

.. _pointer_deref:

--------------
Dereferencing
--------------

``*p`` or ``deref(p)`` follows the pointer to the value.  Both panic if the
pointer is null:

.. code-block:: das

    *p         // dereference
    deref(p)   // same thing

For struct pointers, ``.`` auto-dereferences — no ``->`` operator is needed:

.. code-block:: das

    var inscope pt = new Point(x = 5.0, y = 6.0)
    print("{pt.x}\n")     // 5 — same as (*pt).x
    pt.x = 10.0           // modify through auto-deref

.. _pointer_null_safety:

-----------
Null safety
-----------

Null checks
^^^^^^^^^^^

Pointers can be compared to ``null``:

.. code-block:: das

    if (p != null) {
        print("{*p}\n")      // safe — we checked
    }

Null dereference panics at runtime and can be caught with ``try``/``recover``:

.. code-block:: das

    try {
        var np : int?
        print("{*np}\n")
    } recover {
        print("caught null dereference\n")
    }

Safe navigation ``?.``
^^^^^^^^^^^^^^^^^^^^^^

``?.`` returns ``null`` instead of panicking when the pointer is null:

.. code-block:: das

    p?.x         // returns x if p is non-null, null otherwise
    a?.b?.c      // chains — short-circuits on first null

Safe navigation results are themselves nullable, so combine with ``??``
for a concrete fallback:

.. code-block:: das

    let val = p?.x ?? -1     // -1 if p is null

Null coalescing ``??``
^^^^^^^^^^^^^^^^^^^^^^

``??`` provides a default value when the left side is null:

.. code-block:: das

    let x = p ?? default_value

For pointer dereference:

.. code-block:: das

    let x = *p ?? 0    // 0 if p is null

.. _pointer_delete:

--------
Deletion
--------

``delete`` frees heap memory and sets the pointer to null.  **Requires unsafe.**

::

    var p = new Point()
    unsafe {
        delete p       // frees memory, p becomes null
    }

Prefer ``var inscope`` for automatic cleanup — it adds a ``finally`` block
that deletes the pointer when the scope exits:

.. code-block:: das

    var inscope p = new Point()
    // p is automatically deleted at end of scope

.. _pointer_arithmetic:

-------------------
Pointer arithmetic
-------------------

All pointer arithmetic **requires unsafe**.  No bounds checking is performed.

Indexing
^^^^^^^^

``p[i]`` accesses the ``i``-th element at the pointer's address:

.. code-block:: das

    var arr <- [10, 20, 30, 40, 50]
    unsafe {
        var p = addr(arr[0])
        print("{p[0]}, {p[2]}\n")     // 10, 30
    }

Increment and addition
^^^^^^^^^^^^^^^^^^^^^^

::

    unsafe {
        ++ p         // advance pointer by one element
        p += 3       // advance by three elements
    }

.. warning::

   Pointer arithmetic can easily cause out-of-bounds access or invalid
   pointer states.  Use array bounds-checked access whenever possible.

.. _pointer_void:

--------------
Void pointers
--------------

``void?`` is an untyped pointer — equivalent to ``void*`` in C/C++.  It is
used for opaque handles and C/C++ interop.  You must ``reinterpret`` it back
to a typed pointer before dereferencing:

.. code-block:: das

    unsafe {
        var x = 123
        var px = addr(x)
        var vp : void? = reinterpret<void?> px     // erase type
        var px2 = reinterpret<int?> vp             // restore type
        print("{*px2}\n")                          // 123
    }

.. _pointer_intptr:

------
intptr
------

``intptr(p)`` converts any pointer (raw or smart) to a ``uint64`` integer
representing its memory address:

.. code-block:: das

    let address = intptr(p)   // uint64

Useful for debugging, logging, pointer identity comparisons, or hashing.

.. _pointer_reinterpret:

-----------
reinterpret
-----------

``reinterpret<T>`` performs a raw bit cast between types of the same size.
**Requires unsafe.**  It does not convert values — it reinterprets the raw bits:

.. code-block:: das

    unsafe {
        let f = 1.0
        let bits = reinterpret<int> f      // IEEE 754: 0x3f800000
        let back = reinterpret<float> bits // 1.0
    }

Can also cast between pointer types:

.. code-block:: das

    unsafe {
        var p : int? = addr(x)
        var vp = reinterpret<void?> p    // to void?
        var p2 = reinterpret<int?> vp    // back to int?
    }

.. _pointer_typeinfo:

---------
Type info
---------

Several ``typeinfo`` queries test pointer properties at compile time:

.. code-block:: das

    typeinfo is_pointer(p)       // true if p is a pointer type
    typeinfo is_smart_ptr(p)     // true if p is a smart_ptr<T>
    typeinfo is_void_pointer(p)  // true if p is void?
    typeinfo can_delete_ptr(p)   // true if delete is valid for p

.. _pointer_summary:

-------
Summary
-------

**Safe (no unsafe required):**

* ``new T()`` — heap allocate, returns ``T?``
* ``*p`` / ``deref(p)`` — dereference (panics if null)
* ``p.field`` — auto-deref field access
* ``p?.field`` — safe navigation (null-propagating)
* ``p ?? default`` — null coalescing
* ``safe_addr(x)`` — temporary pointer (``T?#``)
* ``var inscope p = new T()`` — automatic cleanup
* ``intptr(p)`` — pointer to integer

**Unsafe (requires unsafe block):**

* ``addr(x)`` — address of variable
* ``delete p`` — free heap memory
* ``p[i]`` — pointer indexing
* ``++ p`` / ``p += N`` — pointer arithmetic
* ``reinterpret<T>`` — raw bit cast

.. seealso::

    :ref:`Unsafe <unsafe>` for the full list of unsafe operations.

    :ref:`Values and Data Types <datatypes_and_values>` for smart pointers
    (``smart_ptr<T>``).

    :ref:`Temporary types <temporary>` for temporary pointers (``T?#``) and
    ``safe_addr``.

    :ref:`Tutorial: Pointers <tutorial_pointers>` for a hands-on walkthrough.
