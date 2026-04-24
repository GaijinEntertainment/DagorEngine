.. _datatypes_and_values:

=====================
Values and Data Types
=====================

Daslang is a strong, statically typed language.  All variables have a type.
Daslang's basic POD (plain old data) data types are:

.. code-block:: das

    int, uint, float, bool, double, int64, uint64
    int2, int3, int4, uint2, uint3, uint4, float2, float3, float4,
    range, urange, range64, urange64

All PODs are represented with machine register/word. All PODs are passed to functions by value.

Daslang's storage types are:

.. code-block:: das

    int8, uint8, int16, uint16 - 8/16-bits signed and unsigned integers

They can't be manipulated, but can be used as storage type within structs, classes, etc.

Daslang's other types are:

.. code-block:: das

    string, das_string, struct, pointers, references, block, lambda, function pointer,
    array, table, tuple, variant, iterator, bitfield


All Daslang's types are initialized with zeroed memory by default.

.. _userdata-index:

--------
Integer
--------

An integer represents a 32-bit (un)signed number:

.. code-block:: das

    let a = 123    // decimal, integer
    let u = 123u   // decimal, unsigned integer
    let h = 0x0012 // hexadecimal, unsigned integer
    let o = 075    // octal, unsigned integer

    let a = int2(123, 124)    // two integers type
    let u = uint2(123u, 124u) // two unsigned integer type

--------
Float
--------

A float represents a 32-bit floating point number:

.. code-block:: das

    let a = 1.0
    let b = 0.234
    let a = float2(1.0, 2.0)

--------
Bool
--------

A bool is a double-valued (Boolean) data type. Its literals are ``true``
and ``false``. A bool value expresses the validity of a condition
(tells whether the condition is true or false):

.. code-block:: das

    let a = true
    let b = false

All conditionals (if, elif, while) work only with the bool type.

--------
String
--------

Strings are an immutable sequence of characters. In order to modify a
string, it is necessary to create a new one.

