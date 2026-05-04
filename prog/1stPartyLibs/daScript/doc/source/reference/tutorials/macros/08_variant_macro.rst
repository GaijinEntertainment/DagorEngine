.. _tutorial_macro_variant_macro:

.. index::
   single: Tutorial; Macros; Variant Macro
   single: Tutorial; Macros; variant_macro
   single: Tutorial; Macros; AstVariantMacro
   single: Tutorial; Macros; is / as / ?as
   single: Tutorial; Macros; InterfaceAsIs
   single: Tutorial; Macros; interfaces

====================================================
 Macro Tutorial 8: Variant Macros
====================================================

Tutorials 3--7 transformed *functions*, *structs*, and *blocks*.  Variant
macros are different: they intercept the **``is``**, **``as``**, and
**``?as``** operators during type inference and replace them with
arbitrary expressions.

``[variant_macro(name="X")]`` registers a class that extends
``AstVariantMacro``.  When the compiler encounters ``expr is Name``,
``expr as Name``, or ``expr ?as Name``, it calls every registered
variant macro's visitor method in order.  The first one to return a
non-default ``ExpressionPtr`` wins --- the operator is replaced with
the returned expression.


Three-tier resolution
=====================

The ``is`` / ``as`` / ``?as`` operators go through three resolution
stages:

.. code-block:: text

   expr is/as/?as Name
     ↓
   1. Variant macros  (visitExpr*Variant returns non-default)
     ↓
   2. Generic operator functions  (def operator is/as Name)
     ↓
   3. Built-in variant handling  (native variant types)

If a variant macro claims the expression, stages 2 and 3 are never
reached.  If all macros return ``default<ExpressionPtr>``, the
compiler tries generic ``operator is``/``operator as`` overloads, and
finally falls through to built-in ``variant`` type dispatching.


``AstVariantMacro`` methods
===========================

+--------------------------------------+-----------------------------------------+
| Method                               | Intercepts                              |
+======================================+=========================================+
| ``visitExprIsVariant(prog, mod, e)`` | ``expr is Name`` — type check           |
+--------------------------------------+-----------------------------------------+
| ``visitExprAsVariant(prog, mod, e)`` | ``expr as Name`` — type cast            |
+--------------------------------------+-----------------------------------------+
| ``visitExprSafeAsVariant(prog, mod,  | ``expr ?as Name`` — null-safe cast      |
| e)``                                 |                                         |
+--------------------------------------+-----------------------------------------+

Each method receives the full ``ProgramPtr`` and ``Module?`` context,
plus the expression node.  Return ``default<ExpressionPtr>`` to
*decline* and let the next macro (or built-in logic) handle it.
Return any other expression to *claim* the operator --- the compiler
replaces the original node with your expression.


Case study: ``InterfaceAsIs``
=============================

This tutorial uses the ``InterfaceAsIs`` variant macro from
``daslib/interfaces``.  It makes the ``is``, ``as``, and ``?as``
operators work with ``[interface]`` types --- no separate module file
is needed because the macro lives in the standard library.

``daslib/interfaces`` already provides two structure macros:

- ``[interface]`` — marks a class as an interface (function-only fields)
- ``[implements(IFoo)]`` — generates a proxy class and a
  ``get`IFoo`` getter method on the implementing struct

The ``InterfaceAsIs`` variant macro builds on top of these generated
getters.


Type guard pattern
------------------

Every visitor method starts with a *type guard* — a series of checks
that decide whether this macro should handle the expression:

.. code-block:: das

   def override visitExprIsVariant(prog : ProgramPtr; mod : Module?;
                                   expr : smart_ptr<ExprIsVariant>) : ExpressionPtr {
       assume vtype = expr.value._type
       // 1. Value must be a pointer to a structure
       if (!(vtype.isPointer && vtype.firstType != null && vtype.firstType.isStructure)) {
           return <- default<ExpressionPtr>
       }
       // 2. Target name must refer to an [interface]-annotated struct
       let iname = string(expr.name)
       var tgt = prog |> find_unique_structure(iname)
       if (tgt == null || !is_interface_struct(tgt)) {
           return <- default<ExpressionPtr>
       }
       ...
   }

If any check fails, the method returns ``default<ExpressionPtr>`` ---
declining to handle this expression and letting the next resolution
stage take over.


``is`` — compile-time check
----------------------------

Once the guard passes, ``visitExprIsVariant`` looks for a
``get`IFoo`` field on the source struct.  If found, the struct
implements the interface → return ``true``.  Otherwise → ``false``:

.. code-block:: das

   let getter_field = "get`{iname}"
   var st = vtype.firstType.structType
   for (fld in st.fields) {
       if (string(fld.name) == getter_field) {
           return <- qmacro(true)
       }
   }
   return <- qmacro(false)

The result is a **compile-time constant** — no runtime cost at all.
``w is IDrawable`` becomes the literal ``true`` in the final program.


``as`` — get interface proxy
-----------------------------

``visitExprAsVariant`` generates a call to the getter function:

.. code-block:: das

   let func_name = "{st.name}`get`{iname}"
   return <- qmacro($c(func_name)(*$e(expr.value)))

``$c(func_name)`` creates a function call by name
(e.g. ``Widget`get`IDrawable``).  ``*$e(expr.value)`` dereferences
the pointer to pass the struct by reference.

So ``w as IDrawable`` becomes ``Widget`get`IDrawable(*w)``, which
returns an ``IDrawable?`` proxy.


``?as`` — null-safe access
---------------------------

``visitExprSafeAsVariant`` adds a null check before calling the
getter:

.. code-block:: das

   var inscope val <- clone_expression(expr.value)
   return <- qmacro($e(val) != null ? $c(func_name)(*$e(expr.value)) : null)

