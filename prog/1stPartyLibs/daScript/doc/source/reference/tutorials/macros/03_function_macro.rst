.. _tutorial_macro_function_macro:

.. index::
   single: Tutorial; Macros; Function Macro

====================================================
 Macro Tutorial 3: Function Macros
====================================================

This tutorial builds three function macros that demonstrate the three
main methods of ``AstFunctionAnnotation``:

* **``[log_calls]``** uses ``apply()`` to rewrite a function's body,
  adding entry/exit logging with nested-call indentation.
* **``[expect_range]``** uses ``verifyCall()`` to validate every call
  site, rejecting constant arguments that fall outside a given range.
* **``[no_print]``** uses ``lint()`` to walk the fully-compiled body
  and reject calls to the builtin ``print`` function.

.. code-block:: das

   [log_calls]
   def add(a, b : int) : int {
       return a + b
   }

At compile time, ``[log_calls]`` rewrites the function body to:

.. code-block:: das

   def add(a, b : int) : int {
       if (true) {
           print(">> ")
           print("add({a}, {b})\n")
           if (true) {
               return a + b       // original body
           }
       } finally {
           print("<< add\n")
       }
   }

Recursive calls produce indented output that visualizes the call tree::

   >> fib(3)
     >> fib(2)
       >> fib(1)
       << fib
       >> fib(0)
       << fib
     << fib
     >> fib(1)
     << fib
   << fib

New concepts introduced:

* **``[function_macro]``** — macro kind that modifies functions
* **``AstFunctionAnnotation``** — base class with ``apply()``,
  ``verifyCall()``, and ``lint()`` methods
* **``apply()``** — transforms a function’s AST at definition time
* **``verifyCall()``** — validates each call site after type inference
* **``lint()``** — walks the fully-compiled AST after all types are resolved
* **``ExprStringBuilder``** — build string interpolations from AST nodes
* **``qmacro_block`` with ``finally``** — generate statement blocks
  with cleanup sections
* **``if (true) { ... }``** — scoping trick for variable isolation
* **``$e(func.body)``** — inject the original function body
* **``func.body |> move``** — replace the function body
* **``var public``** — module-level mutable state shared across modules
* **``AnnotationArgumentList``** — reading annotation argument names
  and values (``iValue``)
* **``ExprConstInt``** — extracting compile-time integer values from AST
* **``AstVisitor``** — walking the compiled AST tree
* **``make_visitor``** / **``visit()``** — adapting and running a visitor
* **``expr.func._module.name``** — identifying a function’s source module


Prerequisites
=============

You should be comfortable with the material in
:ref:`tutorial_macro_call_macro` and :ref:`tutorial_macro_when_expression`
— ``[call_macro]``, ``visit()``, ``qmacro``, ``$e()``, ``$i()``, and
``qmacro_block``.


Function macros vs. call macros
===============================

Call macros (tutorials 1 and 2) transform **call expressions** —
they receive a call site and return a replacement expression.

Function macros transform **function definitions**.  They receive a
``FunctionPtr`` and modify the function's AST (body, arguments, return
type, annotations) before the function is compiled.  The base class
is ``AstFunctionAnnotation`` and it provides several overridable methods:

* **``apply()``** — runs once when the function is compiled.
  Use it to transform the function’s body, arguments, or annotations.
* **``verifyCall()``** — runs at every call site after type inference.
  Use it to validate arguments (return ``false`` to reject the call).
* **``transform()``** — runs at every call site and can replace the
  call expression entirely (used by ``[constant_expression]`` in daslib).
* **``lint()``** — runs after the function is fully compiled (types
  resolved, overloads selected).  Use it to validate structural
  properties of the finished AST.

This tutorial demonstrates ``apply()`` with ``[log_calls]``,
``verifyCall()`` with ``[expect_range]``, and ``lint()`` with
``[no_print]``.


Part 1: [log_calls] — apply()
==============================

The ``apply()`` method receives the function being compiled and can
modify its AST arbitrarily:

.. code-block:: das

   [function_macro(name="log_calls")]
   class LogCallsMacro : AstFunctionAnnotation {
       def override apply(var func : FunctionPtr;
                          var group : ModuleGroup;
                          args : AnnotationArgumentList;
                          var errors : das_string) : bool {
           // ... transform func ...
           return true
       }
   }

Returning ``true`` from ``apply()`` means the transformation succeeded.
Returning ``false`` aborts compilation with an error.


Building the call signature string
===================================

