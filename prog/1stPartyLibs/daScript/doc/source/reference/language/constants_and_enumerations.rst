.. _constants_and_enumerations:


=========================================
Constants, Enumerations, Global variables
=========================================

.. index::
    single: Constants, Enumerations, Global variables



daScript allows you to bind constant values to a global variable identifier.
Whenever possible, all constant global variables will be evaluated at compile time.
There are also enumerations, which are strongly typed constant collections similar to enum classes in C++.

--------
Constant
--------

.. index::
    single: Constants

Constants bind a specific value to an identifier. Constants are exactly global variables. Their value cannot be changed.

Constants are declared with the following syntax::

    let
      foobar = 100
    let
      floatbar = 1.2
    let
      stringbar = "I'm a constant string"
    let blah = "I'm string constant which is declared on the same line as variable"

Constants are always globally scoped from the moment they are declared.
Any subsequential code can reference them.

You can not change such global variables.

Constants can be ``shared``::

    let shared blah <- [{string "blah"; "blahh"; "blahh"}]

Shared constants point to the same memory in different instances of `Context`.
They are initialized once during the first context initialization.

---------------
Global variable
---------------

Mutable global variables are defined as::

    var
      foobar = 100
    var barfoo = 100

Their usage can be switched on and off on a per-project basis via `CodeOfPolicies`.

Local static variables can be declared via the `static_let` macro::

    require daslib/static_let

    def foo
        static_let <|
            var bar = 13
        bar = 14

Variable ``bar`` in the example above is effectively a global variable.
However, it's only visible inside the scope of the corresponding `static_let` macro.

Global variables can be `private` or `public` ::

    let public foobar = 100

    let private barfoo = 100

If not specified, structures inherit module publicity (i.e. in public modules global variables are public,
and in private modules global variables are private).


.. _enumerations:

-----------
Enumeration
-----------

.. index::
    single: Enumerations

An enumeration binds a specific value to a name. Enumerations are also evaluated at compile time
and their value cannot be changed.

An enum declaration introduces a new enumeration to the program.
Enumeration values can only be compile time constant expressions.
It is not required to assign specific value to enum::

    enum Numbers
        zero     // will be 0
        one      // will be 1
        two      // will be 2
        ten = 9+1 // will be 10, since its explicitly specified

Enumerations can be `private` or `public`::

    enum private Foo
        fooA
        fooB

    enum public Bar
        barA
        barB

If not specified, enumeration inherit module publicity (i.e. in public modules enumerations are public,
and in private modules enumerations are private).

An enum name itself is a strong type, and all enum values are of this type.
An enum value can be addressed as 'enum name' followed by exact enumeration ::

    let one: Numbers = Numbers one

An enum value can be converted to an integer type with an explicit cast ::

    let one: Numbers = Numbers one
    assert(int(one) == 1)

Enumerations can specify one of the following storage types: ``int``, ``int8``, ``int16``, ``uint``, ``uint8``, or ``uint16``::

    enum Characters : uint8
        ch_a = 'A'
        ch_b = 'B'

Enumeration values will be truncated down to the storage type.

The `each_enum` iterator iterates over specific enumeration type values.
Any enum element needs to be provided to specify the enumeration type::

	for x in each_enum(Characters ch_a)
		print("x = {x}\n")

