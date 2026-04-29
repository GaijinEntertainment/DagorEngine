.. _functions:

========
Function
========

.. index::
    single: Functions

Function pointers are first-class values, like integers or strings, and can be stored in table slots,
local variables, arrays, and passed as function parameters.
Functions themselves are declarations (much like in C++).

--------------------
Function declaration
--------------------

.. index::
    single: Function Declaration

Functions are similar to those in most other typed languages:

.. code-block:: das

    def twice(a: int): int {
        return a+a
    }

Completely empty functions (without arguments) can be also declared:

.. code-block:: das

    def foo {
        print("foo")
    }

    //same as above
    def foo() {
        print("foo")
    }

Daslang can always infer a function's return type.
Returning different types is a compilation error:

.. code-block:: das

    def foo(a:bool) {
        if ( a ) {
            return 1
        } else {
            return 2.0  // error, expecting int
        }
    }

The return type can be specified explicitly with ``:`` or ``->`` — both are equivalent:

.. code-block:: das

    def add(a, b : int) : int {
        return a + b
    }

    def add(a, b : int) -> int {   // same as above
        return a + b
    }


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Publicity
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Functions can be ``private`` or ``public``

.. code-block:: das

    def private foo(a:bool)

    def public bar(a:float)

If not specified, functions inherit module publicity (i.e. in public modules functions are public,
and in private modules functions are private).

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Function calls
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can call a function by using its name and passing in all its arguments (with the possible omission of the default arguments):

.. code-block:: das

    def foo(a, b: int) {
        return a + b
    }

    def bar {
        foo(1, 2) // a = 1, b = 2
    }

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Named Arguments Function call
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can also call a function by using its name and passing all its arguments with explicit names (with the possible omission of the default arguments):

.. code-block:: das

    def foo(a, b: int) {
        return a + b
    }

    def bar {
        foo([a = 1, b = 2])  // same as foo(1, 2)
    }

Named arguments should be still in the same order:

.. code-block:: das

    def bar {
        foo([b = 1, a = 2])  // error, out of order
    }

Named argument calls increase the readability of callee code and ensure correctness in refactorings of the existing functions.
They also allow default values for arguments other than the last ones:

.. code-block:: das

    def foo(a:int=13, b: int) {
        return a + b
    }

    def bar {
        foo([b = 2])  // same as foo(13, 2)
    }


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Function pointer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Pointers to a function use a similar declaration to that of a block or lambda.
The type is written as ``function`` followed by an optional type signature in angle brackets:

.. code-block:: das

    function < (arg1:int; arg2:float&) : bool >

The ``->`` operator can be used instead of ``:`` for the return type:

.. code-block:: das

    function < (arg1:int; arg2:float&) -> bool >   // equivalent

If no type signature is specified, ``function`` alone represents a function pointer with
an unspecified signature.

Function pointers can be obtained by using the ``@@`` operator:

.. code-block:: das

    def twice(a:int) {
        return a + a
    }

    let fn = @@twice

When multiple functions have the same name, a pointer can be obtained by explicitly specifying signature:

.. code-block:: das

    def twice(a:int) {
        return a + a
    }

    def twice(a:float) {  // when this one is required
        return a + a
    }

    let fn = @@<(a:float):float> twice

Function pointers can be called via ``invoke`` or via call notation:

.. code-block:: das

    let t = invoke(fn, 1)   // t = 2
    let t = fn(1)           // t = 2, same as

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Nameless functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Pointers to nameless functions can be created with a syntax
similar to that of lambdas or blocks (see :ref:`Blocks <blocks_declarations>`):

.. code-block:: das

    let fn <- @@ ( a : int ) {
        return a + a
    }

Nameless local functions do not capture variables at all:

.. code-block:: das

    var count = 1
    let fn <- @@ ( a : int ) {
        return a + count            // compilation error, can't locate variable count
    }

Internally, a regular function will be generated:

.. code-block:: das

    def _localfunction_thismodule_8_8_1`function ( a:int const ) : int {
            return a + a
    }

    let fn:function<(a:int const):int> const <- @@_localfunction_thismodule_8_8_1`function

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Generic functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Generic functions are similar to C++ templated functions.
Daslang will instantiate them during the infer pass of compilation:

