.. _tutorial_macro_template_type_macro:

.. index::
   single: Tutorial; Macros; Template Type Macro
   single: Tutorial; Macros; typemacro_function
   single: Tutorial; Macros; Template Structure
   single: Tutorial; Macros; Structure Aliases

====================================================
 Macro Tutorial 16: Template Type Macro
====================================================

:ref:`Tutorial 15 <tutorial_macro_type_macro>` showed a type macro that
returns an array type.  This tutorial goes further — the ``pair`` macro
**generates a structure** with type-parameterized fields, turning
``pair(type<int>, type<float>)`` into a struct with ``first : int``
and ``second : float``.

The implementation uses the *manual* approach (without the
``[template_structure]`` shorthand) to expose the mechanics behind
template type resolution: structure aliases, generic inference, and
template class instantiation.


Key concepts
============

**Structure aliases** — every generated struct stores its type
parameters as aliases (``T1 → int``, ``T2 → float``).  On the generic
path the compiler reads them back to deduce ``auto(T1)``,
``auto(T2)``.

**``[typemacro_function]``** annotation (from ``daslib/typemacro_boost``)
— converts a regular function into an ``AstTypeMacro``.  It
auto-generates the class, registers it with ``add_new_type_macro``, and
extracts ``dimExpr`` arguments into the function parameters.  The first
two parameters are always ``macroArgument`` (the ``TypeDecl`` node) and
``passArgument`` (non-null in generic context); subsequent parameters
are the user arguments.

**``TypeMacroTemplateArgument``** — a helper struct that pairs a
template parameter name with its declared type (``argument_type``) and,
after inference, the resolved concrete type (``inferred_type``).


Template definition
===================

The template struct uses alias names (``T1``, ``T2``) for its fields.
The ``template`` keyword prevents direct instantiation — these aliases
only make sense after the type macro resolves them::

    struct template Pair {
        first  : T1
        second : T2
    }


Two paths — concrete and generic
=================================

The type macro function handles two compiler contexts:

**Concrete** (``passArgument == null``) — the user wrote fully-specified
types like ``pair(type<int>, type<float>)``::

    1. verify_arguments     — ensure no remaining auto types
    2. template_structure_name — build mangled name "Pair<int,float>"
    3. find_unique_structure   — deduplicate (skip if already exists)
    4. qmacro_template_class  — clone the template struct, rename
       it, clear the template flag, apply substitution rules
    5. make_typemacro_template_instance — annotate the new struct so
       it can be recognized as a Pair instance later
    6. add_structure_aliases — store T1=int, T2=float on the struct

**Generic** (``passArgument != null``) — a function parameter uses
``auto(…)`` patterns like ``pair(type<auto(T1)>, type<auto(T2)>)`` and
the compiler is matching a concrete argument against it::

    1. is_typemacro_template_instance — verify the argument is indeed
       a Pair instance
    2. infer_struct_aliases — read T1, T2 aliases from the concrete
       struct into template_arguments[i].inferred_type
    3. infer_template_types — for each argument, call
       infer_generic_type to match auto(T1) against the actual type,
       then update_alias_map so the compiler resolves the names


Section 1 — The macro module
=============================

.. literalinclude:: ../../../../../tutorials/macros/template_type_macro_mod.das
   :language: das
   :caption: tutorials/macros/template_type_macro_mod.das

The ``[typemacro_function]`` annotation on ``pair`` converts the
function into an ``AstTypeMacro`` registered under the name
``"pair"``.  The annotation also generates code that extracts
``dimExpr[1]`` and ``dimExpr[2]`` into the ``T1`` and ``T2``
``TypeDeclPtr`` parameters.

The argument names in ``TypeMacroTemplateArgument`` (``"T1"``,
``"T2"``) **must** match the alias names used in the template struct
fields.  This is how ``qmacro_template_class`` knows which alias to
substitute when cloning the structure.


Section 2 — Using the type macro
=================================

.. literalinclude:: ../../../../../tutorials/macros/16_template_type_macro.das
   :language: das
   :caption: tutorials/macros/16_template_type_macro.das

Three patterns are demonstrated:

1. **Concrete instantiation** — ``var p : pair(type<int>, type<float>)``
   triggers the concrete path.  The macro generates a struct
   ``Pair<int,float>`` with fields ``first : int`` and
   ``second : float``.

2. **Generic parameter** — ``get_first`` and ``get_second`` accept
   any ``Pair`` instantiation.  The compiler matches ``auto(T1)`` /
   ``auto(T2)`` against the stored aliases and deduces the element
   types.

3. **Generic return type** — ``swap_pair`` returns
   ``pair(type<T2>, type<T1>)`` — the swapped types.  Both generic
   parameter inference and concrete struct generation happen in the
   same call.

Running the tutorial::

    $ daslang tutorials/macros/16_template_type_macro.das
    concrete: first=42 second=3.14
    get_first:  10
    get_second: 2.5
    swapped: first=2.5 second=10


.. seealso::

   Full source:
   :download:`16_template_type_macro.das <../../../../../tutorials/macros/16_template_type_macro.das>`,
   :download:`template_type_macro_mod.das <../../../../../tutorials/macros/template_type_macro_mod.das>`

   Previous tutorial: :ref:`tutorial_macro_type_macro`

   Next tutorial: :ref:`tutorial_macro_qmacro`

   Standard library: ``daslib/typemacro_boost.das`` — infrastructure for
   template type macros (``TypeMacroTemplateArgument``,
   ``infer_template_types``, ``add_structure_aliases``, etc.)

   Language reference: :ref:`Macros <macros>` — full macro system documentation
