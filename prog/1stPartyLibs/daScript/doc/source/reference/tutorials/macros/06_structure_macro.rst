.. _tutorial_macro_structure_macro:

.. index::
   single: Tutorial; Macros; Structure Macro
   single: Tutorial; Macros; structure_macro
   single: Tutorial; Macros; AstStructureAnnotation
   single: Tutorial; Macros; apply
   single: Tutorial; Macros; patch
   single: Tutorial; Macros; finish
   single: Tutorial; Macros; serializable

====================================================
 Macro Tutorial 6: Structure Macros
====================================================

Tutorials 3–5 transformed *function calls*.  Structure macros
operate on **struct and class definitions** instead.

``[structure_macro(name="X")]`` registers a class that extends
``AstStructureAnnotation``.  When a struct is annotated with
``[X]``, the compiler calls the macro's methods at three stages of
the compilation pipeline:

+---------------------+----------------------------------------------+
| Method              | When it runs                                 |
+=====================+==============================================+
| ``apply()``         | During parsing, **before** inference.        |
|                     | Can add fields, validate annotation          |
|                     | arguments, and generate new functions.       |
+---------------------+----------------------------------------------+
| ``patch()``         | **After** inference.  Types are resolved,    |
|                     | so type-aware checks are possible.  Can set  |
|                     | ``astChanged`` to restart inference.         |
+---------------------+----------------------------------------------+
| ``finish()``        | After all inference and optimization.        |
|                     | Read-only — useful for diagnostics.          |
+---------------------+----------------------------------------------+

This tutorial builds a ``[serializable]`` annotation that:

1. Adds a ``_version`` field and generates a **stub**
   ``describe_StructName()`` function — header only (``apply``).
2. Fills in the describe function body with per-field printing,
   **skipping** non-serializable types like function pointers and
   lambdas — only possible after inference (``patch``).
3. Prints a compile-time summary of each struct (``finish``).


.. code-block:: das

   [serializable(version=2)]
   struct Player {
       name : string
       health : int
       on_hit : function<(damage:int):void>   // skipped by describe
   }

After compilation the struct gains a ``_version : int = 2`` field and a
generated ``describe_Player`` function that prints ``name`` and
``health`` but skips ``on_hit`` (a function pointer).


Why three methods?
==================

Each method runs at a different point in the pipeline, which determines
what it can and cannot do:

* **apply()** — The struct definition is parsed but types are not
  resolved.  You can add fields and generate functions, but you cannot
  inspect ``fld._type.baseType`` because types are still
  ``autoinfer``.  Any generated functions **must** be created here so
  they exist when inference runs — but their bodies can be stubs
  that ``patch()`` fills in later.

* **patch()** — Runs after inference, so all types are resolved.  This
  is the place for type-aware code generation or validation.  After
  modifying a function body, set ``astChanged = true`` to restart
  inference.  Use a guard to avoid infinite loops (e.g., check
  whether the body length changed since the stub was created).

* **finish()** — Everything is final: types are resolved, code is
  optimized.  No modifications allowed.  Use it for diagnostics,
  compile-time reporting, or AOT-related output.


The module: ``structure_macro_mod.das``
=======================================

Registration
------------

.. code-block:: das

   [structure_macro(name="serializable")]
   class SerializableMacro : AstStructureAnnotation {
       ...
   }

``[structure_macro(name="serializable")]`` tells the compiler:

   *When a struct or class is annotated with* ``[serializable]``,
   *call this class's methods during compilation.*

Like ``[function_macro]``, the macro class must be compiled before
any module that uses the annotation — hence the two-file setup.


Inside ``apply()``
------------------

The method receives the struct definition, annotation arguments, and
an error string.  It runs during parsing, before inference.


Step 1 — Validate arguments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

   var version = 1
   for (arg in args) {
       if (arg.name == "version") {
           let val = get_annotation_argument_value(arg)
           if (val is tInt) {
               version = val as tInt
           } else {
               errors := "[serializable] 'version' argument must be an integer"
               return false
           }
       } else {
           errors := "[serializable] unknown argument — only 'version' is supported"
           return false
       }
   }

We iterate the ``AnnotationArgumentList`` and accept only
``version`` (integer).  Returning ``false`` aborts compilation with
the error message stored in ``errors``.


Step 2 — Add a field
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

   st |> add_structure_field("_version",
       clone_type(qmacro_type(type<int>)),
       qmacro($v(version)))

``add_structure_field`` appends a new field to the struct's field
list.  It **moves** both the ``TypeDeclPtr`` and ``ExpressionPtr``
arguments, so they must be either temporaries or clones.

.. warning::

   Never pass a ``var inscope`` variable directly to
   ``add_structure_field`` — it will be moved *and* destroyed at
   scope exit, causing a double-free crash.  Always pass
   ``clone_type(...)`` or an inline temporary.

