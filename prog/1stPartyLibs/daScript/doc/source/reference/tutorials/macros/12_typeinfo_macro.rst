.. _tutorial_macro_typeinfo_macro:

.. index::
   single: Tutorial; Macros; Typeinfo Macro
   single: Tutorial; Macros; typeinfo_macro
   single: Tutorial; Macros; AstTypeInfoMacro
   single: Tutorial; Macros; getAstChange

====================================================
 Macro Tutorial 12: Typeinfo Macros
====================================================

Previous tutorials transformed calls, functions, structures, blocks,
variants, for-loops, lambda captures, and reader macros.  Typeinfo macros
extend the built-in ``typeinfo`` expression with **custom compile-time
type introspection**.

``[typeinfo_macro(name="X")]`` registers a class that extends
``AstTypeInfoMacro``.  The macro is invoked with the syntax
``typeinfo X(expr)`` (gen2) or ``typeinfo X<subtrait>(expr)``.  During
type inference the compiler calls:

``getAstChange(expr, errors) → ExpressionPtr``
   Receives an ``ExprTypeInfo`` node and returns a replacement AST
   expression — typically a constant (string, int, bool) or an array
   literal.  The returned expression replaces the entire ``typeinfo``
   call at compile time.  Return ``null`` with an error message to
   report a compilation error.


ExprTypeInfo fields
-------------------

The ``expr`` parameter exposes several fields:

===============  ====================  =======================================
Field            Type                  Description
===============  ====================  =======================================
``typeexpr``     ``TypeDeclPtr``       The type argument (from ``type<T>``)
``subexpr``      ``ExpressionPtr``     The expression argument (if used)
``subtrait``     ``string``            The ``<name>`` in ``typeinfo X<name>``
``extratrait``   ``string``            The second ``<a;b>`` parameter
``trait``        ``string``            The name of the typeinfo trait
``at``           ``LineInfo``          Source location for error reporting
===============  ====================  =======================================

``typeexpr`` gives access to the full type declaration.  On structure
types, ``typeexpr.structType.fields`` is a list of ``FieldDeclaration``
nodes (each with ``name``, ``_type``, ``flags``).  On enum types,
``typeexpr.enumType.list`` contains ``EnumEntry`` nodes (each with
``name``).


Motivation
==========

The ``typeinfo`` keyword already provides many built-in traits:
``sizeof``, ``typename``, ``is_ref``, ``has_field``, ``is_enum``,
``is_struct``, and dozens more.  But sometimes you need *domain-specific*
compile-time introspection — a description string for serialization, an
array of enum names for UI dropdowns, or a boolean check for conditional
compilation.  ``[typeinfo_macro]`` lets you add such traits using pure
daslang code.


The module file
===============

The macro module defines three ``AstTypeInfoMacro`` subclasses, each
returning a different expression type (string, array, bool).

Full source: :download:`typeinfo_macro_mod.das <../../../../../tutorials/macros/typeinfo_macro_mod.das>`


struct_info — returning a string
--------------------------------

``typeinfo struct_info(type<T>)`` builds a description string at compile
time:

.. code-block:: das

    [typeinfo_macro(name="struct_info")]
    class TypeInfoGetStructInfo : AstTypeInfoMacro {
        def override getAstChange(expr : smart_ptr<ExprTypeInfo>;
                                  var errors : das_string) : ExpressionPtr {
            if (expr.typeexpr == null) {
                errors := "type is missing or not inferred"
                return <- default<ExpressionPtr>
            }
            if (!expr.typeexpr.isStructure) {
                errors := "expecting structure type"
                return <- default<ExpressionPtr>
            }
            var result = build_string() <| $(var w) {
                w |> write("{expr.typeexpr.structType.name}(")
                var first = true
                for (i in iter_range(expr.typeexpr.structType.fields)) {
                    assume fld = expr.typeexpr.structType.fields[i]
                    if (fld.flags.classMethod) {
                        continue
                    }
                    if (!first) {
                        w |> write(", ")
                    }
                    w |> write("{fld.name}:{describe(fld._type, false, false, false)}")
                    first = false
                }
                w |> write(")")
            }
            return <- new ExprConstString(at = expr.at, value := result)
        }
    }

Key points:

- ``expr.typeexpr.isStructure`` validates that the type is a structure.
- ``expr.typeexpr.structType.fields`` iterates all field declarations.
- ``fld.flags.classMethod`` skips class methods (only data fields appear).
- ``describe(fld._type, ...)`` converts a ``TypeDeclPtr`` to a human-readable
  type name.
- The result is an ``ExprConstString`` — a compile-time string constant.


enum_value_strings — returning an array
----------------------------------------

``typeinfo enum_value_strings(type<E>)`` returns a fixed-size array of
enum value names:

.. code-block:: das

    [typeinfo_macro(name="enum_value_strings")]
    class TypeInfoGetEnumValueStrings : AstTypeInfoMacro {
        def override getAstChange(expr : smart_ptr<ExprTypeInfo>;
                                  var errors : das_string) : ExpressionPtr {
            // ... validation ...
            var inscope arr <- new ExprMakeArray(
                at = expr.at,
                makeType <- typeinfo ast_typedecl(type<string>))
            for (i in iter_range(expr.typeexpr.enumType.list)) {
                if (true) {
                    assume entry = expr.typeexpr.enumType.list[i]
                    var inscope nameExpr <- new ExprConstString(
                        at = expr.at, value := entry.name)
                    arr.values |> emplace <| nameExpr
                }
            }
            return <- arr
        }
    }