To log which function was called and with what arguments, we need a
string like ``"add(2, 3)\n"`` at runtime.  This must be built as an
``ExprStringBuilder`` — a compile-time AST node that generates string
interpolation code:

.. code-block:: das

   var inscope call_sb <- new ExprStringBuilder(at = func.at)
   call_sb.elements |> emplace_new <| qmacro($v("{string(func.name)}("))
   for (i, arg in count(), func.arguments) {
       if (i > 0) {
           call_sb.elements |> emplace_new <| quote(", ")
       }
       call_sb.elements |> emplace_new <| qmacro($i(arg.name))
   }
   call_sb.elements |> emplace_new <| quote(")\n")

How it works:

* **``ExprStringBuilder``** is the AST node behind daslang's ``"..."``
  string interpolation.  Its ``elements`` array holds a mix of literal
  strings and expression nodes that become ``{expr}`` segments.
* **``$v("text")``** (value) injects a constant string — here the function
  name and opening parenthesis.
* **``$i(arg.name)``** (identifier) creates a variable reference — at
  runtime it evaluates to the argument's actual value.
* **``quote("text")``** creates a literal string expression — used for
  fixed separators like ``", "`` and ``")\n"``.
* **``count()``** and **``func.arguments``** — the arguments array on
  ``FunctionPtr`` holds the function's parameter declarations.
  ``count()`` provides a 0-based index for the comma-separator logic.

For ``add(a, b : int)``, this produces the equivalent of::

  "{string(func.name)}({a}, {b})\n"

which at runtime evaluates to ``"add(2, 3)\n"``.


The if(true) scoping pattern
============================

The generated code uses ``if (true) { ... }`` blocks that may look
redundant.  They serve a real purpose — each ``if (true) { ... }``
creates a **new lexical scope**.  This is important because:

1. The macro may introduce local variables (like ``ref_time`` in extended
   versions).  Scoping prevents name clashes with the original body.
2. The original body may contain ``return`` statements.  Wrapping it in
   a scope ensures ``finally`` still runs.
3. Multiple ``[log_calls]`` annotations (or other function macros) can
   each add their own scoped variables without conflicts.


Constructing the replacement body
=================================

The heart of the macro builds a new function body using
``qmacro_block``.  This generates a statement list (``ExprBlock``)
rather than a single expression:

.. code-block:: das

   var inscope new_body <- qmacro_block() {
       if (true) {
           print("{repeat("  ",LOG_DEPTH++)}>> ")
           print($e(call_sb))
           if (true) {
               $e(func.body)
           }
       } finally {
           print("{repeat("  ",--LOG_DEPTH)}<< {$v(string(func.name))}\n")
       }
   }

Key details:

* **``qmacro_block() { ... }``** produces an ``ExpressionPtr`` containing
  a block of statements (an ``ExprBlock``).  Unlike ``qmacro()`` which
  produces a single expression, ``qmacro_block`` can hold multiple
  statements, ``if``/``else``, ``finally``, etc.
* **``$e(call_sb)``** splices in the ``ExprStringBuilder`` we built
  earlier.  At runtime this becomes the ``print("add(2, 3)\n")`` call.
* **``$e(func.body)``** splices the function's **original body** into the
  inner ``if (true)`` block.  This is the key technique — the macro wraps
  the original code rather than replacing it.
* **``finally { ... }``** — the generated block has a ``finally`` section
  that runs even when the original body executes a ``return``.  This
  guarantees the exit log line is always printed and ``LOG_DEPTH`` is
  decremented.
* **``LOG_DEPTH++``** / **``--LOG_DEPTH``** — pre/post-increment controls
  indentation depth.  ``repeat("  ", LOG_DEPTH++)`` prints the current
  depth's indentation then increments; ``--LOG_DEPTH`` decrements before
  printing the exit indentation.
* **``$v(string(func.name))``** injects the function name as a compile-time
  constant string into the exit log.


Replacing the function body
===========================

Finally, we swap the function's body with our new block:

.. code-block:: das

   func.body |> move <| new_body
   return true

``move`` replaces ``func.body`` with ``new_body`` and clears
``new_body``.  This is the standard pattern for function body
replacement in ``apply()`` — the old body has already been captured
inside the new one via ``$e(func.body)``, so no information is lost.


Public variables for shared state
=================================

``LOG_DEPTH`` is declared at module scope with ``var public``:

.. code-block:: das

   var public LOG_DEPTH = 0

