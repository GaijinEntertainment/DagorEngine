.. _tutorial_macro_qmacro:

.. index::
   single: Tutorial; Macros; Quasi-quotation
   single: Tutorial; Macros; qmacro
   single: Tutorial; Macros; Reification
   single: Tutorial; Macros; templates_boost

====================================================
 Macro Tutorial 17: Quasi-quotation Reference
====================================================

:ref:`Tutorial 16 <tutorial_macro_template_type_macro>` used
``qmacro_template_class`` to generate a struct from a template.  This
tutorial is a **comprehensive reference** — it exercises every
quasi-quotation variant and reification operator provided by
``daslib/templates_boost`` in a single macro module.


Quasi-quotation variants
========================

All functions below are ``[call_macro]`` helpers (or the underlying
runtime functions) from ``templates_boost``.  They build AST nodes at
macro expansion time:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Variant
     - Description
   * - ``quote(expr)``
     - Plain AST capture, **no** reification.  Use for literal constants.
   * - ``qmacro(expr)``
     - Expression with reification splices (``$v``, ``$e``, etc.).
   * - ``qmacro_type(type<T>)``
     - Build a ``TypeDeclPtr``.  ``$t(td)`` splices another ``TypeDeclPtr`` inside.
   * - ``qmacro_variable("name", type<T>)``
     - Build a ``VariablePtr``.  Used for function parameters.
   * - ``qmacro_expr(${ stmt; })``
     - Single statement (e.g. ``let`` / ``var`` declaration).
   * - ``qmacro_block() { stmts }``
     - Multiple statements as an ``ExprBlock``.  ``$b(stmts)`` splices an array.
   * - ``qmacro_block_to_array() { stmts }``
     - Like ``qmacro_block`` but returns ``array<ExpressionPtr>``.
   * - ``qmacro_function("name") $(args) { body }``
     - Complete function.  ``$a(args)`` splices the parameter list.
   * - ``qmacro_template_class("Name", type<T>)``
     - Clone a ``struct template``, rename it, apply substitution rules.
       Also clones and renames any methods defined on the template.
   * - ``qmacro_method("Cls\`name", cls) $(self…) { }``
     - Class method.  Used here to add ``get_tuple`` to the cloned struct.
   * - ``qmacro_template_function(@@fn)``
     - Clone a ``def template`` function.  Types fixed up manually.


Reification operators
=====================

Inside ``qmacro(…)`` and friends, dollar-prefixed splices inject
compile-time values into the generated AST:

.. list-table::
   :header-rows: 1
   :widths: 15 45 40

   * - Operator
     - Meaning
     - Example
   * - ``$v(val)``
     - Compile-time **value** → AST constant
     - ``$v("hello")`` → ``ExprConstString``
   * - ``$e(expr)``
     - Splice an existing ``ExpressionPtr``
     - ``$e(existing_ast)`` → inserts that node
   * - ``$i(name)``
     - String → **identifier** reference
     - ``$i("tag")`` → ``ExprVar { name="tag" }``
   * - ``$c(name)``
     - String → function **call** name
     - ``$c("print")`` → ``ExprCall { name="print" }``
   * - ``$f(name)``
     - String → **field** access name
     - ``pair.$f("first")`` → ``pair.first``
   * - ``$t(td)``
     - Splice a ``TypeDeclPtr`` in type position
     - ``type<array<$t(int_type)>>`` → ``type<array<int>>``
   * - ``$a(args)``
     - Splice ``array<VariablePtr>`` as parameters
     - ``$($a(fn_args))`` in ``qmacro_function``
   * - ``$b(body)``
     - Splice ``array<ExpressionPtr>`` as block body
     - ``{ $b(fn_body) }`` in ``qmacro_function``


Section 1 — The macro module
=============================

The entire macro is a single ``[call_macro]`` named ``build_demo``.
Its ``visit`` method exercises **every** variant listed above,
including ``qmacro_method`` — used to add a ``get_tuple`` method
to the cloned ``DemoIntFloat`` struct.

