.. _tutorial_macro_type_macro:

.. index::
   single: Tutorial; Macros; Type Macro
   single: Tutorial; Macros; type_macro
   single: Tutorial; Macros; AstTypeMacro

====================================================
 Macro Tutorial 15: Type Macro
====================================================

Previous tutorials showed macros attached to functions, structures,
loops, enums, typeinfo expressions, and entire compilation passes.
**Type macros** operate in a different domain: they let you define
custom type expressions that the compiler resolves during type
inference.

``AstTypeMacro`` is the base class.  It has a single method:

``visit(prog : ProgramPtr; mod : Module?; td : TypeDeclPtr; passT : TypeDeclPtr) → TypeDeclPtr``
   ``prog`` is the program being compiled.
   ``mod`` is the module that registered the macro.
   ``td`` is the ``TypeDecl`` node representing the macro invocation —
   its ``dimExpr`` array carries the arguments.
   ``passT`` is non-null only in a generic context (see below).
   Return the resolved ``TypeDeclPtr``, or an empty pointer on error.

The annotation ``[type_macro(name="…")]`` registers a class as a type
macro.  The name string determines the identifier used in type
position::

    [type_macro(name="padded")]
    class PaddedTypeMacro : AstTypeMacro { ... }

After registration, ``padded(type<float>, 4)`` is a valid type
expression.


How the parser stores arguments
===============================

When the compiler sees a type-macro invocation like
``padded(type<float>, 4)`` it creates a ``TypeDecl`` with
``baseType = Type.typeMacro`` and populates the ``dimExpr`` array:

.. list-table::
   :header-rows: 1
   :widths: 20 35 45

   * - Index
     - AST node
     - Contains
   * - ``dimExpr[0]``
     - ``ExprConstString``
     - The macro name (``"padded"``)
   * - ``dimExpr[1]``
     - ``ExprTypeDecl``
     - The first argument — the type (wraps ``type<float>``)
   * - ``dimExpr[2]``
     - ``ExprConstInt``
     - The second argument — the size (``4``)

Type arguments are wrapped in ``ExprTypeDecl``; their inferred result is
available via ``dimExpr[i]._type``.  Value arguments keep their
expression type — ``ExprConstInt`` for integer literals,
``ExprConstString`` for strings, and so on.


Two inference contexts
======================

The compiler calls ``visit()`` in two different contexts:

**Concrete** — all type arguments are already inferred.
   Example: ``var data : padded(type<float>, 4)``.
   Here ``td.dimExpr[1]._type`` is the fully-resolved ``float`` type,
   and ``passT`` is null.
   The macro clones ``_type``, adds a dimension, and returns
   ``float[4]``.

**Generic** — the type includes unresolved type parameters.
   Example: ``def foo(arr : padded(type<auto(TT)>, 4))``.
   Here ``td.dimExpr[1]._type`` is null because the type has not
   been inferred yet.  ``passT`` is non-null and carries the actual
   argument type being matched.
   The macro must return a type the compiler can use for generic
   matching — typically by cloning the ``ExprTypeDecl.typeexpr`` (which
   preserves ``auto(TT)``) and adding the dimension.

.. important::

   Always check ``td.dimExpr[i]._type == null`` to distinguish the
   generic path from the concrete path.  In the generic path, fall
   back to ``.typeexpr`` or create an ``autoinfer`` type.


Section 1 — The macro module
=============================

The type macro lives in its own module so that it is available via
``require`` — just like macros in previous tutorials.

.. literalinclude:: ../../../../../tutorials/macros/type_macro_mod.das
   :language: das
   :caption: tutorials/macros/type_macro_mod.das

The ``visit`` method first validates that exactly two user arguments
were provided (``dimExpr`` length 3 — the name plus two arguments) and
that the size argument is a constant integer.

For the **generic path** (``_type == null``): if the first argument is an
``ExprTypeDecl`` (e.g. wrapping ``auto(TT)``), clone its ``.typeexpr``;
otherwise create a pure ``autoinfer`` type.  Add the requested dimension
and return.

For the **concrete path**: clone ``_type`` (the already-inferred type),
add the dimension, and return the final type — e.g. ``float[4]``.


Section 2 — Using the type macro
=================================

.. literalinclude:: ../../../../../tutorials/macros/15_type_macro.das
   :language: das
   :caption: tutorials/macros/15_type_macro.das

``test_concrete`` declares ``var data : padded(type<float>, 4)``.
The compiler invokes the macro in concrete mode — ``_type`` is ``float``,
so the macro returns ``float[4]``.  ``data`` is a fixed-size array of
4 floats.

``sum_padded`` declares its parameter as
``padded(type<auto(TT)>, 4)``.  The compiler invokes the macro in
generic mode.  When called with ``floats`` (a ``float[4]``), ``TT`` is
deduced as ``float``; when called with ``ints`` (an ``int[4]``), ``TT``
is deduced as ``int``.

Running the tutorial::

    $ daslang tutorials/macros/15_type_macro.das
    concrete: data = [[ 1; 2; 3; 4]]
    generic: float sum = 100
    generic: int sum = 10


.. seealso::

   Full source:
   :download:`15_type_macro.das <../../../../../tutorials/macros/15_type_macro.das>`,
   :download:`type_macro_mod.das <../../../../../tutorials/macros/type_macro_mod.das>`

   Previous tutorial: :ref:`tutorial_macro_pass_macro`

   Next tutorial: :ref:`tutorial_macro_template_type_macro`

   Language reference: :ref:`Macros <macros>` — full macro system documentation
