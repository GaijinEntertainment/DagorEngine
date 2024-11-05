.. _generic_programming:


===================
Generic Programming
===================

.. index::
    single: Generic Programming

Daslang allows omission of types in statements, functions, and function declaration, making writing in it similar to dynamically typed languages, such as Python or Lua.
Said functions are *instantiated* for specific types of arguments on the first call.

There are also ways to inspect the types of the provided arguments, in order to change the behavior of function, or to provide reasonable meaningful errors during the compilation phase.
Most of these ways are achieved with s

Unlike C++ with its SFINAE, you can use common conditionals (if) in order to change the instance of the function depending on the type info of its arguments.
Consider the following example::

    def setSomeField(var obj; val)
        if typeinfo(has_field<someField> obj)
            obj.someField = val

This function sets ``someField`` in the provided argument *if* it is a struct with a ``someField`` member.

We can do even more.  For example::

    def setSomeField(var obj; val: auto(valT))
        if typeinfo(has_field<someField> obj)
            if typeinfo(typename obj.someField) == typeinfo(typename type valT -const)
                obj.someField = val

This function sets ``someField`` in the provided argument *if* it is a struct with a ``someField`` member, and only if ``someField`` is of the same type as ``val``!

^^^^^^^^^
typeinfo
^^^^^^^^^

Most type reflection mechanisms are implemented with the typeinfo operator. There are:

    * ``typeinfo(typename object)`` // returns typename of object
    * ``typeinfo(fulltypename object)`` // returns full typename of object, with contracts (like !const, or !&)
    * ``typeinfo(sizeof object)`` // returns sizeof
    * ``typeinfo(is_pod object)`` // returns true if object is POD type
    * ``typeinfo(is_raw object)`` // returns true if object is raw data, i.e., can be copied with memcpy
    * ``typeinfo(is_struct object)`` // returns true if object is struct
    * ``typeinfo(has_field<name_of_field> object)`` // returns true if object is struct with field name_of_field
    * ``typeinfo(is_ref object)`` // returns true if object is reference to something
    * ``typeinfo(is_ref_type object)`` // returns true if object is of reference type (such as array, table, das_string or other handled reference types)
    * ``typeinfo(is_const object)`` // returns true if object is of const type (i.e., can't be modified)
    * ``typeinfo(is_pointer object)`` // returns true if object is of pointer type, i.e., int?

All typeinfo can work with types, not objects, with the ``type`` keyword::

    typeinfo(typename type int) // returns "int"

^^^^^^^^^^^^^^^^^^^^^^^^^^^
auto and auto(named)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Instead of ommitting the type name in a generic, it is possible to use an explicit ``auto`` type or ``auto(name)`` to type it::

    def fn(a: auto): auto
        return a

or ::

    def fn(a: auto(some_name)): some_name
        return a

This is the same as::

    def fn(a)
        return a

This is very helpful if the function accepts numerous arguments, and some of them have to be of the same type::

    def fn(a, b) // a and b can be of different types
        return a + b

This is not the same as::

    def fn(a, b: auto) // a and b are one type
        return a + b

Also, consider the following::

    def set0(a, b; index: int) // a is only supposed to be of array type, of same type as b
        return a[index] = b

If you call this function with an array of floats and an int, you would get a not-so-obvious compiler error message::

    def set0(a: array<auto(some)>; b: some; index: int) // a is of array type, of same type as b
        return a[index] = b

Usage of named ``auto`` with ``typeinfo`` ::

    def fn(a: auto(some))
        print(typeinfo(typename type some))

    fn(1) // print "const int"

You can also modify the type with delete syntax::

    def fn(a: auto(some))
        print(typeinfo(typename type some -const))

    fn(1) // print "int"

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
type contracts and type operations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Generic function arguments, result, and inferred type aliases can be operated on during the inference.

`const` specifies, that constant and regular expressions will be matched::

    def foo ( a : Foo const )   // accepts Foo and Foo const