Key points:

- ``ExprMakeArray`` requires a ``makeType`` field — use ``typeinfo
  ast_typedecl(type<string>)`` to get the AST representation of
  ``string``.
- ``expr.typeexpr.enumType.list`` iterates all ``EnumEntry`` nodes.
- The ``if (true)`` block is needed for ``var inscope`` lifetime scoping
  (each loop iteration creates and consumes a new ``inscope`` smart pointer).
- The result is a **fixed-size array** (``string[N]``), not a dynamic
  ``array<string>``.


has_non_static_method — returning a bool with subtrait
------------------------------------------------------

``typeinfo has_non_static_method<name>(type<T>)`` checks whether a class
has a non-static method with the given name.  The method name is passed
via the ``subtrait`` parameter:

.. code-block:: das

    [typeinfo_macro(name="has_non_static_method")]
    class TypeInfoHasNonStaticMethod : AstTypeInfoMacro {
        def override getAstChange(expr : smart_ptr<ExprTypeInfo>;
                                  var errors : das_string) : ExpressionPtr {
            // ... validation ...
            if (empty(expr.subtrait)) {
                errors := "expecting method name as subtrait"
                return <- default<ExpressionPtr>
            }
            var found = false
            for (i in iter_range(expr.typeexpr.structType.fields)) {
                assume fld = expr.typeexpr.structType.fields[i]
                if (fld.name == expr.subtrait && fld.flags.classMethod) {
                    found = true
                    break
                }
            }
            return <- new ExprConstBool(at = expr.at, value = found)
        }
    }

Key points:

- ``expr.subtrait`` is the string between angle brackets — in
  ``typeinfo has_non_static_method<speak>(type<T>)``, ``subtrait`` is
  ``"speak"``.
- Class methods are stored as struct fields with the ``classMethod`` flag.
- The result is ``ExprConstBool`` — a compile-time boolean, perfect for
  use in ``static_if`` conditional compilation.


The usage file
==============

Full source: :download:`12_typeinfo_macro.das <../../../../../tutorials/macros/12_typeinfo_macro.das>`


Section 1: struct_info
----------------------

.. code-block:: das

    struct Vec3 {
        x : float
        y : float
        z : float
    }

    struct Person {
        name : string
        age : int
    }

    def section1() {
        let v = typeinfo struct_info(type<Vec3>)
        print("  Vec3: {v}\n")
        let p = typeinfo struct_info(type<Person>)
        print("  Person: {p}\n")
    }

``typeinfo struct_info(type<Vec3>)`` is replaced at compile time with
the constant string ``"Vec3(x:float, y:float, z:float)"``.  No runtime
reflection is involved.


Section 2: enum_value_strings
-----------------------------

.. code-block:: das

    enum Color {
        Red
        Green
        Blue
    }

    def section2() {
        let colors = typeinfo enum_value_strings(type<Color>)
        for (c in colors) {
            print("    {c}\n")
        }
    }

The macro generates a fixed-size ``string[3]`` array containing
``"Red"``, ``"Green"``, ``"Blue"``.  Use ``let`` (not ``var <-``) to
bind the result — fixed-size arrays are value types.


Section 3: has_non_static_method
--------------------------------

.. code-block:: das

    class Animal {
        name : string
        def speak() {
            print("    {name} says hello\n")
        }
    }

    class Rock {
        weight : float
    }

    def section3() {
        let animal_can_speak = typeinfo has_non_static_method<speak>(type<Animal>)
        let rock_can_speak = typeinfo has_non_static_method<speak>(type<Rock>)
        static_if (typeinfo has_non_static_method<speak>(type<Animal>)) {
            print("  (static_if confirmed: Animal can speak)\n")
        }
    }

The subtrait makes typeinfo macros parametric — the same macro handles
any method name.  Since the result is a compile-time ``bool``, it works
directly in ``static_if`` for conditional code generation.


Output
======

.. code-block:: text

    --- Section 1: struct_info ---
      Vec3: Vec3(x:float, y:float, z:float)
      Person: Person(name:string, age:int)
    --- Section 2: enum_value_strings ---
      Color values (3):
        Red
        Green
        Blue
      Direction values (4):
        North
        South
        East
        West
    --- Section 3: has_non_static_method ---
      Animal has 'speak': true
      Animal has 'fly': false
      Rock has 'speak': false
      (static_if confirmed: Animal can speak)


Real-world usage
================

The standard library provides several typeinfo macros:

- ``typeinfo fields_count(type<T>)`` — number of struct fields
  (``daslib/type_traits.das``)
- ``typeinfo safe_has_property<name>(expr)`` — does a type have a
  property function? (``daslib/type_traits.das``)
- ``typeinfo enum_length(type<E>)`` — number of enum values
  (``daslib/enum_trait.das``)
- ``typeinfo enum_names(type<E>)`` — array of enum value name strings
  (``daslib/enum_trait.das``)


.. seealso::

   Full source:
   :download:`12_typeinfo_macro.das <../../../../../tutorials/macros/12_typeinfo_macro.das>`,
   :download:`typeinfo_macro_mod.das <../../../../../tutorials/macros/typeinfo_macro_mod.das>`

   Previous tutorial: :ref:`tutorial_macro_reader_macro`

   Next tutorial: :ref:`tutorial_macro_enumeration_macro`

   Standard library: ``daslib/type_traits.das``, ``daslib/enum_trait.das``

   Language reference: :ref:`Macros <macros>` — full macro system documentation
