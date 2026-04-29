.. _tutorial_macro_advanced_function_macro:

.. index::
   single: Tutorial; Macros; Advanced Function Macro
   single: Tutorial; Macros; Memoize
   single: Tutorial; Macros; patch
   single: Tutorial; Macros; transform

====================================================
 Macro Tutorial 4: Advanced Function Macros
====================================================

This tutorial builds a ``[memoize]`` function macro that demonstrates the
full ``apply()`` / ``patch()`` / ``transform()`` lifecycle of
``AstFunctionAnnotation``.  Where tutorial 3 used ``apply()`` alone to
rewrite a function body, here we generate *new* companion functions,
module-level cache variables, and redirect every call site — including
recursive ones — to a memoized wrapper.

.. code-block:: das

   [memoize]
   def fib(n : int) : int {
       if (n <= 1) { return n; }
       return fib(n - 1) + fib(n - 2)
   }

Without memoization, ``fib(30)`` would take over a billion recursive
calls.  With ``[memoize]``, each unique argument is computed only once —
exponential time becomes linear.


How the three methods work together
====================================

``AstFunctionAnnotation`` has several override points.  ``[memoize]``
uses three of them in sequence:

1. **``apply()``** — runs when the annotation is first attached to a
   function, *before* type inference.  We reject functions that cannot
   be memoized: generics (types are unknown), void returns (nothing to
   cache), and zero-argument functions (nothing to hash).

2. **``patch()``** — runs *after* type inference succeeds.  All types
   are resolved, so we can build the cache table type and the wrapper
   function.  Setting ``astChanged = true`` tells the compiler to
   restart inference so the new functions get type-checked.  The
   "already processed" guard (``find_arg(args, "patched") is tBool``)
   ensures we don't regenerate on the second pass.

3. **``transform()``** — runs on *every call site* of the annotated
   function during inference.  On the second pass (after ``patch()``
   generated the wrapper), it replaces each call with a call to the
   memoized wrapper.  On the first pass, it returns ``default`` to
   leave the call unchanged.


What patch() generates
=======================

For a function ``fib(n : int) : int``, ``patch()`` produces three things:

**A private copy of the original function** (without ``[memoize]``) so the
wrapper can call it without triggering ``transform()`` again:

.. code-block:: das

   def private `memoize`original`fib(n : int) : int {
       if (n <= 1) { return n; }
       return fib(n - 1) + fib(n - 2)
   }

**A private global cache variable:**

.. code-block:: das

   var private `memoize`cache`fib : table<uint64; int>

**A private wrapper function** that checks the cache, calls the original
on miss, and stores the result via ``insert_clone``:

.. code-block:: das

   def private `memoize`fib(n : int) : int {
       let key = hash(n)
       if (key_exists(`memoize`cache`fib, key)) {
           unsafe { return `memoize`cache`fib[key]; }
       }
       let result = `memoize`original`fib(n)
       `memoize`cache`fib |> insert_clone(key, result)
       return result
   }

