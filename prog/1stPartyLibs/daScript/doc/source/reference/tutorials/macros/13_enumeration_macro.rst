.. _tutorial_macro_enumeration_macro:

.. index::
   single: Tutorial; Macros; Enumeration Macro
   single: Tutorial; Macros; enumeration_macro
   single: Tutorial; Macros; AstEnumerationAnnotation
   single: Tutorial; Macros; enum_total
   single: Tutorial; Macros; string_to_enum
   single: Tutorial; Macros; enum_trait

====================================================
 Macro Tutorial 13: Enumeration Macros
====================================================

Previous tutorials transformed calls, functions, structures, blocks,
variants, for-loops, lambda captures, reader macros, and typeinfo
expressions.  Enumeration macros let you intercept enum declarations
at compile time and either **modify the enum** or **generate new code**.

``[enumeration_macro(name="X")]`` registers a class that extends
``AstEnumerationAnnotation``.  This is the simplest macro type — it has
only one method:

``apply(var enu, var group, args, var errors) → bool``
   Called **before the infer pass**.  The macro receives the full
   ``EnumerationPtr`` and can add/remove entries, generate functions, or
   create global variables.  Return ``true`` on success, ``false`` with
   an error message to abort compilation.


Motivation
==========

Enumerations in daslang are simple value lists.  But real-world code
often needs derived information:

- **A "total" sentinel** — the number of values, for array sizing or
  range checking.
- **String constructors** — converting user input strings to enum values
  at runtime.

Both can be achieved with enumeration macros:

- Section 1 shows ``[enum_total]`` — a custom macro that *modifies* an
  enum by appending a ``total`` entry.
- Section 2 shows ``[string_to_enum]`` from ``daslib/enum_trait`` — a
  standard library macro that *generates code* (a lookup table and two
  constructor functions).


The module file — enum_total
============================

The macro module defines a single ``AstEnumerationAnnotation`` subclass
that adds a ``total`` entry to any enum.

Full source: :download:`enum_macro_mod.das <../../../../../tutorials/macros/enum_macro_mod.das>`

.. code-block:: das

    [enumeration_macro(name="enum_total")]
    class EnumTotalAnnotation : AstEnumerationAnnotation {
        def override apply(var enu : EnumerationPtr;
                           var group : ModuleGroup;
                           args : AnnotationArgumentList;
                           var errors : das_string) : bool {
            // Check that the enum doesn't already have a "total" entry.
            for (ee in enu.list) {
                if (ee.name == "total") {
                    errors := "enumeration already has a 'total' field"
                    return false
                }
            }
            // Add a new "total" entry at the end.
            let idx = add_enumeration_entry(enu, "total")
            if (idx < 0) {
                errors := "failed to add 'total' field"
                return false
            }
            // Set total = number of original entries.
            // length(enu.list) is now N+1, so total = N+1-1 = N.
            enu.list[idx].value |> move_new() <| new ExprConstInt(
                at = enu.at, value = length(enu.list) - 1)
            return true
        }
    }

Key points:

- ``enu.list`` is the array of ``EnumEntry`` nodes — iterate it to
  validate existing entries.
- ``add_enumeration_entry(enu, "total")`` appends a new entry and
  returns its index (or ``-1`` on failure).
- ``enu.list[idx].value`` is an ``ExpressionPtr`` — use ``move_new``
  to assign a new ``ExprConstInt`` with the desired integer value.
- ``enu.at`` provides the source location for the generated expression.
- Return ``false`` with an error message to abort compilation — the
  error will point to the enum declaration.


The usage file
==============

Full source: :download:`13_enumeration_macro.das <../../../../../tutorials/macros/13_enumeration_macro.das>`


Section 1 — enum_total (modifying the enum)
--------------------------------------------

.. code-block:: das

    require enum_macro_mod

    [enum_total]
    enum Direction {
        North
        South
        East
        West
    }

    def section1() {
        print("--- Section 1: enum_total ---\n")
        print("Direction.total = {int(Direction.total)}\n")
        for (d in each(Direction.North)) {
            if (d == Direction.total) {
                break
            }
            print("  {d}\n")
        }
    }

The ``[enum_total]`` annotation runs before type inference and appends
``total = 4`` to the enum.  At compile time the enum becomes:

.. code-block:: das

    enum Direction {
        North  = 0
        South  = 1
        East   = 2
        West   = 3
        total  = 4
    }

The ``each()`` function from ``daslib/enum_trait`` iterates all values
(including ``total``), so the loop breaks when it reaches the sentinel.


Section 2 — string_to_enum (generating code)
---------------------------------------------

.. code-block:: das

    require daslib/enum_trait

    [string_to_enum]
    enum Color {
        Red
        Green
        Blue
    }

    def section2() {
        print("--- Section 2: string_to_enum ---\n")
        let c1 = Color("Red")
        print("Color(\"Red\")   = {c1}\n")
        let c2 = Color("invalid", Color.Blue)
        print("Color(\"invalid\", Color.Blue) = {c2}\n")
    }