.. code-block:: das

    def twice(a) {
        return a + a
    }

    let f = twice(1.0)  // 2.0 float
    let i = twice(1)    // 2 int

Generic functions allow code similar to dynamically-typed languages like Python or Lua,
while still enjoying the performance and robustness of strong, static typing.

You cannot take the address of a generic function.

Unspecified types can also be written via ``auto`` notation:

.. code-block:: das

    def twice(a:auto) {   // same as 'twice' above
        return a + a
    }

Generic functions can specialize generic type aliases, and use them as part of the declaration:

.. code-block:: das

    def twice(a:auto(TT)) : TT {
        return a + a
    }

In the example above, alias ``TT`` is used to enforce the return type contract.

Type aliases can be used before the corresponding ``auto``:

.. code-block:: das

    def summ(base : TT; a:auto(TT)[] ) {
        var s = base
        for ( x in a ) {
            s += x
        }
        return s
    }

In the example above, ``TT`` is inferred from the type of the passed array ``a``, and expected as a first argument ``base``.
The return type is inferred from the type of ``s``, which is also ``TT``.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Function overloading
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Functions can be specialized if their argument types are different:

.. code-block:: das

    def twice(a: int) {
        print("int")
        return a + a
    }
    def twice(a: float) {
        print("float")
        return a + a
    }

    let i = twice(1)    // prints "int"
    let f = twice(1.0)  // prints "float"

Declaring functions with the same exact argument list is a compilation-time error.

Functions can be partially specialized:

.. code-block:: das

    def twice(a:int) {      // int
        return a + a
    }
    def twice(a:float) {    // float
        return a + a
    }
    def twice(a:auto[]) {   // any array
        return length(a)*2
    }
    def twice(a) {          // any other case
        return a + a
    }

Daslang uses the following rules for matching partially specialized functions:

    1. Non-``auto`` is more specialized than ``auto``.
    2. If both are non-``auto``, the one without a cast is more specialized.
    3. Ones with arrays are more specialized than ones without. If both have an array, the one with the actual value is more specialized than the one without.
    4. Ones with a base type of auto\alias are less specialized. If both are auto\alias, it is assumed that they have the same level of specialization.
    5. For pointers and arrays, the subtypes are compared.
    6. For tables, tuples and variants, subtypes are compared, and all must be the same or equally specialized.
    7. For functions, blocks, or lambdas, subtypes and return types are compared, and all must be the same or equally specialized.

When matching functions, Daslang picks the ones which are most specialized and sorts by substitute distance.
Substitute distance is increased by 1 for each argument if a cast is required for the LSP (Liskov substitution principle).
At the end, the function with the least distance is picked. If more than one function is left for picking, a compilation error is reported.

Function specialization can be limited by contracts (contract macros):

.. code-block:: das

    [expect_any_array(blah)]  // array<foo>, [], or dasvector`.... or similar
    def print_arr ( blah ) {
        for ( i in range(length(blah)) ) {
            print("{blah[i]}\n")
        }
    }

In the example above, only arrays will be matched.

It is possible to use boolean logic operations on contracts:

.. code-block:: das

    [expect_any_tuple(blah) || expect_any_variant(blah)]
    def print_blah ...

In the example above print_blah will accept any tuple or variant.
Available logic operations are ``!``, ``&&``, ``||`` and ``^^``.

LSP can be explicitly prohibited for a particular function argument via the ``explicit`` keyword:

.. code-block:: das

    def foo ( a : Foo explicit ) // will accept Foo, but not any subtype of Foo

^^^^^^^^^^^^^^^^^^
Default Parameters
^^^^^^^^^^^^^^^^^^

.. index::
    single: Function Default Parameters

Daslang's functions can have default parameters.

A function with default parameters is declared as follows:

.. code-block:: das

    def test(a, b: int, c: int = 1, d: int = 1) {
        return a + b + c + d
    }

When the function *test* is invoked and the parameters ``c`` or ``d`` are not specified,
the compiler will generate a call with default value to the unspecified parameter. A default parameter can be
any valid compile-time const Daslang expression. The expression is evaluated at compile-time.