`==const` specifies, that const of the expression has to match const of the argument::

    def foo ( a : Foo const ==const )   // accepts Foo const only
    def foo ( var a : Foo ==const )     // accepts Foo only

`-const` will remove const from the matching type::

    def foo ( a : array<auto -const> )  // matches any array, with non-const elements

`#` specifies that only temporary types are accepted::

    def foo ( a : Foo# )    // accepts Foo# only

`-#` will remove temporary type from the matching type::

    def foo ( a : auto(TT) )        // accepts any type
        var temp : TT -# := a;      // TT -# is now a regular type, and when `a` is temporary, it can clone it into `temp`

`&` specifies that argument is passed by reference::

    def foo ( a : auto& )           // accepts any type, passed by reference

`==&` specifies that reference of the expression has to match reference of the argument::

    def foo ( a : auto& ==& )   // accepts any type, passed by reference (for example variable i, even if its integer)
    def foo ( a : auto ==& )    // accepts any type, passed by value     (for example value 3)

`-&` will remove reference from the matching type::

    def foo ( a : auto(TT)& )       // accepts any type, passed by reference
        var temp : TT -& = a;       // TT -& is not a local reference

`[]` specifies that the argument is a static array of arbitrary dimension::

    def foo ( a : auto[] )          // accepts static array of any type of any size

`-[]` will remove static array dimension from the matching type::

    def take_dim( a : auto(TT) )
        var temp : TT -[]           // temp is type of element of a
    // if a is int[10] temp is int
    // if a is int[10][20][30] temp is still int

`implicit` specifies that both temporary and regular types can be matched, but the type will be treated as specified. `implicit` is _UNSAFE_::

    def foo ( a : Foo implicit )    // accepts Foo and Foo#, a will be treated as Foo
    def foo ( a : Foo# implicit )   // accepts Foo and Foo#, a will be treated as Foo#

`explicit` specifies that LSP will not be applied, and only exact type match will be accepted::

    def foo ( a : Foo )             // accepts Foo and any type that is inherited from Foo directly or indirectly
    def foo ( a : Foo explicit )    // accepts Foo only

^^^^^^^
options
^^^^^^^

Multiple options can be specified as a function argument::

    def foo ( a : int | float )   // accepts int or float

Optional types always make function generic.

Generic options will be matched in the order listed::

    def foo ( a : Bar explicit | Foo )   // first will try to match exactly Bar, than anything else inherited from Foo

`|#` shortcat matches previous type, with temporary flipped::

    def foo ( a : Foo |# )   // accepts Foo and Foo# in that order
    def foo ( a : Foo# |# )  // accepts Foo# and Foo in that order

^^^^^^^^
typedecl
^^^^^^^^

Consider the following example::

    struct A
        id : string

    struct B
        id : int

    def get_table_from_id(t : auto(T))
        var tab : table<typedecl(t.id); T>  // NOTE typedecl
        return <- tab

    [export]
    def main
        var a : A
        var b : B
        var aTable <- get_table_from_id(a)
        var bTable <- get_table_from_id(b)
        print("{typeinfo(typename aTable)}\n")
        print("{typeinfo(typename bTable)}\n")

Here table is created with a key type of `id` field of the provided struct.
This feature allows to create types based on the provided expression type.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
generic tuples and type<> expressions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Consider the following example::

    tuple Handle
        h : auto(HandleType)
        i : int

    def make_handle ( t : auto(HandleType) ) : Handle
        var h : type<Handle> // NOTE type<Handle>
        return h

    def take_handle ( h : Handle )
        print("count = {h.i} of type {typeinfo(typename type<HandleType>)}\n")

    [export]
    def main
        let h = make_handle(10)
        take_handle(h)

In the function make_handle, the type of the variable h is created with the type<> expression.
type<> is inferred in context (this time based on a function argument).
This feature allows to create types based on the provided expression type.

Generic function take_handle takes any Handle type, but only Handle type tuple.

This carries some similarity to the C++ template system, but is a bit more limited due to tuples being weak types.
