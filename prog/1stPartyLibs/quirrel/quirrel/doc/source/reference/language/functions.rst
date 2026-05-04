.. _functions:


=================
Functions
=================

.. index::
    single: Functions

Functions are first-class values like integers or strings and can be stored in table slots,
local variables, arrays and passed as function parameters.
Functions can be implemented in Quirrel or in a native language with calling conventions
compatible with ANSI C.

--------------------
Function declaration
--------------------

.. index::
    single: Function Declaration

A local function can be declared with the following function expression::

    function tuna(a,b,c) {
        return a+b-c;
    }

    let function tuna(a,b,c) {
        return a+b-c;
    }

    let tuna = function tuna(a,b,c) {
        return a+b-c;
    }

These declarations mean that `tuna` can't be reassigned (see `bindings`)

or::

    local tuna = function tuna(a,b,c) {
        return a+b-c;
    }

    local function tuna(a,b,c) {
        return a+b-c;
    }


These declarations declare a local variable that can be reassigned

^^^^^^^^^^^^^^^^^^
Default Parameters
^^^^^^^^^^^^^^^^^^

.. index::
    single: Function Default Parameters

Quirrel functions can have default parameters.

A function with default parameters is declared as follows: ::

    function test(a, b, c = 10, d = 20) {
        ....
    }

when the function *test* is invoked and parameter c or d is not specified,
the VM automatically assigns the default value to the unspecified parameter. A default parameter can be
any valid quirrel expression. The expression is evaluated at runtime.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Functions with variable number of parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. index::
    single: Functions with variable number of parameters

Quirrel functions can have a variable number of parameters (varargs functions).

A vararg function is declared by adding three dots (``...``) at the end of its parameter list.

When the function is called all the extra parameters will be accessible through the *array*
called ``vargv``, that is passed as an implicit parameter.

``vargv`` is a regular quirrel array and can be used accordingly.::

    function test(a,b,...) {

        for (local i = 0; i< vargv.len(); i++) {
            println($"varparam {i} = {vargv[i]}")
        }
        foreach (i,val in vargv) {
            println($"varparam {i} = {val}")
        }
    }

    test("goes in a","goes in b",0,1,2,3,4,5,6,7,8)

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Function attributes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. index::
    single: Function attributes

A function can have attributes that can be used to provide additional information about the function.
Currently the only supported attribute is ``pure``.
A pure function is a function that does not have side effects and does not modify any
external state. This can be used to optimize the evaluation of constant expressions.

A pure function is declared as follows: ::

    function [pure] test(a, b) {
        return a + b
    }

or, in lambda form ::

    @[pure](a, b) {
        return a + b
    }


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Type Annotations in Functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. index::
    single: Function Type Annotations

Quirrel supports optional **type annotations** for function parameters and return values. Type annotations are validated during execution; passing a value of an incompatible type to a parameter or returning a mismatched type results in a runtime error. Type annotations improve code readability, enable better tooling support, and help document expected interfaces.

A function with type annotations uses the following syntax::

    function name(param1: Type1, param2: Type2, ...: VarargType): ReturnType {
        // body
    }

Parameter types are specified after a colon following the parameter name, and the return type is declared after the closing parenthesis of the parameter list, also preceded by a colon.

Supported types include:

- Primitive types: ``int``, ``float``, ``number`` (shorthand for ``int | float``), ``bool``, ``string``
- Container and system types: ``table``, ``array``, ``function``, ``thread``, ``userdata``, ``generator``, ``userpointer``, ``instance``, ``class``, ``weakref``
- Special types: ``null``, ``any``
- Union types using the ``|`` operator: e.g., ``number | null``

Union types may be wrapped in parentheses for clarity::

    function f(x: (number | null)): (null | number)

Variadic parameters (``...``) can also be annotated to indicate the expected type of extra arguments::

    function type_test2(x: int, ...: float): int {
        return 1 + x
    }

Here, ``...: float`` documents that all variadic arguments are expected to be of type ``float``.

Default parameters may carry type annotations as well::

    function type_test4(x: number|null = null): null|number {
        return 1.0 + (x ?? 6)
    }

Lambda (anonymous) functions fully support type annotations::

    let type_test8 = @(x: bool|number|string, y: any): any (
        x + y
    )