Then ``transform()`` redirects every call to ``fib(...)`` —
including recursive calls inside ``fib`` itself — to
``\`memoize\`fib(...)``.


Module file: ``advanced_function_macro_mod.das``
=================================================

apply() — pre-inference validation
------------------------------------

.. code-block:: das

   [function_macro(name="memoize")]
   class MemoizeMacro : AstFunctionAnnotation {

       def override apply(var func : FunctionPtr; var group : ModuleGroup;
                          args : AnnotationArgumentList; var errors : das_string) : bool {
           if (func.isGeneric) {
               errors := "cannot memoize a generic function — all argument types must be specified"
               return false
           }
           if (func.result.isVoid) {
               errors := "cannot memoize a void function — there is nothing to cache"
               return false
           }
           if (length(func.arguments) == 0) {
               errors := "cannot memoize a function with no arguments — there is nothing to hash"
               return false
           }
           return true
       }

``apply()`` rejects functions at parse time — before the compiler has
resolved types.  The checks use pre-inference properties that are already
available: ``isGeneric``, ``result.isVoid``, and ``length(arguments)``.


patch() — code generation after inference
-------------------------------------------

The "already processed" guard
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

   def override patch(var fn : FunctionPtr; var group : ModuleGroup;
                      args, progArgs : AnnotationArgumentList;
                      var errors : das_string; var astChanged : bool&) : bool {

       // Guard: already processed?
       if (find_arg(args, "patched") is tBool) {
           return true
       }

Because ``patch()`` sets ``astChanged = true``, inference restarts and
``patch()`` is called again.  Without this guard, the macro would
generate duplicate functions and hit an infinite loop.

Mark as processed and trigger restart
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

       // Mark as processed and trigger inference restart
       for (ann in fn.annotations) {
           if (ann.annotation.name == "memoize") {
               astChanged = true
               ann.arguments |> add_annotation_argument("patched", true)
           }
       }

``add_annotation_argument`` stores data in the annotation that persists
across inference passes.  We also store the wrapper function name later,
so ``transform()`` can read it.

Step 1 — clone the original function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

       var inscope originalCopy <- clone_function(fn)
       originalCopy.name := originalCopyName
       originalCopy.flags |= FunctionFlags.generated | FunctionFlags.privateFunction
       // Remove [memoize] from the clone to prevent infinite transform loop
       let memoizeIdx = find_index_if(each(originalCopy.annotations)) $(ann) {
           return ann.annotation.name == "memoize"
       }
       if (memoizeIdx >= 0) {
           originalCopy.annotations |> erase(memoizeIdx)
       }
       compiling_module() |> add_function(originalCopy)

The wrapper needs to call the *real* implementation.  But ``transform()``
redirects *all* calls to the annotated function — including calls inside
the wrapper.  The solution is to clone the original, strip the
``[memoize]`` annotation from the clone, and have the wrapper call the
unannotated copy.

Step 2 — create the cache variable
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

       var inscope retType <- clone_type(fn.result)
       retType.flags &= ~TypeDeclFlags.constant
       retType.flags &= ~TypeDeclFlags.ref
       var inscope wrapperRetType <- clone_type(retType)

       var inscope keyType <- new TypeDecl(baseType = Type.tUInt64, at = fn.at)
       var inscope cacheType <- new TypeDecl(baseType = Type.tTable, at = fn.at)
       move(cacheType.firstType) <| keyType
       move(cacheType.secondType) <| retType
       add_global_var(compiling_module(), cacheName, clone_type(cacheType), fn.at, true)

The table type ``table<uint64; RetType>`` is built manually because
``$t()`` splicing doesn't work inside ``typeinfo ast_typedecl`` for table
value types.  ``clone_type(cacheType)`` is required because
``add_global_var`` takes ownership of the ``TypeDeclPtr`` without cloning
it — if you pass an ``inscope`` variable directly, it gets deleted at
scope exit and the compiler crashes on the next inference pass.

Step 4 — hash key computation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

       var inscope hashExprs : array<ExpressionPtr>
       for (arg in fn.arguments) {
           hashExprs |> emplace_new <| qmacro(hash($i(arg.name)))
       }

       // Combine hashes with XOR
       var inscope keyExpr <- hashExprs[0]
       for (i in range(1, length(hashExprs))) {
           if (true) {
               var inscope xorExpr <- qmacro($e(keyExpr) ^ $e(hashExprs[i]))
               unsafe { keyExpr <- xorExpr; }
           }
       }

For multiple arguments, the cache key is ``hash(a) ^ hash(b) ^ ...``.
In daslang, every type has a ``hash()`` function, so this works for
strings, floats, structs, etc.  The ``if (true)`` wrapper is a workaround
for ``var inscope`` not being allowed directly in loop bodies.

Step 6 — assemble the wrapper body
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

       var inscope bodyExprs : array<ExpressionPtr>
       bodyExprs |> emplace_new <| qmacro_expr(${ let key = $e(keyExpr); })
       bodyExprs |> emplace_new <| qmacro_expr(${ if (key_exists($i(cacheName), key)) { unsafe { return $i(cacheName)[key]; } } })
       bodyExprs |> emplace_new <| qmacro_expr(${ let result = $c(originalCopyName)($a(callArgs)); })
       bodyExprs |> emplace_new <| qmacro_expr(${ $i(cacheName) |> insert_clone(key, result); })
       bodyExprs |> emplace_new <| qmacro_expr(${ return result; })

Each ``qmacro_expr`` generates one statement.  The splicing operators:

- ``$e(expr)`` — splice an expression AST node
- ``$i(name)`` — splice a name as an identifier (``ExprVar``)
- ``$c(name)`` — splice a name into a call expression (``ExprCall``)
- ``$a(array)`` — splice an array of expressions as arguments
- ``$t(type)`` — splice a type declaration

Step 7–8 — create and add the wrapper function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: das

       var inscope wrapperFn <- qmacro_function(wrapperName) $($a(wrapperArgs)) : $t(wrapperRetType) {
           $b(bodyExprs)
       }
       wrapperFn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction
       wrapperFn.body |> force_at(fn.body.at)
       compiling_module() |> add_function(wrapperFn)

       // Store the wrapper name for transform() to read
       for (ann in fn.annotations) {
           if (ann.annotation.name == "memoize") {
               ann.arguments |> add_annotation_argument("wrapper", wrapperName)
           }
       }

``qmacro_function`` creates a new ``FunctionPtr`` with ``$b(bodyExprs)``
splicing the body statements.  ``force_at`` adjusts source locations so
error messages point to the original function.  The wrapper name is stored
in the annotation arguments — ``transform()`` reads it on the next pass.


transform() — call-site redirection
--------------------------------------

.. code-block:: das

   def override transform(var call : smart_ptr<ExprCallFunc>;
                          var errors : das_string) : ExpressionPtr {
       for (ann in call.func.annotations) {
           if (ann.annotation.name == "memoize") {
               let wrapperArg = find_arg(ann.arguments, "wrapper")
               if (wrapperArg is tString) {
                   let wrapperName = wrapperArg as tString
                   var inscope newCall <- clone_expression(call)
                   (newCall as ExprCall).name := wrapperName
                   return <- newCall
               }
           }
       }
       return <- default<ExpressionPtr>
   }

``transform()`` is called for every call to the annotated function.  It
reads the wrapper name from the annotation, clones the call expression,
changes the function name to the wrapper, and returns the replacement.
When it returns a non-default value, the compiler automatically
reports ``astChanged`` — no manual flag needed.

On the first inference pass (before ``patch()`` runs), the ``"wrapper"``
argument doesn't exist yet, so ``transform()`` returns ``default`` and
the call goes through unchanged.


Usage file: ``04_advanced_function_macro.das``
===============================================

.. code-block:: das

   options gen2
   require advanced_function_macro_mod

   [memoize]
   def fib(n : int) : int {
       if (n <= 1) { return n; }
       return fib(n - 1) + fib(n - 2)
   }

   [memoize]
   def slow_add(a, b : int) : int {
       return a + b
   }

   [memoize]
   def greet(name : string) : string {
       return "hello, {name}!"
   }

   [export]
   def main() {
       print("fib(10) = {fib(10)}\n")
       print("fib(20) = {fib(20)}\n")
       print("fib(30) = {fib(30)}\n")
       print("slow_add(3, 4) = {slow_add(3, 4)}\n")
       print("slow_add(3, 4) = {slow_add(3, 4)}\n")  // cached
       print("{greet("daslang")}\n")
       print("{greet("daslang")}\n")  // cached
   }

Output::

   fib(10) = 55
   fib(20) = 6765
   fib(30) = 832040
   slow_add(3, 4) = 7
   slow_add(3, 4) = 7
   hello, daslang!
   hello, daslang!

Three functions are memoized: ``fib`` (recursive, single int argument),
``slow_add`` (multi-argument, hash XOR), and ``greet`` (string result,
cloneable via ``insert_clone``).  The second calls to ``slow_add`` and
``greet`` hit the cache.


Compile-time error examples
============================

The ``apply()`` method rejects invalid uses at compile time:

.. code-block:: das

   // ERROR: cannot memoize a void function — there is nothing to cache
   // [memoize]
   // def fire(x : int) { print("fire {x}\n"); }

   // ERROR: cannot memoize a function with no arguments — there is nothing to hash
   // [memoize]
   // def get_zero() : int { return 0; }


Key techniques summary
=======================

+------------------------------+----------------------------------------------+
| Technique                    | What it does                                 |
+==============================+==============================================+
| ``apply()``                  | Pre-inference validation: ``isGeneric``,     |
|                              | ``isVoid``, argument count                   |
+------------------------------+----------------------------------------------+
| ``patch()``                  | Post-inference code generation with          |
|                              | ``astChanged`` restart                       |
+------------------------------+----------------------------------------------+
| ``transform()``              | Call-site redirection: clone expression,     |
|                              | rename function                              |
+------------------------------+----------------------------------------------+
| "Already processed" guard    | ``find_arg(args, "patched") is tBool``       |
|                              | prevents double generation                   |
+------------------------------+----------------------------------------------+
| ``add_annotation_argument``  | Stores data across inference passes          |
+------------------------------+----------------------------------------------+
| ``clone_function``           | Deep-clones a function with all annotations  |
+------------------------------+----------------------------------------------+
| ``add_global_var``           | Creates module-level variables at compile    |
|                              | time (pass ``clone_type``, not ``inscope``)  |
+------------------------------+----------------------------------------------+
| ``qmacro_function``          | Reification: builds a function from spliced  |
|                              | arguments, body, and return type             |
+------------------------------+----------------------------------------------+
| ``qmacro_expr``              | Reification: builds individual statements    |
+------------------------------+----------------------------------------------+
| ``clone_expression``         | Deep-clones an ``ExpressionPtr``             |
+------------------------------+----------------------------------------------+
| ``find_unique_function``     | Checks whether a function already exists in  |
|                              | a module                                     |
+------------------------------+----------------------------------------------+
| ``insert_clone``             | Table insertion that clones the value        |
+------------------------------+----------------------------------------------+
| ``hash()`` + XOR             | Multi-argument cache key via                 |
|                              | ``hash(a) ^ hash(b)``                        |
+------------------------------+----------------------------------------------+


.. seealso::

   Full source:
   :download:`advanced_function_macro_mod.das <../../../../../tutorials/macros/advanced_function_macro_mod.das>`,
   :download:`04_advanced_function_macro.das <../../../../../tutorials/macros/04_advanced_function_macro.das>`

   Previous tutorial: :ref:`tutorial_macro_function_macro`

   Next tutorial: :ref:`tutorial_macro_tag_function_macro`

   Language reference: :ref:`Macros <macros>` — full macro system documentation