It is valid to declare default values for arguments other than the last one:

.. code-block:: das

    def test(c: int = 1, d: int = 1, a, b: int) { // valid!
        return a + b + c + d
    }

Calling such functions with default arguments requires a named arguments call:

.. code-block:: das

    test(2, 3)           // invalid call, a,b parameters are missing
    test([a = 2, b = 3]) // valid call

Default arguments can be combined with overloading:

.. code-block:: das

    def test(c: int = 1, d: int = 1, a, b: int) {
        return a + b + c + d
    }
    def test(a, b: int) { // now test(2, 3) is valid call
        return test([a = a, b = b])
    }

---------------
OOP-style calls
---------------

There are no methods or function members of structs in Daslang.
However, code can be easily written "OOP style" by using the right pipe operator ``|>``:

.. code-block:: das

    struct Foo {
        x, y: int = 0
    }

    def setXY(var thisFoo: Foo; x, y: int) {
        thisFoo.x = x
        thisFoo.y = y
    }
    ...
    var foo:Foo
    foo |> setXY(10, 11)   // this is syntactic sugar for setXY(foo, 10, 11)
    setXY(foo, 10, 11)     // exactly same as above line


(see :ref:`Structs <structs>`).

---------------------------------------------
Tail Recursion
---------------------------------------------

.. index::
    single: Tail Recursion

Tail recursion is a method for partially transforming recursion in a program into
iteration: it applies when the recursive calls in a function are the last executed
statements in that function (just before the return).

Currently, Daslang does not support tail recursion.
A Daslang function is assumed to always return.

---------------------------------------------
Operator Overloading
---------------------------------------------

Daslang allows you to overload operators, which means that you can define custom behavior for operators when used with your own data types.
To overload an operator, you need to define a special function with the name of the operator you want to overload. Here's the syntax:

.. code-block:: das

    def operator <operator>(<arguments>) : <return_type>
        # Implementation here

In this syntax, ``<operator>`` is the name of the operator you want to overload (e.g. ``+``, ``-``, ``*``, ``/``, ``==``, etc.),
``<arguments>`` are the parameters that the operator function takes, and ``<return_type>`` is the return type of the operator function.

For example, here's how you could overload the == operator for a custom struct called iVec2:

.. code-block:: das

    struct iVec2 {
        x, y: int
    }

    def operator==(a, b: iVec2) {
        return (a.x == b.x) && (a.y == b.y)
    }

In this example, we define a structure called iVec2 with two integer fields (x and y).

We then define an operator== function that takes two parameters (a and b) of type iVec2. This function returns a bool value indicating whether a and b are equal.
The implementation checks whether the x and y components of a and b are equal using the == operator.

With this operator overloaded, you can now use the == operator to compare iVec2 objects, like this:

.. code-block:: das

    let v1 = iVec2(1, 2)
    let v2 = iVec2(1, 2)
    let v3 = iVec2(3, 4)

    print("{v1==v2}") // prints "true"
    print("{v1==v3}") // prints "false"

In this example, we create three iVec2 objects and compare them using the == operator. The first comparison (v1 == v2) returns true because the x and y components of v1 and v2 are equal.
The second comparison (v1 == v3) returns false because the x and y components of v1 and v3 are not equal.

---------------------------------------------
Overloadable operators
---------------------------------------------

The following table lists all operators that can be overloaded in Daslang:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Category
     - Operators
   * - Arithmetic
     - ``+``  ``-``  ``*``  ``/``  ``%``
   * - Comparison
     - ``==``  ``!=``  ``<``  ``>``  ``<=``  ``>=``
   * - Bitwise
     - ``&``  ``|``  ``^``  ``~``  ``<<``  ``>>``  ``<<<``  ``>>>``
   * - Logical
     - ``&&``  ``||``  ``^^``  ``!``
   * - Unary
     - ``-`` (negate)  ``~`` (complement)  ``++``  ``--``
   * - Compound assignment
     - ``+=``  ``-=``  ``*=``  ``/=``  ``%=``  ``&=``  ``|=``  ``^=``  ``<<=``  ``>>=``  ``<<<=``  ``>>>=``  ``&&=``  ``||=``  ``^^=``
   * - Index
     - ``[]``  ``[]=``  ``[]<-``  ``[]:=``  ``[]+=``  ``[]-=``  ``[]*=``  etc.
   * - Safe index
     - ``?[]``
   * - Dot
     - ``.``  ``?.``  ``. name``  ``. name :=``  ``. name +=``  etc.
   * - Type
     - ``:=`` (clone)  ``delete`` (finalize)  ``is``  ``as``  ``?as``
   * - Null coalesce
     - ``??``
   * - Interval
     - ``..``