The ``any`` type disables static checking for that position and accepts any value. It is useful when exact types are unknown or intentionally flexible.

Function attributes (such as ``[pure]``) can be combined with type annotations::

    let type_test10 = (@ [pure] type_test10(x: bool|number|int|null, y: any): any (
        x + y
    ))


---------------
Function calls
---------------

.. index::
    single: Function calls

::

    exp:= derefexp '(' explist ')'

The expression is evaluated in this order: derefexp after the explist (arguments) and at
the end the call.

A function call in Quirrel passes the current environment object *this* as a hidden parameter.
But when the function was immediately indexed from an object, *this* shall be the object
which was indexed, instead.

If we call a function with the syntax::

    mytable.foo(x,y)

the environment object passed to 'foo' as *this* will be 'mytable' (since 'foo' was immediately indexed from 'mytable')

Whereas with the syntax::

    foo(x,y) // implicitly equivalent to this.foo(x,y)

the environment object will be the current *this* (that is, propagated from the caller's *this*).

It may help to remember the rules in the following way:

    ``foo(x,y)`` ---> ``this.foo(x,y)``

    ``table.foo(x,y)`` ---> call ``foo`` with ``(table,x,y)``

It may also help to consider why it works this way: it was initially designed to assist with object-oriented style.
When calling ``foo(x,y)`` it was assumed you're calling another member of the object (or of the file) and
so should operate on the same object.
When calling ``mytable.foo(x,y)`` it is written plainly that you're calling a member of a different object.

---------------------------------------------
Binding an environment to a function
---------------------------------------------

.. index::
    single: Binding an environment to a function

while by default a quirrel function call passes as environment object ``this``, the object
from which the function was indexed. However, it is also possible to statically bind an environment to a
closure using the built-in method ``closure.bindenv(env_obj)``.
The method ``bindenv()`` returns a new instance of a closure with the environment bound to it.
When an environment object is bound to a function, every time the function is invoked, its
``this`` parameter will always be the previously bound environment.
This mechanism is useful to implement callback systems similar to C# delegates.

.. note:: The closure keeps a weak reference to the bound environment object, because of this if
          the object is deleted, the next call to the closure will result in a ``null``
          environment object.

---------------------------------------------
Lambda Expressions
---------------------------------------------

.. index::
    single: Lambda Expressions

::

    exp := '@' '(' paramlist ')' exp

Lambda expressions are syntactic sugar to quickly define a function that consists of a single expression.
This feature comes in handy when functional programming patterns are applied, like map/reduce or passing a compare method to
array.sort().

here is a lambda expression::

    let myexp = @(a,b) a + b

that is equivalent to::

    let myexp = function(a,b) { return a + b }

a more useful usage could be::

    let arr = [2,3,5,8,3,5,1,2,6]
    arr.sort(@(a,b) a <=> b)
    arr.sort(@(a,b) -(a <=> b))

that could have been written as::

    let arr = [2,3,5,8,3,5,1,2,6]
    arr.sort(function(a,b) { return a <=> b } )
    arr.sort(function(a,b) { return -(a <=> b) } )

other than being limited to a single expression lambdas support all features of regular functions.
in fact they are implemented as a compile-time feature.

---------------------------------------------
Free Variables
---------------------------------------------

.. index::
    single: Free Variables

A free variable is a variable external to the function scope and is not a local variable
or parameter of the function.
Free variables reference a local variable from an outer scope.
In the following example the variables ``testy``, ``x`` and ``y`` are bound to the function ``foo``.::

    local x = 10
    local y = 20
    let testy = "I'm testy"

    let function foo(a,b) {
        print(testy)
        return a+b+x+y
    }

A program can read or write a free variable.

---------------------------------------------
Tail Recursion
---------------------------------------------

.. index::
    single: Tail Recursion

Tail recursion is a method for partially transforming recursion in a program into
iteration: it applies when the recursive calls in a function are the last executed
statements in that function (just before the return).
If this happens the quirrel interpreter collapses the caller stack frame before the
recursive call; because of that very deep recursions are possible without risk of a stack
overflow.::

    function loopy(n) {
        if (n > 0) {
            println($"n={n}")
            return loopy(n-1)
        }
    }

    loopy(1000)
