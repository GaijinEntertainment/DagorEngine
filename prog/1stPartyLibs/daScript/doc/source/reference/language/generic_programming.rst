.. _generic_programming:


===================
Generic Programming
===================

.. index::
    single: Generic Programming

Daslang allows you to omit types in statements, functions, and function declarations, making the code similar in style to dynamically typed languages such as Python or Lua.
Said functions are *instantiated* for specific types of arguments on the first call.

There are also ways to inspect the types of the provided arguments, in order to change the behavior of a function, or to provide meaningful errors during the compilation phase.

Unlike C++ with its SFINAE, you can use common conditionals (if) in order to change the instance of the function depending on the type info of its arguments.
Consider the following example:

.. code-block:: das

    def setSomeField(var obj; val) {
        if ( typeinfo has_field<someField>(obj) ) {
            obj.someField = val
        }
    }

This function sets ``someField`` in the provided argument *if* it is a struct with a ``someField`` member.

We can do even more.  For example:

.. code-block:: das

    def setSomeField(var obj; val: auto(valT)) {
        if ( typeinfo has_field<someField>(obj) ) {
            if ( typeinfo typename(obj.someField) == typeinfo typename(type<valT -const>) ) {
                obj.someField = val
            }
        }
    }

This function sets ``someField`` in the provided argument *if* it is a struct with a ``someField`` member, and only if ``someField`` is of the same type as ``val``!

typeinfo
--------

The ``typeinfo`` operator provides compile-time type reflection.
It is the primary mechanism for inspecting types in generic functions.

All ``typeinfo`` traits can operate on either an expression or a ``type<T>`` argument:

.. code-block:: das

    typeinfo sizeof(type<float3>)       // 12
    typeinfo typename(my_variable)      // type name of the variable

**Name and String Traits**

* ``typeinfo typename(expr)`` — human-readable type name
* ``typeinfo fulltypename(expr)`` — full type name with contracts (``const``, ``&``, etc.)
* ``typeinfo stripped_typename(expr)`` — type name without ``const``/``temp``/``ref`` decorators
* ``typeinfo undecorated_typename(expr)`` — type name without module prefix
* ``typeinfo modulename(expr)`` — module name of the type
* ``typeinfo struct_name(expr)`` — structure name (for struct/class types)
* ``typeinfo struct_modulename(expr)`` — module name of the structure

**Size and Layout Traits**

* ``typeinfo sizeof(expr)`` — size of the type in bytes
* ``typeinfo alignof(expr)`` — alignment of the type
* ``typeinfo dim(expr)`` — size of the first dimension (fixed-size arrays)
* ``typeinfo offsetof<field>(expr)`` — byte offset of a field in a struct
* ``typeinfo vector_dim(expr)`` — dimension of a vector type (e.g. 3 for ``float3``)

**Boolean Type-Query Traits**