``qmacro_type(type<int>)`` creates a ``TypeDeclPtr`` for ``int``.
``qmacro($v(version))`` creates an integer constant expression.


Step 3 — Generate a stub describe function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

   let funcName = "describe_{st.name}"
   var inscope bodyExprs : array<ExpressionPtr>

   bodyExprs |> emplace_new <| qmacro(print($v("{st.name} (version ")))
   bodyExprs |> emplace_new <| qmacro(print("{obj._version}"))
   bodyExprs |> emplace_new <| qmacro(print($v("):\n")))

   var inscope fn <- qmacro_function(funcName) $(obj : $t(st)) {
       $b(bodyExprs)
   }
   fn.flags |= FunctionFlags.generated
   fn.body |> force_at(st.at)
   add_function(st._module, fn)

The stub function prints only the header line — the struct name and
version.  **Per-field printing is deferred to** ``patch()`` **where
types are known.**

The function must exist before inference so callers (like ``main``)
can reference ``describe_Color()`` or ``describe_Player()``.  Its
body will be extended in ``patch()`` after types resolve.

Two kinds of content appear in ``qmacro``:

* **Compile-time constants** — field names, struct name.  Use
  ``$v("string")`` to splice a constant string into the generated
  code.  String interpolation like ``"{st.name}"`` resolves at
  *macro execution time* and becomes a literal.

* **Runtime values** — ``obj._version``, ``obj.$f(fld.name)``.
  Inside ``qmacro``, ``obj`` refers to the generated function's
  parameter.  ``$f(fld.name)`` splices a string as a field-access
  name.  To convert any value to a string for ``print``, use string
  interpolation: ``print("{obj.$f(fld.name)}")``.

``qmacro_function(funcName) $(obj : $t(st)) { $b(bodyExprs) }``
builds a complete function:

* ``funcName`` — a string for the function name
* ``$t(st)`` — splices the struct type as the parameter type
* ``$b(bodyExprs)`` — splices the statement array into the body

``add_function(st._module, fn)`` registers the function in the
struct's module so callers can find it.


Inside ``patch()``
------------------

This is the heart of the tutorial.  In ``apply()`` we created a stub
function — now we fill it in, using inferred type information to skip
non-serializable fields.

Step 1 — Guard against re-patching
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

   if (find_arg(args, "patched") is tBool) {
       return true
   }

Setting ``astChanged`` causes the compiler to re-run inference,
which calls ``patch()`` again.  Without a guard, this would loop
forever.  We add a ``"patched"`` annotation argument (in Step 5)
and check for it here — if present, the work is already done.


Step 2 — Find the stub function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

   let funcName = "describe_{st.name}"
   var inscope fn <- st._module |> find_unique_function(funcName)

``find_unique_function`` (from ``daslib/ast_boost``) searches a
module for a function by name.  It returns a ``smart_ptr<Function>``
pointing to the same object in the module — modifications through
this pointer affect the actual function.


Step 3 — Get the body as ExprBlock
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

   unsafe {
       var blk = reinterpret<ExprBlock?> fn.body

The function body is an ``ExpressionPtr`` internally — we know it
is an ``ExprBlock`` because we built it that way in ``apply()``.
``reinterpret<ExprBlock?>`` (requires ``unsafe``) gives us a typed
pointer so we can access the ``list`` array of statements.


Step 4 — Append field-printing statements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

       for (fld in st.fields) {
           if (fld.name == "_version") {
               continue
           }
           if (fld._type.baseType == Type.tLambda ||
               fld._type.baseType == Type.tFunction) {
               continue
           }
           blk.list |> emplace_new <| qmacro(print($v("  {fld.name} = ")))
           blk.list |> emplace_new <| qmacro(print("{obj.$f(fld.name)}"))
           blk.list |> emplace_new <| qmacro(print($v("\n")))
       }
   }

Now ``fld._type.baseType`` is resolved — this was ``autoinfer`` in
``apply()`` but is the real type here.  We skip fields whose type
is lambda or function pointer (blocks cannot appear as struct
fields, so no ``tBlock`` check is needed).  The ``obj`` reference
works because the generated expressions will be re-inferred inside
the function where ``obj`` is a parameter.


Step 5 — Mark as patched and trigger re-inference
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

   for (ann in st.annotations) {
       if (ann.annotation.name == "serializable") {
           ann.arguments |> add_annotation_argument("patched", true)
       }
   }
   astChanged = true

We add a ``"patched"`` boolean argument to our own annotation —
this is the marker that Step 1 checks on the next pass.  Then
``astChanged = true`` tells the compiler to re-run inference on
the modified function body.  On the next pass, ``find_arg(args,
"patched")`` returns ``tBool`` and ``patch()`` returns immediately.


Inside ``finish()``
-------------------

