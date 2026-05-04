.. _move_copy_clone:

=====================
Move, Copy, and Clone
=====================

.. index::
    single: Move
    single: Copy
    single: Clone
    single: Assignment

Daslang has three assignment operators that control how values are transferred between variables.
Understanding when to use each one is essential for writing correct code.

.. list-table::
   :header-rows: 1
   :widths: 10 15 75

   * - Operator
     - Name
     - Effect
   * - ``=``
     - Copy
     - Bitwise copy. Source remains unchanged.
   * - ``<-``
     - Move
     - Transfers ownership. Source is zeroed.
   * - ``:=``
     - Clone
     - Deep copy. Source remains unchanged.

--------------------
Copy (=)
--------------------

The copy operator performs a bitwise copy of the right-hand side into the left-hand side.
The source value is not modified:

.. code-block:: das

    var a = 10
    var b = a       // b is now 10, a is still 10

Copy works for all POD types (``int``, ``float``, ``bool``, ``string``, pointers, etc.) and for
structs whose fields are all copyable.

Types that manage owned resources — ``array``, ``table``, ``lambda``, and ``iterator`` — cannot
be copied. Attempting to copy them produces:

.. code-block:: das

    // error: this type can't be copied, use move (<-) or clone (:=) instead

^^^^^^^^^^^^^^^^^^^
Relaxed assign
^^^^^^^^^^^^^^^^^^^

By default, the compiler automatically promotes ``=`` to ``<-`` when:

- The right-hand side is a temporary value (struct literal, function return value, ``new`` expression)
- The type cannot be copied but can be moved

This means you can often write ``=`` and the compiler will do the right thing:

.. code-block:: das

    var a : array<int>
    a = get_data()          // automatically becomes: a <- get_data()

This behavior is controlled by the ``relaxed_assign`` option (default ``true``).
Set ``options relaxed_assign = false`` to require explicit ``<-`` in all cases
(see :ref:`Options <options>`).

--------------------
Move (<-)
--------------------

The move operator transfers ownership of a value. After a move, the source is zeroed:

.. code-block:: das

    var a : array<int>
    a |> push(1)
    a |> push(2)

    var b <- a              // b now owns the array, a is empty

Move is the primary way to transfer containers and other non-copyable types.

^^^^^^^^^^^^^^^^^^^
When to use move
^^^^^^^^^^^^^^^^^^^

Use ``<-`` when:

- You are done with the source variable and want to transfer its contents
- You are initializing a variable from a function return value
- You are passing ownership into a struct field or container

::

    def make_data() : array<int> {
        var result : array<int>
        result |> push(1)
        result |> push(2)
        return <- result        // move out of the function
    }

    var data <- make_data()     // move into the variable

^^^^^^^^^^^^^^^^^^^
Return by move
^^^^^^^^^^^^^^^^^^^

Functions that return non-copyable types must use ``return <-``:

.. code-block:: das

    def make_table() : table<string; int> {
        var t : table<string; int>
        t |> insert("one", 1)
        return <- t
    }

A regular ``return`` would attempt to copy the value, which fails for non-copyable types.

--------------------
Clone (:=)
--------------------

The clone operator creates a deep copy of the right-hand side. Unlike ``=`` (which is a
shallow bitwise copy), ``:=`` recursively clones all nested containers and non-POD fields:

.. code-block:: das

    var a : array<int>
    a |> push(1)
    a |> push(2)

    var b : array<int>
    b := a                  // b is a deep copy; a is unchanged

After cloning, ``a`` and ``b`` are completely independent — modifying one does not affect the other.

(see :ref:`Clone <clone>` for implementation details and auto-generated clone functions).

^^^^^^^^^^^^^^^^^^^
When to use clone
^^^^^^^^^^^^^^^^^^^

Use ``:=`` when:

- You need to duplicate a container (array, table)
- You need an independent copy of a struct that contains non-copyable fields
- You want both the source and destination to remain valid after the operation

