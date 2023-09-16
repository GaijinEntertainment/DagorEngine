.. _datatypes_and_values:

=====================
Values and Data Types
=====================

daScript is a strong, statically typed language.  All variables have a type.
daScript's basic POD (plain old data) data types are::

    int, uint, float, bool, double, int64, uint64
    int2, int3, int4, uint2, uint3, uint4, float2, float3, float4

All PODs are represented with machine register/word. All PODs are passed to functions by value.

daScript's storage types are::

    int8, uint8, int16, uint16 - 8/16-bits signed and unsigned integers

They can't be manipulated, but can be used as storage type within structs, classes, etc.

daScript's other types are::

    string, das_string, struct, pointers, references, block, lambda, function pointer,
    array, table, tuple, variant, iterator, bitfield


All daScript's types are initialized with zeroed memory by default.

.. _userdata-index:

--------
Integer
--------

An integer represents a 32-bit (un)signed number::

    let a = 123    // decimal, integer
    let u = 123u   // decimal, unsigned integer
    let h = 0x0012 // hexadecimal, unsigned integer
    let o = 075    // octal, unsigned integer

    let a = int2(123, 124)    // two integers type
    let u = uint2(123u, 124u) // two unsigned integer type

--------
Float
--------

A float represents a 32-bit floating point number::

    let a = 1.0
    let b = 0.234
    let a = float2(1.0, 2.0)

--------
Bool
--------

A bool is a double-valued (Boolean) data type. Its literals are ``true``
and ``false``. A bool value expresses the validity of a condition
(tells whether the condition is true or false)::

    let a = true
    let b = false

All conditionals (if, elif, while) work only with the bool type.

--------
String
--------

Strings are an immutable sequence of characters. In order to modify a
string, it is necessary to create a new one.

daScript's strings are similar to strings in C or C++.  They are
delimited by quotation marks(``"``) and can contain escape
sequences (``\t``, ``\a``, ``\b``, ``\n``, ``\r``, ``\v``, ``\f``,
``\\``, ``\"``, ``\'``, ``\0``, ``\x<hh>``, ``\u<hhhh>`` and
``\U<hhhhhhhh>``)::

    let a = "I'm a string\n"
    let a = "I'm also
        a multi-line
             string\n"

Strings type can be thought of as a 'pointer to the actual string', like a 'const char \*' in C.
As such, they will be passed to functions by value (but this value is just a reference to the immutable string in memory).

``das_string`` is a mutable string, whose content can be changed. It is simply a builtin handled type, i.e., a std::string bound to daScript.
As such, it passed as reference.

--------
Table
--------

Tables are associative containers implemented as a set of key/value pairs::

    var tab: table<string; int>
    tab["10"] = 10
    tab["20"] = 20
    tab["some"] = 10
    tab["some"] = 20 // replaces the value for 'some' key

(see :ref:`Tables <tables>`).

--------
Array
--------

Arrays are simple sequences of objects. There are static arrays (fixed size) and dynamic arrays (container, size is dynamic).  The index always starts from 0::

    var a = [[int[4] 1; 2; 3; 4]] // fixed size of array is 4, and content is [1, 2, 3, 4]
    var b: array<string>          // empty dynamic array
    push(b,"some")                // now it is 1 element of "some"

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

Functions are similar to those in most other languages::

    def twice(a: int): int
        return a + a

However, there are generic (templated) functions, which will be 'instantiated' during function calls by type inference::

    def twice(a)
        return a + a

    let f = twice(1.0) // 2.0 float
    let i = twice(1)   // 2 int

(see :ref:`Functions <functions>`).

--------------
Reference
--------------

References are types that 'reference' (point to) some other data::

    def twice(var a: int&)
        a = a + a
    var a = 1
    twice(a) // a value is now 2

All structs are always passed to functions arguments as references.


--------------
Pointers
--------------

Pointers are types that 'reference' (point to) some other data, but can be null (point to nothing).
In order to work with actual value, one need to dereference it using the dereference or safe navigation operators.
Dereferencing will panic if a null pointer is passed to it.
Pointers can be created using the new operator, or with the C++ environment.
::

    def twice(var a: int&)
        a = a + a
    def twicePointer(var a: int?)
        twice(*a)

    struct Foo
        x: int

    def getX(foo: Foo?)  // it returns either foo.x or -1, if foo is null
       return foo?.x ?? -1

-----------
Iterators
-----------

Iterators are a sequence which can be traversed, and associated data retrieved.
They share some similarities with C++ iterators.

(see :ref:`Iterators <iterators>`).