.. code-block:: das

   def override finish(var st : StructurePtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
       var serializable = 0
       var skipped = 0
       for (fld in st.fields) {
           if (fld.name == "_version") {
               continue
           }
           if (fld._type.baseType == Type.tLambda ||
               fld._type.baseType == Type.tFunction) {
               skipped++
           } else {
               serializable++
           }
       }
       print("[serializable] {st.name}: {serializable} serializable field(s)")
       if (skipped > 0) {
           print(", {skipped} skipped")
       }
       // ... print version
       return true
   }

``finish()`` runs after all inference and optimization.  The struct
is in its final form — we count serializable and skipped fields and
print a compile-time diagnostic.

``find_arg(args, "version")`` returns an ``RttiValue`` variant,
checked with ``is tInt`` / ``as tInt``.


The usage file
==============

.. code-block:: das

   options gen2
   require structure_macro_mod

   [serializable]
   struct Color {
       r : float
       g : float
       b : float
   }

   [serializable(version=2)]
   struct Player {
       name : string
       health : int
       score : float
       on_hit : function<(damage:int):void>
   }

   [export]
   def main() {
       var c = Color(r = 0.2, g = 0.7, b = 1.0)
       describe_Color(c)

       var p = Player(name = "Alice", health = 100, score = 42.5)
       describe_Player(p)

       print("Color version: {c._version}\n")
       print("Player version: {p._version}\n")
   }

``Color`` uses the default version (1) — all fields are plain types.
``Player`` specifies ``version=2`` and has an ``on_hit`` function
pointer field.  The generated ``describe_Player`` prints ``name``,
``health``, and ``score`` but **skips** ``on_hit`` because
``patch()`` detected it as ``Type.tFunction``.

Compile-time output (from ``finish``)::

   [serializable] Color: 3 serializable field(s), version 1
   [serializable] Player: 3 serializable field(s), 1 skipped, version 2

Runtime output::

   --- describe_Color ---
   Color (version 1):
     r = 0.2
     g = 0.7
     b = 1

   --- describe_Player ---
   Player (version 2):
     name = Alice
     health = 100
     score = 42.5

   --- version info ---
   Color version: 1
   Player version: 2


Compilation pipeline summary
=============================

The full sequence for a ``[serializable]`` struct:

.. code-block:: text

   parse struct definition
     ↓
   apply() → add _version field, generate stub describe_X()
     ↓
   infer types (stub function is inferred with header-only body)
     ↓
   patch() → find stub, append field prints (skip bad types), mark "patched", astChanged
     ↓
   re-infer (modified body now has field-specific print statements)
     ↓
   patch() → "patched" arg found → return immediately
     ↓
   optimize
     ↓
   finish() → compile-time diagnostic (serializable vs skipped)
     ↓
   simulate → run


Key takeaways
=============

.. list-table::
   :widths: 40 60
   :header-rows: 1

   * - Concept
     - What it does
   * - ``[structure_macro(name="X")]``
     - Registers a class as a structure annotation
   * - ``AstStructureAnnotation``
     - Base class with ``apply``, ``patch``, ``finish`` methods
   * - ``apply()``
     - Pre-inference: add fields, generate stub functions
   * - ``patch()``
     - Post-inference: fill in bodies using type info, set ``astChanged``
   * - ``finish()``
     - Final: read-only diagnostics and reporting
   * - ``astChanged``
     - Set ``true`` in ``patch`` to restart inference after changes
   * - ``add_annotation_argument``
     - Marks the annotation as processed to prevent infinite re-patching
   * - ``find_unique_function``
     - Locates a function by name in a module (``ast_boost``)
   * - ``reinterpret<ExprBlock?>``
     - Casts ``fn.body`` to ``ExprBlock?`` to access the statement list
   * - ``add_structure_field``
     - Appends a field to a struct; moves both type and init expression
   * - ``clone_type``
     - Deep-clones a ``TypeDeclPtr``; required before move operations
   * - ``qmacro_function``
     - Builds a complete function from reification splices
   * - ``$v(value)``
     - Splice a compile-time value as a constant expression
   * - ``$f(name)``
     - Splice a string as a field-access name
   * - ``$t(type)``
     - Splice a ``TypeDeclPtr`` into parameter/return types
   * - ``$b(stmts)``
     - Splice ``array<ExpressionPtr>`` as a statement list
   * - ``find_arg``
     - Look up annotation argument values by name
   * - ``force_at``
     - Stamps source location on all generated AST nodes


.. seealso::

   Full source:
   :download:`structure_macro_mod.das <../../../../../tutorials/macros/structure_macro_mod.das>`,
   :download:`06_structure_macro.das <../../../../../tutorials/macros/06_structure_macro.das>`

   Previous tutorial: :ref:`tutorial_macro_tag_function_macro`

   Next tutorial: :ref:`tutorial_macro_block_macro`

   Standard library examples:
   ``daslib/interfaces.das`` (``apply`` + ``finish``),
   ``daslib/decs_boost.das`` (``apply`` with field iteration)

   Language reference: :ref:`Macros <macros>` — full macro system documentation