^^^^^^^^^^^^^^^^^^^^
Clone initialization
^^^^^^^^^^^^^^^^^^^^

You can clone-initialize a variable at declaration:

.. code-block:: das

    var a : array<int>
    a |> push(1)

    var b := a              // clone a into a new variable b

This expands into:

.. code-block:: das

    var b <- clone_to_move(a)

where ``clone_to_move`` creates a temporary clone and moves it into the new variable.

For POD types, clone initialization is optimized to a plain copy.

--------------------
Type Compatibility
--------------------

The following table summarizes which operators work with which types:

.. list-table::
   :header-rows: 1
   :widths: 25 15 15 15

   * - Type
     - ``=`` (copy)
     - ``<-`` (move)
     - ``:=`` (clone)
   * - ``int``, ``float``, POD scalars
     - ✓
     - ✓
     - ✓ (becomes copy)
   * - ``string``
     - ✓
     - ✓
     - ✓
   * - ``array<T>``
     - ✗
     - ✓
     - ✓
   * - ``table<K;V>``
     - ✗
     - ✓
     - ✓
   * - Struct (all POD fields)
     - ✓
     - ✓
     - ✓ (becomes copy)
   * - Struct (has array/table fields)
     - ✗
     - ✓
     - ✓
   * - ``tuple``
     - depends on elements
     - depends on elements
     - depends on elements
   * - ``variant``
     - depends on elements
     - depends on elements
     - depends on elements
   * - Raw pointer
     - ✓
     - ✓
     - ✓
   * - ``smart_ptr``
     - ✗
     - ✓
     - ✓
   * - ``lambda``
     - ✗
     - ✓
     - ✗
   * - ``block``
     - ✗
     - ✗
     - ✗
   * - ``iterator``
     - ✗
     - ✓
     - ✗

A struct, tuple, or variant is copyable/moveable/cloneable only if **all** of its fields are.

-----------------------
Variable Initialization
-----------------------

The three initialization forms correspond to the three operators:

.. code-block:: das

    var x = expr            // copy initialization
    var x <- expr           // move initialization
    var x := expr           // clone initialization

For local variable declarations, the compiler checks the type and reports an error if the
chosen initialization mode is not supported:

.. code-block:: das

    var a = get_array()     // error if relaxed_assign is false:
                            // "local variable can only be move-initialized; use <- for that"

---------------------
Struct Initialization
---------------------

In struct literals, each field can use a different initialization mode:

.. code-block:: das

    struct Foo {
        name : string
        data : array<int>
    }

    var items : array<int>
    items |> push(1)

    var f = Foo(name = "hello", data <- items)

Here ``name`` is copy-initialized and ``data`` is move-initialized. After this, ``items`` is empty.

Clone initialization is also supported in struct literals:

.. code-block:: das

    var f2 = Foo(name = "hello", data := items)

After this, ``items`` still contains its original data.

--------------------
Lambda Captures
--------------------

Lambda capture lists support all three modes. The ``capture`` keyword introduces the capture
list, with each entry specifying a mode and a variable name:

.. code-block:: das

    def make_lambda(a : int) {
        var b = 13
        return @ capture(= a, := b) (c : int) {
            debug(a)
            debug(b)
            debug(c)
        }
    }

Here ``= a`` captures ``a`` by copy and ``:= b`` captures ``b`` by clone.

Each capture entry uses an operator prefix to specify the mode:

.. list-table::
   :header-rows: 1
   :widths: 20 20 15 45

   * - Shorthand
     - Named
     - Mode
     - Requirement
   * - ``& x``
     - ``ref(x)``
     - Reference
     - Variable must outlive the lambda. May require ``unsafe``.
   * - ``= x``
     - ``copy(x)``
     - Copy
     - Type must be copyable.
   * - ``<- x``
     - ``move(x)``
     - Move
     - Type must be moveable. Source is zeroed.
   * - ``:= x``
     - ``clone(x)``
     - Clone
     - Type must be cloneable.