``clone_expression`` is needed because the value expression appears
twice — once in the null check and once in the getter call.  The
original ``expr.value`` is *moved* into the ``$e()`` splice, so a
clone provides the second copy.

``w ?as IDrawable`` becomes::

   w != null ? Widget`get`IDrawable(*w) : null


The usage file
==============

.. code-block:: das

   options gen2
   require daslib/interfaces

   [interface]
   class IDrawable {
       def abstract draw(x, y : int) : void
   }

   [interface]
   class IResizable {
       def abstract resize(w, h : int) : void
   }

   [implements(IDrawable), implements(IResizable)]
   class Widget {
       def Widget() { pass }
       def IDrawable`draw(x, y : int) {
           print("Widget.draw at ({x},{y})\n")
       }
       def IResizable`resize(w, h : int) {
           print("Widget.resize to {w}x{h}\n")
       }
   }

   [implements(IDrawable)]
   class Label {
       text : string
       def Label(t : string) { text = t }
       def IDrawable`draw(x, y : int) {
           print("Label \"{text}\" at ({x},{y})\n")
       }
   }

``Widget`` implements both ``IDrawable`` and ``IResizable``.
``Label`` implements only ``IDrawable``.  The ``InterfaceAsIs`` macro
handles all three operators automatically:

.. code-block:: das

   [export]
   def main() {
       var w = new Widget()
       var l = new Label("hello")

       // is — compile-time check
       print("w is IDrawable  = {w is IDrawable}\n")   // true
       print("l is IResizable = {l is IResizable}\n")   // false

       // as — get interface proxy
       var drawable = w as IDrawable
       drawable->draw(10, 20)

       // ?as — null-safe access
       var maybe_draw = l ?as IDrawable
       if (maybe_draw != null) {
           maybe_draw->draw(5, 5)
       }

       // null pointer — ?as returns null
       var nothing : Label?
       var safe = nothing ?as IDrawable
       print("null ?as IDrawable = {safe}\n")

       unsafe { delete w; delete l }
   }

Runtime output::

   w is IDrawable  = true
   w is IResizable = true
   l is IDrawable  = true
   l is IResizable = false
   Widget.draw at (10,20)
   Widget.resize to 800x600
   Label "hello" at (5,5)
   null ?as IDrawable = null


Compilation pipeline
====================

The full sequence for ``w is IDrawable``:

.. code-block:: text

   parse expression (ExprIsVariant with value=w, name="IDrawable")
     ↓
   infer types → value type is Widget?
     ↓
   call visitExprIsVariant() on each registered variant macro
     ↓
   InterfaceAsIs checks: Widget? → pointer to struct ✓
   "IDrawable" is an [interface] ✓, Widget has get`IDrawable field ✓
     ↓
   return qmacro(true) — replaces ExprIsVariant with ExprConstBool
     ↓
   simulate → constant folded, zero runtime cost


Existing variant macros
=======================

The standard library ships several variant macros.  Studying them is
the best way to learn the pattern:

.. list-table::
   :widths: 35 65
   :header-rows: 1

   * - Macro
     - Purpose
   * - ``BetterRttiVisitor`` (``daslib/ast_boost``)
     - Optimizes ``is`` on AST expression types to ``__rtti`` string comparison
   * - ``ClassAsIs`` (``daslib/dynamic_cast_rtti``)
     - Dynamic casting for classes: ``is`` checks RTTI, ``as``/``?as`` cast pointers
   * - ``BetterJsonMacro`` (``daslib/json_boost``)
     - ``is``/``as`` on ``JsValue`` to access JSON node types
   * - ``InterfaceAsIs`` (``daslib/interfaces``)
     - ``is``/``as``/``?as`` for ``[interface]`` types (this tutorial)


Key takeaways
=============

.. list-table::
   :widths: 40 60
   :header-rows: 1

   * - Concept
     - What it does
   * - ``[variant_macro(name="X")]``
     - Registers a class as a variant macro
   * - ``AstVariantMacro``
     - Base class with ``visitExprIsVariant``, ``visitExprAsVariant``, ``visitExprSafeAsVariant``
   * - ``visitExprIsVariant``
     - Intercepts ``expr is Name`` — return replacement or default to decline
   * - ``visitExprAsVariant``
     - Intercepts ``expr as Name``
   * - ``visitExprSafeAsVariant``
     - Intercepts ``expr ?as Name``
   * - ``default<ExpressionPtr>``
     - Return value meaning "I don't handle this" — pass to next resolver
   * - Type guard pattern
     - Check ``expr.value._type`` before claiming an expression
   * - ``find_unique_structure``
     - Look up a struct by name from the compiling program
   * - ``qmacro(true)``
     - Generate a compile-time constant — zero runtime cost
   * - ``$c(name)(args)``
     - Generate a function call by name in ``qmacro``
   * - ``clone_expression``
     - Deep-copy an expression for safe double use in ``qmacro``


.. seealso::

   Full source:
   :download:`08_variant_macro.das <../../../../../tutorials/macros/08_variant_macro.das>`

   Previous tutorial: :ref:`tutorial_macro_block_macro`

   Next tutorial: :ref:`tutorial_macro_for_loop_macro`

   Standard library:
   ``daslib/interfaces.das`` (``InterfaceAsIs`` macro),
   ``daslib/ast_boost.das`` (``BetterRttiVisitor``),
   ``daslib/dynamic_cast_rtti.das`` (``ClassAsIs``),
   ``daslib/json_boost.das`` (``BetterJsonMacro``)

   Language reference: :ref:`Macros <macros>` — full macro system documentation