Daslang's strings are similar to strings in C or C++.  They are
delimited by quotation marks(``"``) and can contain escape
sequences (``\t``, ``\a``, ``\b``, ``\n``, ``\r``, ``\v``, ``\f``,
``\\``, ``\"``, ``\'``, ``\0``, ``\x<hh>``, ``\u<hhhh>`` and
``\U<hhhhhhhh>``):

.. code-block:: das

    let a = "I'm a string\n"
    let a = "I'm also
        a multi-line
             string\n"

Strings type can be thought of as a 'pointer to the actual string', like a 'const char \*' in C.
As such, they will be passed to functions by value (but this value is just a reference to the immutable string in memory).

``das_string`` is a mutable string, whose content can be changed. It is simply a builtin handled type, i.e., a std::string bound to Daslang.
As such, it passed as reference.

.. _type_conversions:

------------------------------
Type Conversion and Casting
------------------------------

.. index::
    single: Type Conversion
    single: Casting

Daslang is a strongly typed language with **no implicit type conversions**.
All numeric operations require operands of the same type — for example, ``int + float``
is a compilation error.  You must convert explicitly:

.. code-block:: das

    let i = 42
    let f = float(i) + 1.0     // explicit int -> float
    let i2 = i + int(1.0)      // explicit float -> int

Explicit numeric casts
^^^^^^^^^^^^^^^^^^^^^^

Any numeric type can be explicitly converted to any other numeric type using
the target type name as a function:

.. code-block:: das

    float(42)           // int -> float              (42.0)
    int(3.7)            // float -> int, truncates   (3)
    double(3.14)        // float -> double
    float(3.14lf)       // double -> float
    uint(42)            // int -> uint
    int64(42)           // int -> int64
    uint64(42)          // int -> uint64
    int8(42)            // int -> int8 (storage type)
    uint8(42)           // int -> uint8 (storage type)
    int16(42)           // int -> int16 (storage type)
    uint16(42)          // int -> uint16 (storage type)

Float-to-integer conversion truncates toward zero (like C).

Enumeration casts
^^^^^^^^^^^^^^^^^

Enumerations can be converted to their underlying integer type:

.. code-block:: das

    enum Color {
        red
        green
        blue
    }

    let c = Color.green
    let i = int(c)              // 1

Converting an integer back to an enumeration requires ``unsafe`` and ``reinterpret``:

.. code-block:: das

    unsafe {
        let c2 = reinterpret<Color>(1)  // Color.green
    }

String conversion
^^^^^^^^^^^^^^^^^

Any type can be converted to a string via the ``string`` function:

.. code-block:: das

    let s = string(42)          // "42"
    let s2 = string(3.14)      // "3.14"

String interpolation (``{expr}`` inside string literals) also converts expressions to text automatically.

To parse strings into numbers, use the functions from ``require strings``:

.. code-block:: das

    require strings
    let i = to_int("123")       // 123
    let f = to_float("3.14")   // 3.14

There is no ``int(string)`` — use ``to_int`` instead.

What is NOT allowed
^^^^^^^^^^^^^^^^^^^

* **No implicit numeric promotion:** ``int + float`` is a compile error
* **No bool(int):** use a comparison like ``x != 0`` instead
* **No implicit int-to-float assignment:** ``var f : float = 42`` is a compile error; use ``float(42)``
* **No int(string):** use ``to_int`` from the ``strings`` module

--------
Table
--------

Tables are associative containers implemented as a set of key/value pairs:

.. code-block:: das

    var tab: table<string; int>
    tab["10"] = 10
    tab["20"] = 20
    tab["some"] = 10
    tab["some"] = 20 // replaces the value for 'some' key

(see :ref:`Tables <tables>`).

--------
Array
--------

Arrays are simple sequences of objects. There are static arrays (fixed size) and dynamic arrays (container, size is dynamic).  The index always starts from 0:

.. code-block:: das

    var a = fixed_array(1, 2, 3, 4) // fixed size of array is 4, and content is [1, 2, 3, 4]
    var b: array<string>            // empty dynamic array
    push(b,"some")                  // now it is 1 element of "some"

(see :ref:`Arrays <arrays>`).

--------
Struct
--------

Structs are records of data of other types (including structs), similar to C.
All structs (as well as other non-POD types, except strings) are passed by reference.

(see :ref:`Structs <structs>`).

--------
Classes
--------

Classes are similar to structures, but they additionally allow built-in methods and rtti.

(see :ref:`Classes <classes>`).

--------
Variant
--------

Variant is a special anonymous data type similar to a struct, however only one field exists at a time.
It is possible to query or assign to a variant type, as well as the active field value.

(see :ref:`Variants <variants>`).

--------
Tuple
--------

Tuples are anonymous records of data of other types (including structs), similar to a C++ std::tuple.
All tuples (as well as other non-POD types, except strings) are passed by reference.

(see :ref:`Tuples <tuples>`).

-----------
Enumeration
-----------

An enumeration binds a specific integer value to a name, similar to C++ enum classes.

(see :ref:`Enumerations <enumerations>`).

--------
Bitfield
--------

Bitfields are an anonymous data type, similar to enumerations. Each field explicitly represents one bit,
and the storage type is always a uint. Queries on individual bits are available on variants,
as well as binary logical operations.

(see :ref:`Bitfields <bitfields>`).

--------
Function
--------

Functions are similar to those in most other languages:

.. code-block:: das

    def twice(a: int): int {
        return a + a
    }

However, there are generic (templated) functions, which will be 'instantiated' during function calls by type inference:

.. code-block:: das

    def twice(a) {
        return a + a
    }

    let f = twice(1.0) // 2.0 float
    let i = twice(1)   // 2 int

(see :ref:`Functions <functions>`).

--------------
Reference
--------------

References are types that 'reference' (point to) some other data:

.. code-block:: das

    def twice(var a: int&) {
        a = a + a
    }
    var a = 1
    twice(a) // a value is now 2

All structs are always passed to functions arguments as references.


--------------
Pointers
--------------

Pointers are types that 'reference' (point to) some other data, but can be null (point to nothing)
(see :ref:`Pointers <pointers>`).
In order to work with actual value, one need to dereference it using the dereference or safe navigation operators.
Dereferencing will panic if a null pointer is passed to it.
Pointers can be created using the new operator, or with the C++ environment.
::

    def twice(var a: int&) {
        a = a + a
    }
    def twicePointer(var a: int?) {
        twice(*a)
    }

    struct Foo {
        x: int
    }

    def getX(foo: Foo?) { // it returns either foo.x or -1, if foo is null
       return foo?.x ?? -1
    }

--------------
Smart Pointers
--------------

Smart pointers (``smart_ptr<T>``) are reference-counted pointers to C++-managed (handled) types.
They are **not** available for regular Daslang structs or classes — only for types registered
as handled types from the C++ side (such as AST node types in ``daslib/ast``).

Smart pointers are primarily used in the macro and AST manipulation context:

.. code-block:: das

    require daslib/ast

    var inscope expr : smart_ptr<ExprConstInt> <- new ExprConstInt(value=42)

The key properties of smart pointers:

* They maintain a reference count and automatically release the object when the count reaches zero
* They can be moved but not copied via ``<-``
* Dereferencing works the same as regular pointers (``*ptr`` and ``ptr.field``)
* Moving from a smart pointer value requires ``unsafe`` unless the value is a ``new`` expression

Because ``strict_smart_pointers`` is enabled by default, smart pointer variables must be
declared with ``inscope`` to ensure automatic cleanup:

.. code-block:: das

    var inscope a <- new ExprConstInt(value=1)   // create — safe, no unsafe needed
    var inscope b <- a                           // move — safe, a becomes null
    unsafe {
        var inscope c <- some_function()         // move from function result — unsafe
    }

Ownership transfer functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Daslang provides built-in functions for safe smart pointer ownership transfer.
These avoid the need for ``unsafe`` blocks when reassigning smart pointers that
already hold a value:

``move(dest, src)``
    Transfers ownership from ``src`` into ``dest``. If ``dest`` already holds a value,
    its reference count is decremented. After the call, ``src`` becomes null.
    Both arguments must be existing smart pointer variables:

    .. code-block:: das

        var inscope a <- new ExprConstInt(value=1)
        var inscope b <- new ExprConstInt(value=2)
        b |> move <| a       // b now holds what a held; old b is released; a is null

``move_new(dest, src)``
    Transfers ownership from a newly created smart pointer into ``dest``. If ``dest``
    already holds a value, its reference count is decremented. This is the idiomatic
    way to replace the contents of a smart pointer field or variable:

    .. code-block:: das

        var inscope fn <- find_function("foo")
        fn |> move_new <| new Function(name := "bar")   // fn now holds the new Function

    It can also be called in function-call style:

    .. code-block:: das

        move_new(fn) <| new Function(name := "bar")

``smart_ptr_clone(dest, src)``
    Clones (increments the reference count of) ``src`` into ``dest``. Both ``dest`` and
    ``src`` remain valid after the call. If ``dest`` already held a value, it is released.

``smart_ptr_use_count(ptr)``
    Returns the current reference count of the smart pointer as a ``uint``.

Smart pointer types frequently appear in ``daslib/ast`` and ``daslib/ast_boost`` when
building or transforming AST nodes in macros.

-----------
Iterators
-----------

Iterators are a sequence which can be traversed, and associated data retrieved.
They share some similarities with C++ iterators.

(see :ref:`Iterators <iterators>`).

.. seealso::

    :ref:`Structs <structs>`, :ref:`Tuples <tuples>`, and :ref:`Variants <variants>` for composite types,
    :ref:`Arrays <arrays>` and :ref:`Tables <tables>` for container types,
    :ref:`Aliases <aliases>` for type alias declarations,
    :ref:`Bitfields <bitfields>` for the bitfield type.