This makes it accessible from any module that ``require``-s
``function_macro_mod``.  Each ``[log_calls]`` function increments it
on entry and decrements on exit, producing correct indentation for
nested calls.  Because it is a single global variable, it tracks
depth across all annotated functions — not just recursive ones.


Part 2: [expect_range] — verifyCall()
======================================

While ``apply()`` transforms the function at definition time,
``verifyCall()`` runs at every **call site** after type inference.
It receives the call expression and can accept or reject it.

.. code-block:: das

   [function_macro(name="expect_range")]
   class ExpectRangeMacro : AstFunctionAnnotation {
       def override verifyCall(var call : smart_ptr<ExprCallFunc>;
                               args, progArgs : AnnotationArgumentList;
                               var errors : das_string) : bool {
           // ... validate call.arguments ...
           return true
       }
   }

The parameters:

* **``call``** — the call expression at the call site.  ``call.func``
  is the function being called, ``call.arguments`` are the argument
  expressions.
* **``args``** — the annotation’s argument list (e.g., for
  ``[expect_range(value, min=0, max=255)]``, it contains three entries).
* **``errors``** — an output string for the error message.  Set it and
  return ``false`` to produce a compile error.

Returning ``false`` emits error code ``40102`` (``annotation_failed``).


Reading annotation argument values
==================================

Annotation arguments like ``[expect_range(value, min=0, max=255)]``
are stored in an ``AnnotationArgumentList``.  Each entry has a
``name`` and typed value fields:

.. code-block:: das

   var arg_name = ""
   var range_min = int(0x80000000)
   var range_max = int(0x7FFFFFFF)
   for (aa in args) {
       if (aa.basicType == Type.tBool) {
           arg_name = string(aa.name)   // bare name: "value"
       } elif (aa.name == "min") {
           range_min = aa.iValue         // integer value: 0
       } elif (aa.name == "max") {
           range_max = aa.iValue         // integer value: 255
       }
   }

How it works:

* **Bare names** like ``value`` in ``[expect_range(value, ...)]`` are
  stored with ``basicType == Type.tBool`` and their name is the
  argument identifier.  This is how ``[constexpr(a)]`` in daslib’s
  ``constant_expression.das`` identifies which function parameter to check.
* **Named values** like ``min=0`` are stored with their name (``"min"``)
  and the integer value in ``iValue``.
* **``string(aa.name)``** — converts the name for comparison.  For
  ``name == "min"`` the comparison works directly.


Extracting constant integer values
==================================

To check whether a call-site argument is a compile-time constant and
extract its value, we need a helper that navigates the AST:

.. code-block:: das

   [macro_function]
   def public getConstantInt(expr : ExpressionPtr;
                             var result : int&) : bool {
       if (expr is ExprRef2Value) {
           return getConstantInt(
               (expr as ExprRef2Value).subexpr, result)
       } elif (expr is ExprConstInt) {
           result = (expr as ExprConstInt).value
           return true
       }
       return false
   }

Key details:

* **``ExprRef2Value``** — the compiler sometimes wraps constant values
  in a reference-to-value conversion node.  The helper unwraps it
  recursively via ``.subexpr``.
* **``ExprConstInt``** — one of the ``ExprConst*`` family of AST nodes
  (``ExprConstFloat``, ``ExprConstString``, ``ExprConstBool``, etc.).
  All have a ``.value`` field of the corresponding type.
* **``is`` / ``as``** — daslang's type-test and downcast operators work
  on AST node types just like on classes.
* **``[macro_function]``** — marks the function as available during
  compilation (macro expansion time).  Without this annotation, the
  function would only exist at runtime.

Daslib’s ``constant_expression.das`` uses a more general approach:
``expr.__rtti |> starts_with("ExprConst")`` checks for *any* constant
type.  Our helper is specific to integers because ``[expect_range]``
only makes sense for numeric bounds.


Reporting compile-time errors
=============================

The error reporting pattern is straightforward — set the ``errors``
string and return ``false``:

.. code-block:: das

   var val = 0
   if (getConstantInt(ce, val)) {
       if (val < range_min || val > range_max) {
           errors := "{arg_name} = {val} is out of range [{range_min}..{range_max}]"
           return false
       }
   }

The compiler wraps this into a full error message::

   error[40102]: call annotated by expect_range failed
   _test_error.das:12:4
       set_channel("red", 300)
       ^^^^^^^^^^^
   value = 300 is out of range [0..255]

The error code ``40102`` (``annotation_failed``) is always the same for
``verifyCall`` failures.  The string you set in ``errors`` becomes the
detail message below the source location.