The ``[string_to_enum]`` annotation from ``daslib/enum_trait`` generates
three things at compile time:

1. A **private global table** ``_`enum`table`Color`` mapping strings to
   enum values — created with ``add_global_private_let`` and
   ``enum_to_table``.

2. A **single-argument constructor** ``Color(src : string) : Color`` —
   panics if the string is not a valid enum name.

3. A **two-argument constructor** ``Color(src : string; defaultValue : Color) : Color`` —
   returns ``defaultValue`` if the string is not found.

Both constructors are generated with ``qmacro_function``, registered
with ``add_function(compiling_module(), fn)``, and marked as
non-private with ``enumFn.flags &= ~FunctionFlags.privateFunction``.


How string_to_enum works internally
====================================

The ``EnumFromStringConstruction`` class in ``daslib/enum_trait.das``
demonstrates the **code generation** pattern for enumeration macros:

.. code-block:: das

    [enumeration_macro(name="string_to_enum")]
    class EnumFromStringConstruction : AstEnumerationAnnotation {
        def override apply(var enu : EnumerationPtr;
                           var group : ModuleGroup;
                           args : AnnotationArgumentList;
                           var errors : das_string) : bool {
            var inscope enumT <- new TypeDecl(
                baseType = Type.tEnumeration,
                enumType = enu.get_ptr())
            // 1. Create a private global lookup table.
            let varName = "_`enum`table`{enu.name}"
            add_global_private_let(
                compiling_module(), varName, enu.at,
                qmacro(enum_to_table(type<$t(enumT)>)))
            // 2. Generate the panic-on-miss constructor.
            var inscope enumFn <- qmacro_function("{enu.name}")
                $(src : string) : $t(enumT) {
                    if (!key_exists($i(varName), src)) {
                        panic("enum value '{src}' not found")
                    }
                    return $i(varName)?[src] ?? default<$t(enumT)>
                }
            enumFn.flags &= ~FunctionFlags.privateFunction
            force_at(enumFn, enu.at)
            force_generated(enumFn, true)
            compiling_module() |> add_function(enumFn)
            // 3. Generate the default-on-miss constructor.
            var inscope enumFnDefault <- qmacro_function("{enu.name}")
                $(src : string; defaultValue : $t(enumT)) : $t(enumT) {
                    return $i(varName)?[src] ?? defaultValue
                }
            enumFnDefault.flags &= ~FunctionFlags.privateFunction
            force_at(enumFnDefault, enu.at)
            force_generated(enumFnDefault, true)
            compiling_module() |> add_function(enumFnDefault)
            return true
        }
    }

Key code-generation techniques:

- **``qmacro_function("name") $(args) : ReturnType { body }``** —
  creates a new function AST node.  Reification splices (``$t()``,
  ``$i()``) inject types and identifiers from variables.
- **``add_global_private_let(module, name, at, expr)``** — adds a
  private global ``let`` variable initialized by ``expr``.
- **``compiling_module()``** — returns the module being compiled (where
  the annotated enum lives), so generated functions appear in the
  user's module.
- **``force_at(fn, at)``** — sets the source location of all nodes in
  the generated function to ``at``, so error messages point to the
  enum declaration.
- **``force_generated(fn, true)``** — marks the function as
  compiler-generated (suppresses "unused function" warnings).


Output
======

.. code-block:: text

    --- Section 1: enum_total ---
    Direction.total = 4
      North
      South
      East
      West
    --- Section 2: string_to_enum ---
    Color("Red")   = Red
    Color("invalid", Color.Blue) = Blue


Real-world usage
================

``daslib/enum_trait`` provides a rich set of enumeration utilities:

- ``[string_to_enum]`` — generates string constructors (shown above)
- ``each(enumValue)`` — iterates over all values of an enum type
- ``string(enumValue)`` — converts an enum value to its name
- ``to_enum(type<E>, "name")`` — runtime string-to-enum conversion
- ``enum_to_table(type<E>)`` — creates a ``table<string; E>`` lookup
- ``typeinfo enum_length(type<E>)`` — compile-time count of enum values
- ``typeinfo enum_names(type<E>)`` — compile-time array of value names

The two patterns shown in this tutorial cover the majority of
enumeration macro use cases:

- **Modify the enum** — add sentinel values, computed entries, or
  validation (like ``[enum_total]``).
- **Generate code** — create functions, tables, or variables derived
  from the enum's structure (like ``[string_to_enum]``).


.. seealso::

   Full source:
   :download:`13_enumeration_macro.das <../../../../../tutorials/macros/13_enumeration_macro.das>`,
   :download:`enum_macro_mod.das <../../../../../tutorials/macros/enum_macro_mod.das>`

   Previous tutorial: :ref:`tutorial_macro_typeinfo_macro`

   Next tutorial: :ref:`tutorial_macro_pass_macro`

   Standard library: ``daslib/enum_trait.das`` —
   :ref:`enum_trait module reference <stdlib_enum_trait>`

   Language reference: :ref:`Macros <macros>` — full macro system documentation