* ``typeinfo is_pod(expr)`` — true if plain-old-data
* ``typeinfo is_raw(expr)`` — true if raw data (can be memcpy'd)
* ``typeinfo is_struct(expr)`` — true if structure type
* ``typeinfo is_class(expr)`` — true if class
* ``typeinfo is_handle(expr)`` — true if handled (native-bound) type
* ``typeinfo is_ref(expr)`` — true if passed/stored by reference
* ``typeinfo is_ref_type(expr)`` — true if reference type by nature (array, table, etc.)
* ``typeinfo is_ref_value(expr)`` — true if has explicit ``ref`` qualifier
* ``typeinfo is_const(expr)`` — true if const-qualified
* ``typeinfo is_temp(expr)`` — true if temporary-qualified
* ``typeinfo is_temp_type(expr)`` — true if temporary type
* ``typeinfo is_pointer(expr)`` — true if pointer type
* ``typeinfo is_smart_ptr(expr)`` — true if smart pointer
* ``typeinfo is_void(expr)`` — true if void
* ``typeinfo is_void_pointer(expr)`` — true if void pointer
* ``typeinfo is_string(expr)`` — true if string type
* ``typeinfo is_numeric(expr)`` — true if numeric type
* ``typeinfo is_numeric_comparable(expr)`` — true if numeric-comparable
* ``typeinfo is_vector(expr)`` — true if vector type (``float2``/``int3``/etc.)
* ``typeinfo is_any_vector(expr)`` — true if handled vector-template type
* ``typeinfo is_array(expr)`` — true if ``array<T>``
* ``typeinfo is_table(expr)`` — true if ``table<K;V>``
* ``typeinfo is_dim(expr)`` — true if has any dimension (fixed-size array)
* ``typeinfo is_enum(expr)`` — true if enumeration
* ``typeinfo is_bitfield(expr)`` — true if bitfield
* ``typeinfo is_tuple(expr)`` — true if tuple
* ``typeinfo is_variant(expr)`` — true if variant
* ``typeinfo is_function(expr)`` — true if function type
* ``typeinfo is_lambda(expr)`` — true if lambda type
* ``typeinfo is_iterator(expr)`` — true if iterator type
* ``typeinfo is_iterable(expr)`` — true if can be iterated with ``for``
* ``typeinfo is_local(expr)`` — true if local type
* ``typeinfo is_workhorse(expr)`` — true if workhorse type

**Capability Traits**

* ``typeinfo can_copy(expr)`` — true if the type can be copied
* ``typeinfo can_move(expr)`` — true if the type can be moved
* ``typeinfo can_clone(expr)`` — true if the type can be cloned
* ``typeinfo can_clone_from_const(expr)`` — true if can be cloned from a const source
* ``typeinfo can_new(expr)`` — true if can be heap-allocated with ``new``
* ``typeinfo can_delete(expr)`` — true if can be deleted
* ``typeinfo can_delete_ptr(expr)`` — true if pointer can be deleted
* ``typeinfo can_be_placed_in_container(expr)`` — true if valid for arrays/tables
* ``typeinfo need_delete(expr)`` — true if requires explicit deletion
* ``typeinfo need_inscope(expr)`` — true if needs ``inscope`` lifetime management
* ``typeinfo has_nontrivial_ctor(expr)`` — true if has non-trivial constructor
* ``typeinfo has_nontrivial_dtor(expr)`` — true if has non-trivial destructor
* ``typeinfo has_nontrivial_copy(expr)`` — true if has non-trivial copy semantics
* ``typeinfo is_unsafe_when_uninitialized(expr)`` — true if unsafe when uninitialized

**Field and Annotation Traits** (see also :ref:`Annotations <annotations>`)

* ``typeinfo has_field<name>(expr)`` — true if the struct/handle has a field named ``name``
* ``typeinfo safe_has_field<name>(expr)`` — same as above, but returns false instead of error
* ``typeinfo has_annotation<name>(expr)`` — true if the struct has annotation ``name``
* ``typeinfo has_annotation_argument<name>(expr)`` — true if annotation has argument ``name``
* ``typeinfo safe_has_annotation_argument<name>(expr)`` — returns false instead of error
* ``typeinfo annotation_argument<name>(expr)`` — returns the value of an annotation argument
* ``typeinfo variant_index<name>(expr)`` — returns the index of a variant field
* ``typeinfo safe_variant_index<name>(expr)`` — returns -1 instead of error

**Existence Checks**

* ``typeinfo builtin_function_exists(expr)`` — true if a ``@@function`` exists
* ``typeinfo builtin_annotation_exists(expr)`` — true if an annotation type exists
* ``typeinfo builtin_module_exists(expr)`` — true if a module is loaded
* ``typeinfo is_argument(expr)`` — true if the expression is a function argument
* ``typeinfo mangled_name(expr)`` — returns the mangled name of a ``@@function``

**User-Defined Traits**

Any trait name not in the list above is dispatched to the TypeInfoMacro system,
allowing user-defined typeinfo extensions (e.g., ``ast_typedecl``).

auto and auto(named)
--------------------

Instead of omitting the type name in a generic, it is possible to use an explicit ``auto`` type or ``auto(name)`` to type it:

.. code-block:: das

    def fn(a: auto): auto {
        return a
    }

or

.. code-block:: das

    def fn(a: auto(some_name)): some_name {
        return a
    }

This is the same as:

.. code-block:: das

    def fn(a) {
        return a
    }

This is very helpful if the function accepts numerous arguments, and some of them have to be of the same type:

.. code-block:: das

    def fn(a, b) { // a and b can be of different types
        return a + b
    }

This is not the same as:

.. code-block:: das

    def fn(a, b: auto) { // a and b are one type
        return a + b
    }

Also, consider the following:

.. code-block:: das

    def set0(a, b; index: int) { // a is only supposed to be of array type, of same type as b
        return a[index] = b
    }

If you call this function with an array of floats and an int, you would get a not-so-obvious compiler error message:

.. code-block:: das

    def set0(a: array<auto(some)>; b: some; index: int) { // a is of array type, of same type as b
        return a[index] = b
    }

Usage of named ``auto`` with ``typeinfo``

.. code-block:: das

    def fn(a: auto(some)) {
        print(typeinfo typename(type<some>))
    }

    fn(1) // print "const int"

You can also modify the type with delete syntax:

.. code-block:: das

    def fn(a: auto(some)) {
        print(typeinfo typename(type<some -const>))
    }

    fn(1) // print "int"

type contracts and type operations
----------------------------------

Generic function arguments, result, and inferred type aliases can be operated on during the inference.

``const`` specifies, that constant and regular expressions will be matched:

.. code-block:: das

    def foo ( a : Foo const )   // accepts Foo and Foo const

``==const`` specifies, that const of the expression has to match const of the argument:

.. code-block:: das

    def foo ( a : Foo const ==const )   // accepts Foo const only
    def foo ( var a : Foo ==const )     // accepts Foo only

``-const`` will remove const from the matching type:

.. code-block:: das

    def foo ( a : array<auto -const> )  // matches any array, with non-const elements

``#`` specifies that only temporary types are accepted:

.. code-block:: das

    def foo ( a : Foo# )    // accepts Foo# only

``-#`` will remove temporary type from the matching type:

.. code-block:: das

    def foo ( a : auto(TT) ) {      // accepts any type
        var temp : TT -# := a       // TT -# is now a regular type, and when `a` is temporary, it can clone it into `temp`
    }

``&`` specifies that argument is passed by reference:

.. code-block:: das

    def foo ( a : auto& )           // accepts any type, passed by reference

``==&`` specifies that reference of the expression has to match reference of the argument:

.. code-block:: das

    def foo ( a : auto& ==& )   // accepts any type, passed by reference (for example variable i, even if its integer)
    def foo ( a : auto ==& )    // accepts any type, passed by value     (for example value 3)

``-&`` will remove reference from the matching type:

.. code-block:: das

    def foo ( a : auto(TT)& ) {     // accepts any type, passed by reference
        var temp : TT -& = a        // TT -& is not a local reference
    }

``[]`` specifies that the argument is a static array of arbitrary dimension:

.. code-block:: das

    def foo ( a : auto[] )          // accepts static array of any type of any size

``-[]`` will remove static array dimension from the matching type:

.. code-block:: das

    def take_dim( a : auto(TT) ) {
        var temp : TT -[]           // temp is type of element of a
    }
    // if a is int[10] temp is int
    // if a is int[10][20][30] temp is still int

``implicit`` specifies that both temporary and regular types can be matched, but the type will be treated as specified. ``implicit`` is _UNSAFE_:

.. code-block:: das

    def foo ( a : Foo implicit )    // accepts Foo and Foo#, a will be treated as Foo
    def foo ( a : Foo# implicit )   // accepts Foo and Foo#, a will be treated as Foo#

``explicit`` specifies that LSP will not be applied, and only exact type match will be accepted:

.. code-block:: das

    def foo ( a : Foo )             // accepts Foo and any type that is inherited from Foo directly or indirectly
    def foo ( a : Foo explicit )    // accepts Foo only

options
-------

Multiple options can be specified as a function argument:

.. code-block:: das

    def foo ( a : int | float )   // accepts int or float

Optional types always make function generic.

Generic options will be matched in the order listed:

.. code-block:: das

    def foo ( a : Bar explicit | Foo )   // first will try to match exactly Bar, than anything else inherited from Foo

``|#`` shortcat matches previous type, with temporary flipped:

.. code-block:: das

    def foo ( a : Foo |# )   // accepts Foo and Foo# in that order
    def foo ( a : Foo# |# )  // accepts Foo# and Foo in that order

typedecl
--------

Consider the following example:

.. code-block:: das

    struct A {
        id : string
    }
    struct B {
        id : int
    }
    def get_table_from_id(t : auto(T)) {
        var tab : table<typedecl(t.id); T>  // NOTE typedecl
        return <- tab
    }

    [export]
    def main {
        var a : A
        var b : B
        var aTable <- get_table_from_id(a)
        var bTable <- get_table_from_id(b)
        print("{typeinfo typename(aTable)}\n")
        print("{typeinfo typename(bTable)}\n")
    }

Expected output:

.. code-block:: text

    table<string const;A const>
    table<int const;B const>

Here table is created with a key type of ``id`` field of the provided struct.
This feature allows to create types based on the provided expression type.

generic tuples and type<> expressions
-------------------------------------

Consider the following example:

.. code-block:: das

    tuple Handle {
        h : auto(HandleType)
        i : int
    }

    def make_handle ( t : auto(HandleType) ) : Handle {
        var h : type<Handle> // NOTE type<Handle>
        return h
    }

    def take_handle ( h : Handle ) {
        print("count = {h.i} of type {typeinfo typename(type<HandleType>)}\n")
    }

    [export]
    def main {
        let h = make_handle(10)
        take_handle(h)
    }

Expected output:

.. code-block:: text

    count = 0 of type int const

In the function make_handle, the type of the variable h is created with the type<> expression.
type<> is inferred in context (this time based on a function argument).
This feature allows to create types based on the provided expression type.

Generic function take_handle takes any Handle type, but only Handle type tuple.

This carries some similarity to the C++ template system, but is a bit more limited due to tuples being weak types.

.. _generic_module_prefixes:

Module prefixes in generics
===========================

Generic functions are always instanced as private functions in the *calling* module.
This means that unqualified function calls inside a generic resolve using the
defining module's visible symbols — **not** the caller's.

Three prefixes control how names are resolved inside a generic:

===========  ===================================================================
Prefix       Resolution
===========  ===================================================================
*(none)*     Resolved in the module where the generic is **defined** — the
             caller's overloads are **not** visible.
``_::``      Resolved as if the call were made implicitly in the **current
             module** (the one that instances the generic) — the caller's
             overloads **are** visible.
``__::``     Resolved strictly in the module where the generic is **defined**
             — only that module's own symbols, nothing imported.
===========  ===================================================================

This distinction matters whenever a library generic should dispatch to
user-provided overloads.  For example:

.. code-block:: das

    // --- module "serializer" ---
    module serializer

    [generic]
    def save(val) {
        _::write(val)       // resolves in the caller's module
    }

    // --- user code ---
    require serializer

    struct Color { r : float; g : float; b : float }

    def write(c : Color) {       // user overload
        print("{c.r},{c.g},{c.b}")
    }

    [export]
    def main() {
        save(Color(r=1.0))     // calls user's `write(Color)` via _::
    }

If ``save`` called plain ``write(val)`` instead of ``_::write(val)``, the
user's overload would not be found — the call would resolve in the
``serializer`` module's scope, where no ``write(Color)`` exists.

This is why the built-in ``:=`` and ``delete`` operators are always emitted as
``_::clone`` and ``_::finalize`` — so that user-defined ``clone`` and
``finalize`` overloads are picked up when generics are instanced in user code.

.. seealso::

    :ref:`Modules <modules>` for full details on ``_::`` and ``__::`` prefixes,
    :ref:`Functions <functions>` for regular (non-generic) function declarations,
    :ref:`Datatypes <datatypes_and_values>` for type traits and built-in types,
    :ref:`Temporary types <temporary>` for ``#``/``-#`` temporary qualifiers,
    :ref:`Aliases <aliases>` for ``auto(Name)`` aliases,
    :ref:`Unsafe <unsafe>` for the ``implicit`` keyword.