Runtime values pass through
===========================

An important design decision: ``verifyCall`` only checks **constant**
arguments.  When a runtime variable is passed, ``getConstantInt``
returns ``false`` and the call is allowed:

.. code-block:: das

   var alpha = 200
   set_channel("alpha", alpha)   // runtime value — compiles fine

This is intentional.  ``verifyCall`` is a **best-effort compile-time
check** — it catches mistakes in literal arguments but cannot validate
runtime expressions.  For runtime validation, you would use ``apply()``
to inject runtime bounds checks into the function body.


Part 3: [no_print] — lint()
===========================

The ``lint()`` method runs after the function is **fully compiled** —
types are resolved, overloads are selected, and the AST is ready to
simulate.  This makes it ideal for structural validation that needs
complete type information.

.. code-block:: das

   [function_macro(name="no_print")]
   class NoPrintMacro : AstFunctionAnnotation {
       def override lint(var func : FunctionPtr;
                         var group : ModuleGroup;
                         args, progArgs : AnnotationArgumentList;
                         var errors : das_string) : bool {
           // ... walk func body ...
           return true
       }
   }

Compared to the other methods:

* **``apply()``** — runs at definition time, before type checking.
  The body has parsed expressions but types may not be resolved yet.
* **``verifyCall()``** — runs at each call site after type inference.
  Has access to call arguments but not the full function body.
* **``lint()``** — runs after everything is compiled.  The function’s
  ``body`` has full type annotations, ``ExprCall`` nodes have their
  ``.func`` pointers linked to the resolved ``Function`` objects.

Ironic contrast: ``[log_calls]`` *adds* print calls to every function,
while ``[no_print]`` *forbids* them.


Walking the AST with a visitor
==============================

To inspect the function body, ``lint()`` uses the **visitor pattern**.
We define a class that inherits from ``AstVisitor`` and overrides
``preVisitExprCall`` to intercept function calls:

.. code-block:: das

   [macro]
   class NoPrintVisitor : AstVisitor {
       found_print : bool = false
       @safe_when_uninitialized print_at : LineInfo
       def override preVisitExprCall(
               expr : smart_ptr<ExprCall>) : void {
           if (expr.func != null
                   && expr.name == "print"
                   && expr.func._module.name == "$") {
               found_print = true
               print_at = expr.at
           }
       }
   }

Key details:

* **``[macro]``** — required annotation for classes used during
  compilation.  Without it, the visitor class would not exist at
  macro expansion time.
* **``preVisitExprCall``** — called before each ``ExprCall`` node in
  the AST walk.  The ``AstVisitor`` base class has ``preVisit*`` and
  ``visit*`` hooks for every AST node type (``ExprFor``, ``ExprWhile``,
  ``ExprNew``, etc.).
* **``@safe_when_uninitialized``** — ``LineInfo`` is a struct with
  no default initializer.  This annotation tells the compiler it is
  intentionally left uninitialized until ``found_print`` is set.
* **``expr.func``** — at lint time, this pointer is always linked
  to the resolved ``Function`` object (unlike at ``apply()`` time
  where function resolution may not be complete).


Checking the function’s module
==============================

The check ``expr.func._module.name == "$"`` distinguishes the builtin
``print`` from any user-defined function that happens to be named
``print``:

* **``_module``** — the field name uses an underscore prefix because
  ``module`` is a reserved keyword in daslang.  In C++ the field is
  ``Function::module``; in daslang macros it is ``func._module``.
* **``"$"``** — the builtin module name.  All built-in functions
  (``print``, ``assert``, ``length``, math functions, etc.) belong
  to module ``"$"``.
* This pattern is used throughout daslib — for example,
  ``daslib/lint.das`` checks ``expr.func._module.name |> eq <| "$"``
  to detect calls to ``panic``.


Running the visitor from lint()
===============================

The ``lint()`` method creates the visitor, adapts it with
``make_visitor``, and walks the function body with ``visit()``:

.. code-block:: das

   def override lint(...) : bool {
       var astVisitor = new NoPrintVisitor()
       var inscope adapter <- make_visitor(*astVisitor)
       visit(func, adapter)
       if (astVisitor.found_print) {
           errors := "function {string(func.name)} must not call builtin print"
           unsafe { delete astVisitor; }
           return false
       }
       unsafe { delete astVisitor; }
       return true
   }