.. literalinclude:: ../../../../../tutorials/macros/qmacro_mod.das
   :language: das
   :caption: tutorials/macros/qmacro_mod.das

Key observations:

- **``quote`` vs ``qmacro``** — use ``quote`` when no ``$…`` splices are
  needed; use ``qmacro`` when you need reification.

- **``qmacro_function`` + ``$a`` + ``$b``** — builds a complete function
  from dynamically constructed argument and body arrays.

- **``qmacro_template_class`` + ``add_structure_alias``** — the call
  macro clones the ``struct template``, renames it, and returns a
  ``TypeDeclPtr``.  ``add_structure_alias`` then registers alias types
  (``TFirst`` → ``int``, ``TSecond`` → ``float``) on the cloned struct
  so the compiler resolves them during compilation.  The template's
  ``describe()`` method is also cloned and renamed to
  ``DemoIntFloat`describe`` with the same alias substitution applied.

- **``qmacro_method``** — generates a standalone method (``get_tuple``)
  and attaches it to the cloned ``DemoIntFloat`` struct.  Uses
  ``add_ptr_ref`` to obtain the ``StructurePtr`` and ``$t(pair_type)``
  to splice the struct type into the ``self`` parameter.

- **``qmacro_template_function``** — clones a ``def template`` function.
  The template uses ``$t(t_value)`` tag types in its signature, so
  ``qmacro_template_function`` auto-generates substitution rules from
  the tags — no manual type fixup is needed after cloning.


Section 2 — Using the generated artifacts
==========================================

.. literalinclude:: ../../../../../tutorials/macros/17_qmacro.das
   :language: das
   :caption: tutorials/macros/17_qmacro.das

``build_demo()`` triggers the call macro.  The generated artefacts:

1. **``demo_run(tag)``** — a function whose body was assembled from
   ``qmacro``, ``qmacro_expr``, ``qmacro_block``,
   ``qmacro_block_to_array``, and all eight reification operators.

2. **``DemoIntFloat``** — a struct cloned from the ``DemoPair``
   template via ``qmacro_template_class``.  Type aliases ``TFirst``
   and ``TSecond`` are registered on the cloned struct with
   ``add_structure_alias``, so the compiler resolves them to ``int``
   and ``float``.  The template's ``describe()`` method is also
   cloned and renamed to ``DemoIntFloat`describe``.  Additionally,
   ``qmacro_method`` adds a ``get_tuple()`` method that returns
   ``(first, second)`` as a tuple.

3. **``demo_add_int``** — a function cloned from the ``demo_add``
   template.  The template's ``$t(t_value)`` tag types are resolved
   to ``int`` automatically by ``qmacro_template_function``.

Running the tutorial::

    $ daslang tutorials/macros/17_qmacro.das
    --- demo_run ---
    hello from qmacro!
    the literal is: true
    called via $c
    $i resolved tag = test
    pair.$f first = 42
    qmacro_block body:
      step 1
      step 2
      step 3
      extra A
      extra B
    title = items

    --- DemoIntFloat (qmacro_template_class) ---
    first  = 42
    second = 3.14
    get_tuple() = (42, 3.14)

    --- demo_add_int (qmacro_template_function) ---
    demo_add_int(10, 20) = 30


.. seealso::

   Full source:
   :download:`17_qmacro.das <../../../../../tutorials/macros/17_qmacro.das>`,
   :download:`qmacro_mod.das <../../../../../tutorials/macros/qmacro_mod.das>`

   Previous tutorial: :ref:`tutorial_macro_template_type_macro`

   Standard library: ``daslib/templates_boost.das`` — quasi-quotation
   infrastructure (all ``qmacro_*`` variants, ``Template``, ``apply_template``)

   Language reference: :ref:`Macros <macros>` — full macro system documentation