Multiple captures are separated by commas:

.. code-block:: das

    return @ capture(= a, <- arr, := table) () {
        // a is copied, arr is moved, table is cloned
    }

Generators also support captures:

.. code-block:: das

    var g <- generator<int> capture(= a) {
        for (x in range(1, a)) {
            yield x
        }
        return false
    }

(see :ref:`Lambdas <lambdas>`).

--------------------
Containers
--------------------

``array`` and ``table`` types cannot be copied. This is because a bitwise copy would create two
variables pointing to the same underlying memory, leading to double-free errors.

To transfer a container, use move:

.. code-block:: das

    var a : array<int>
    a |> push(1)
    var b <- a              // a is now empty

To duplicate a container, use clone:

.. code-block:: das

    var a : array<int>
    a |> push(1)
    var b : array<int>
    b := a                  // independent deep copy

Clone of ``array<T>`` resizes the destination and clones each element. Clone of ``table<K;V>``
clears the destination and re-inserts each key-value pair.

--------------------
Classes
--------------------

Copying or moving class values requires ``unsafe``:

.. code-block:: das

    class Foo {
        x : int
    }

    unsafe {
        var a = new Foo(x=1)
        var b = *a              // copy requires unsafe
    }

--------------------
Custom Clone
--------------------

You can define a custom clone function for any type. If a custom clone exists, it is called
by the ``:=`` operator regardless of whether the type is natively cloneable:

.. code-block:: das

    struct Connection {
        id : int
        socket : int
    }

    def clone(var dest : Connection; src : Connection) {
        dest.id = src.id
        dest.socket = open_new_socket()   // custom logic instead of bitwise copy
        print("cloned connection {src.id}\n")
    }

    var a = Connection(id=1, socket=42)
    var b : Connection
    b := a                            // calls custom clone

(see :ref:`Clone <clone>` for auto-generated clone functions for structs, tuples, variants,
arrays, and tables).

--------------------
Quick Reference
--------------------

Here is a complete example showing all three operators:

.. code-block:: das

    options gen2

    def make_data() : array<int> {
        var result : array<int>
        result |> push(1)
        result |> push(2)
        result |> push(3)
        return <- result
    }

    [export]
    def main {
        // Copy (scalars)
        var a = 10
        var b = a
        print("copy: a={a} b={b}\n")

        // Move (containers)
        var data <- make_data()
        print("data = {data}\n")

        var moved <- data
        print("moved = {moved}\n")
        print("data after move = {data}\n")

        // Clone (deep copy)
        var cloned : array<int>
        cloned := moved
        cloned |> push(4)
        print("cloned = {cloned}\n")
        print("moved = {moved}\n")
    }

Expected output:

.. code-block:: text

    copy: a=10 b=10
    data = [[ 1; 2; 3]]
    moved = [[ 1; 2; 3]]
    data after move = [[]]
    cloned = [[ 1; 2; 3; 4]]
    moved = [[ 1; 2; 3]]

**I want to transfer ownership (source becomes empty):**
    Use ``<-`` (move)

**I want an independent deep copy (both remain valid):**
    Use ``:=`` (clone)

**I want a simple value copy (POD types):**
    Use ``=`` (copy)

**I'm returning a container from a function:**
    Use ``return <-``

**I'm initializing a variable from a function call:**
    Use ``var x <- func()`` or rely on relaxed assign with ``var x = func()``

**The compiler says "this type can't be copied":**
    The type contains arrays, tables, or other non-copyable fields. Use ``<-`` to move
    or ``:=`` to clone.

.. seealso::

    :ref:`Clone <clone>` for custom clone operator implementation,
    :ref:`Finalizers <finalizers>` for delete-after-move semantics,
    :ref:`Arrays <arrays>` and :ref:`Tables <tables>` for non-copyable container types,
    :ref:`Structs <structs>` and :ref:`Classes <classes>` for struct and class copy/move rules,
    :ref:`Temporary types <temporary>` for temporary-type clone rules.