Operators can be defined as free functions or as struct methods.

---------------------------------------------
Unary operators
---------------------------------------------

Unary operators take a single argument. To overload unary minus (negate):

.. code-block:: das

    def operator -(a : Vec2) : Vec2 {
        return Vec2(x = -a.x, y = -a.y)
    }

Prefix increment (``++x``) and postfix increment (``x++``) are separate operators.
In the parser, ``++operator`` is the prefix form and ``operator++`` is the postfix form:

.. code-block:: das

    struct Counter {
        value : int
    }

    def operator ++(var c : Counter) : Counter {
        c.value += 1
        return c
    }

The same pattern applies to ``--``.

---------------------------------------------
Compound assignment operators
---------------------------------------------

Compound assignment operators modify the left-hand operand in place.
The first parameter must be a mutable reference (``var ... &``):

.. code-block:: das

    def operator +=(var a : Vec2&; b : Vec2) {
        a.x += b.x
        a.y += b.y
    }

    def operator *=(var a : Vec2&; s : float) {
        a.x *= s
        a.y *= s
    }

This pattern works for all compound assignments: ``-=``, ``/=``, ``%=``, ``&=``, ``|=``, ``^=``,
``<<=``, ``>>=``, ``<<<=``, ``>>>=``, ``&&=``, ``||=``, ``^^=``.

---------------------------------------------
Index operators
---------------------------------------------

Index operators control how ``[]`` behaves on your types.

``operator []`` defines read access, ``operator []=`` defines write access, and
compound variants like ``operator []+=`` define in-place index operations:

.. code-block:: das

    struct Matrix2x2 {
        data : float[4]
    }

    def operator [](m : Matrix2x2; i : int) : float {
        return m.data[i]
    }

    def operator []=(var m : Matrix2x2&; i : int; v : float) {
        m.data[i] = v
    }

    def operator []+=(var m : Matrix2x2&; i : int; v : float) {
        m.data[i] += v
    }

Additional index operators include ``[]<-`` (move into index), ``[]:=`` (clone into index),
``[]-=``, ``[]*=``, and others matching the compound assignment family.

The safe index operator ``?[]`` can be overloaded to return a default value when the
index is out of range.

---------------------------------------------
Clone and finalize operators
---------------------------------------------

``operator :=`` overloads clone behaviour:

.. code-block:: das

    struct Resource {
        name : string
        refcount : int
    }

    def operator :=(var dst : Resource&; src : Resource) {
        dst.name = src.name
        dst.refcount = src.refcount + 1
    }

Custom finalization can be defined via a ``finalize`` function or ``operator delete``:

.. code-block:: das

    def finalize(var r : Resource) {
        print("releasing {r.name}\n")
    }

---------------------------------------------
is, as, and ?as operators
---------------------------------------------

The ``is``, ``as``, and ``?as`` operators can be overloaded for custom type-checking
and casting behaviour:

.. code-block:: das

    def operator is(a : MyVariant; b : type<int>) : bool {
        // return true if MyVariant currently holds an int
    }

    def operator as(a : MyVariant; b : type<int>) : int {
        // extract int value, panic if wrong type
    }

    def operator ?as(a : MyVariant; b : type<int>) : int? {
        // extract int value or return null
    }

These are commonly used with variant types and in libraries like ``daslib/ast_boost``
and ``daslib/json_boost``.

---------------------------------------------
Null-coalesce operator
---------------------------------------------

``operator ??`` can be overloaded to provide a default value when a nullable
or optional type is null:

.. code-block:: das

    def operator ??(a : MyOptional; default_value : int) : int {
        // return contained value or default_value
    }

---------------------------------------------
Struct method operators
---------------------------------------------

Operators can be defined as struct methods instead of free functions:

.. code-block:: das

    struct Stack {
        items : array<int>

        def const operator [](index : int) : int {
            return items[index]
        }

        def operator []=(index : int; value : int) {
            items[index] = value
        }
    }

Use the ``const`` qualifier on read-only operators. Write operators omit ``const``
because they mutate the struct's state.

---------------------------------------------
Overloading the '.' and '?.' operators
---------------------------------------------

Daslang allows you to overload the dot . operator, which is used to access fields of structure or a class.
To overload the dot . operator, you need to define a special function with the name operator `.` Here's the syntax:

.. code-block:: das

    def operator.(<object>: <type>, <name>: string) : <return_type>
        # Implementation here

Alternatively you can specify field explicitly:

.. code-block:: das

    def operator.<name> (<object>: <type>) : <return_type>
        # Implementation here

In this syntax, <object> is the object you want to access, <type> is the type of the object, <name> is the name of the field you want to access, and <return_type> is the return type of the operator function.

Operator ?. works in a similar way.

For example, here's how you could overload the dot . operator for a custom structure called Goo:

.. code-block:: das

    struct Goo {
        a: string
    }
    def operator.(t: Goo, name: string) : string {
        return "{name} = {t . . a}"
    }
    def operator. length(t: Goo) : int {
        return length(t . . a)
    }

In this example, we define a struct called Goo with a string field called a.

We then define two operator. functions:

The first one takes two parameters (t and name) and returns a string value that contains the name of the field or method being accessed (name)
and the value of the a field of the Goo object (t.a).
The second one takes one parameter (t) and returns the length of the a field of the Goo object (t.a).
With these operators overloaded, you can now use the dot . operator to access fields and methods of a Goo object, like this:

.. code-block:: das

    var g = Goo(a ="hello")
    var field = g.a
    var length = g.length

In this example, we create an instance of the Goo struct and access its world field using the dot . operator.
The overloaded operator. function is called and returns the string "world = hello".
We also access the length property of the Goo object using the dot . operator.
The overloaded operator. length function is called and returns the length of the a field of the Goo object (5 in this case).

The . . syntax is used to access the fields of a structure or a class while bypassing overloaded operations.

---------------------------------------------
Overloading accessors
---------------------------------------------

Daslang allows you to overload accessors, which means that you can define custom behavior for accessing fields of your own data types.
Here is an example of how to overload the accessor for a custom struct called Foo:

.. code-block:: das

    require math

    struct Foo {
        dir : float3
    }
    def operator . magnitude ( foo : Foo ) : float {
        return length(foo.dir)
    }
    def operator . magnitude := ( var foo:Foo; value:float ) {
        foo.dir = normalize(foo.dir) * value
    }
    [export]
    def main {
        var f = Foo(dir=float3(1,2,3))
        print("magnitude = {f.magnitude} // {f}\n")
        f.magnitude := 10.
        print("magnitude = {f.magnitude} // {f}\n")
    }

Expected output:

.. code-block:: text

    magnitude = 3.7416575 // [[ 1,2,3]]
    magnitude = 10 // [[ 2.6726124,5.345225,8.017837]]

It now has accessor ``magnitude`` which can be used to get and set the magnitude of the ``dir`` field.

Classes allow to overload accessors for properties as well:

.. code-block:: das

    class Foo {
        dir : float3
        def const operator . magnitude : float {
            return length(dir)
        }
        def operator . magnitude := ( value:float ) {
            dir = normalize(dir) * value
        }
    }

.. seealso::

    :ref:`Generic programming <generic_programming>` for generic functions and type inference,
    :ref:`Lambdas <lambdas>` for anonymous callable objects with captures,
    :ref:`Blocks <blocks>` for stack-bound callable objects,
    :ref:`Annotations <annotations>` for function annotations like ``[export]`` and ``[private]``,
    :ref:`Move, copy, and clone <clone_to_move>` for ``return <-`` move semantics,
    :ref:`Classes <classes>` for member functions and method-like calls.