* **``make_visitor(*astVisitor)``** — wraps the daslang visitor object
  into an adapter that the C++ ``visit()`` function can call.  The
  ``*`` dereferences the smart pointer.
* **``visit(func, adapter)``** — walks the function’s AST, calling
  the visitor’s ``preVisit*`` / ``visit*`` hooks at each node.
* **Error reporting** uses the same pattern as ``verifyCall()`` —
  set ``errors`` and return ``false``.  The compiler emits error
  code ``40102`` with message text ``"function annotation lint failed"``
  plus your detail string.
* **``unsafe { delete astVisitor; }``** — explicit cleanup of the
  heap-allocated visitor.  Required because ``new`` allocates on
  the heap and the visitor is not managed by ``inscope``.


Usage examples
==============

[log_calls] — simple functions:

.. code-block:: das

   [log_calls]
   def add(a, b : int) : int {
       return a + b
   }

   [log_calls]
   def greet(name : string) {
       print("hello, {name}!\n")
   }

Calling ``add(2, 3)`` produces::

   >> add(2, 3)
   << add

Calling ``greet("daslang")`` produces::

   >> greet(daslang)
   hello, daslang!
   << greet

Recursive functions show the call tree via indentation:

.. code-block:: das

   [log_calls]
   def fib(n : int) : int {
       if (n <= 1) {
           return n
       } else {
           return fib(n - 1) + fib(n - 2)
       }
   }

Calling ``fib(3)`` produces::

   >> fib(3)
     >> fib(2)
       >> fib(1)
       << fib
       >> fib(0)
       << fib
     << fib
     >> fib(1)
     << fib
   << fib

[expect_range] — compile-time bounds checking:

.. code-block:: das

   [expect_range(value, min=0, max=255)]
   def set_channel(name : string; value : int) {
       print("  {name} = {value}\n")
   }

Valid calls compile normally::

   set_channel("red", 128)    // ok
   set_channel("green", 0)    // ok
   set_channel("blue", 255)   // ok

Out-of-range constants are rejected at compile time:

.. code-block:: das

   // set_channel("red", 300)   // compile error!
   // error[40102]: call annotated by expect_range failed
   //   value = 300 is out of range [0..255]

Runtime variables are allowed through:

.. code-block:: das

   var alpha = 200
   set_channel("alpha", alpha)   // runtime value — passes through

[no_print] — lint-time structural validation:

.. code-block:: das

   [no_print]
   def compute(a, b : int) : int {
       return a * b + 1
   }

This compiles — ``compute`` has no ``print`` calls.  Adding
``[no_print]`` to a function that calls ``print`` fails at lint time:

.. code-block:: das

   // [no_print]   // uncomment to see:
   // error[40102]: function annotation lint failed
   //   function bad_compute must not call builtin print
   def bad_compute(a, b : int) : int {
       print("computing {a} * {b}\n")
       return a * b
   }


Extending with timing
=====================

A natural extension is to add execution timing using
``ref_time_ticks()`` and ``get_time_nsec()``.  The pattern is the
same — the only addition is a local timing variable in the generated
body:

.. code-block:: das

   var inscope new_body <- qmacro_block() {
       if (true) {
           let ref_time = ref_time_ticks()
           print(">> ")
           print($e(call_sb))
           if (true) {
               $e(func.body)
           }
       } finally {
           print("<< {$v(string(func.name))} - {get_time_nsec(ref_time)}ns\n")
       }
   }

The ``if (true)`` scope keeps ``ref_time`` local to each instrumented
function, preventing name clashes when multiple ``[log_calls]`` functions
call each other.


Running the tutorial
====================

::

  daslang.exe tutorials/macros/03_function_macro.das

Expected output::

  >> add(2, 3)
  << add
  sum = 5

  >> greet(daslang)
  hello, daslang!
  << greet

  >> fib(3)
    >> fib(2)
      >> fib(1)
      << fib
      >> fib(0)
      << fib
    << fib
    >> fib(1)
    << fib
  << fib
  fib(3) = 2

  color channels:
    red = 128
    green = 0
    blue = 255
    alpha = 200

  compute = 13


.. seealso::

   Full source:
   :download:`function_macro_mod.das <../../../../../tutorials/macros/function_macro_mod.das>`,
   :download:`03_function_macro.das <../../../../../tutorials/macros/03_function_macro.das>`

   Previous tutorial: :ref:`tutorial_macro_when_expression`

   Next tutorial: :ref:`tutorial_macro_advanced_function_macro`

   Language reference: :ref:`Macros <macros>` — full macro system documentation
