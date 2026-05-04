.. _tutorial_integration_cpp_calling_functions:

.. index::
   single: Tutorial; C++ Integration; Calling Functions

==========================================
 C++ Integration: Calling Functions
==========================================

This tutorial shows **two ways** to call daslang functions from C++:

* **Part A — Low-level:** manual ``cast<T>::from`` / ``to`` +
  ``evalWithCatch``.  Gives maximum control over the calling convention.
* **Part B — High-level:** ``das_invoke_function`` / ``das_invoke_function_by_name``.
  Type-safe variadic templates that handle marshalling and AOT dispatch
  automatically.  **Prefer this in production code.**

.. note::

   All argument and return value marshalling goes through ``vec4f``, the
   SIMD-aligned register type that daslang uses for its calling convention.
   Part A works with ``vec4f`` directly; Part B hides it behind templates.


Prerequisites
=============

* Tutorial 01 completed (:ref:`tutorial_integration_cpp_hello_world`).
* Familiarity with the daslang lifecycle (init → compile → simulate →
  eval → shutdown).


The daslang file
=================

The script exports several functions with different signatures:

.. code-block:: das

   options gen2

   [export]
   def add(a : int; b : int) : int {
       return a + b
   }

   [export]
   def square(x : float) : float {
       return x * x
   }

   [export]
   def greet(name : string) : string {
       return "Hello, {name}!"
   }

   struct Vec2 {
       x : float
       y : float
   }

   [export]
   def make_vec2(x : float; y : float) : Vec2 {
       return Vec2(x = x, y = y)
   }

   [export]
   def will_fail() {
       panic("something went wrong!")
   }


Part A — Low-level calling
==========================

Passing arguments with ``cast<T>::from``
-----------------------------------------

Every daslang function takes arguments as an array of ``vec4f``.  Use
``cast<T>::from(value)`` to pack a C++ value into a ``vec4f`` slot:

.. code-block:: cpp

   vec4f args[2];
   args[0] = cast<int32_t>::from(17);   // int
   args[1] = cast<int32_t>::from(25);   // int

   vec4f ret = ctx.evalWithCatch(fnAdd, args);

The supported primitive types are:

==============  ============================
C++ type        ``cast<>`` specialization
==============  ============================
``int32_t``     ``cast<int32_t>``
``uint32_t``    ``cast<uint32_t>``
``int64_t``     ``cast<int64_t>``
``float``       ``cast<float>``
``double``      ``cast<double>``
``bool``        ``cast<bool>``
``char *``      ``cast<char *>``
pointers        ``cast<T *>``
==============  ============================


Reading return values with ``cast<T>::to``
-------------------------------------------

The return value of ``evalWithCatch`` is also a ``vec4f``.  Extract it with
``cast<T>::to(ret)``:

.. code-block:: cpp

   int32_t result = cast<int32_t>::to(ret);

For strings, the returned ``char *`` points into the context's heap and
remains valid until the context is destroyed or garbage-collected:

.. code-block:: cpp

   const char * result = cast<char *>::to(ret);


Verifying function signatures
------------------------------

``verifyCall<ReturnType, ArgTypes...>`` checks a ``SimFunction``'s debug info
against the expected C++ types:

.. code-block:: cpp

   if (!verifyCall<int32_t, int32_t, int32_t>(fnAdd->debugInfo, dummyLibGroup)) {
       tout << "'add' has wrong signature\n";
       return;
   }

This is a **development-time safety net** — it walks debug info and is
slow, so call it once during setup, never in a hot loop.


Functions returning structs (cmres)
------------------------------------

When a daslang function returns a struct, the caller supplies a result
buffer.  Use the three-argument form of ``evalWithCatch``:

.. code-block:: cpp

   // C++ struct layout must exactly match the daslang struct.
   struct Vec2 { float x; float y; };

   Vec2 v;
   ctx.evalWithCatch(fnMakeVec2, args, &v);

The third argument (``void * cmres``) is where the result is written.


Handling exceptions
--------------------

If the script calls ``panic()``, ``evalWithCatch`` catches it.  Check with
``getException()``:

.. code-block:: cpp

   ctx.evalWithCatch(fnFail, nullptr);
   if (auto ex = ctx.getException()) {
       printf("Caught: %s\n", ex);
   }


Part B — High-level calling
===========================

``das_invoke_function`` (from ``daScript/simulate/aot.h``) is a family of
variadic templates that handle argument marshalling, AOT dispatch, and
return-value extraction in a single call.  **This is the recommended way
to call daslang functions from C++.**

Include the header:

.. code-block:: cpp

   #include "daScript/simulate/aot.h"


Obtaining a ``Func`` handle
----------------------------

The ``Func`` struct (``daScript/misc/arraytype.h``) is a lightweight wrapper
around ``SimFunction *``.  Convert a found ``SimFunction *`` into a ``Func``:

.. code-block:: cpp

   auto fnAdd = ctx.findFunction("add");
   Func add_func(fnAdd);

``Func`` is small and copyable — pass it by value.


Calling with ``invoke``
------------------------

For functions returning scalars or strings, use
``das_invoke_function<RetType>::invoke``:

.. code-block:: cpp

   int32_t result = das_invoke_function<int32_t>::invoke(&ctx, nullptr,
       add_func, 17, 25);
   // result == 42

   float sq = das_invoke_function<float>::invoke(&ctx, nullptr,
       square_func, 3.5f);
   // sq == 12.25

   char * greeting = das_invoke_function<char *>::invoke(&ctx, nullptr,
       greet_func, "World");
   // greeting == "Hello, World!"

The second argument (``LineInfo *``) can be ``nullptr`` in most cases.
It is used for error reporting and stack traces.


Calling with ``invoke_cmres``
------------------------------

When a daslang function returns a struct, use ``invoke_cmres`` instead:

.. code-block:: cpp

   Vec2 v = das_invoke_function<Vec2>::invoke_cmres(&ctx, nullptr,
       make_vec2_func, 1.5f, 2.5f);
   // v.x == 1.5, v.y == 2.5

This allocates the result buffer on the stack and passes it as the
caller-managed result pointer.


Calling by name
----------------

``das_invoke_function_by_name`` looks up the function by name at call time.
It validates that exactly one function matches and that it is not private:

.. code-block:: cpp

   int32_t result = das_invoke_function_by_name<int32_t>::invoke(
       &ctx, nullptr, "add", 100, 200);
   // result == 300

This is convenient for one-off calls but slower than using a ``Func``
handle.  For functions called repeatedly, cache the ``Func`` instead.


Part A vs Part B comparison
============================

==============================  ==============================  ==============================
Aspect                          Part A (cast + evalWithCatch)   Part B (das_invoke_function)
==============================  ==============================  ==============================
Header                          ``daScript.h``                  ``simulate/aot.h``
Argument marshalling            Manual ``vec4f[]``              Automatic (variadic templates)
Return value extraction         Manual ``cast<T>::to``          Automatic (template parameter)
Struct returns                  ``evalWithCatch(f, a, &buf)``   ``invoke_cmres(...)``
AOT dispatch                    No                              Yes
Exception handling              ``getException()`` after call   Throws C++ exception
Best for                        Learning / edge cases           Production code
==============================  ==============================  ==============================


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_02
   bin\Release\integration_cpp_02.exe

Expected output::

   === Part A: Low-level (cast + evalWithCatch) ===
     add(17, 25) = 42
     square(3.5) = 12.25
     greet("World") = "Hello, World!"
   int=42 float=3.14 string=test bool=true
     make_vec2(1.5, 2.5) = { x=1.5, y=2.5 }
     Caught expected exception: something went wrong!

   === Part B: High-level (das_invoke_function) ===
     add(17, 25) = 42
     square(3.5) = 12.25
     greet("World") = "Hello, World!"
   int=42 float=3.14 string=test bool=true
     make_vec2(1.5, 2.5) = { x=1.5, y=2.5 }
     add(100, 200) by name = 300


.. seealso::

   Full source:
   :download:`02_calling_functions.cpp <../../../../tutorials/integration/cpp/02_calling_functions.cpp>`,
   :download:`02_calling_functions.das <../../../../tutorials/integration/cpp/02_calling_functions.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_hello_world`

   Next tutorial: :ref:`tutorial_integration_cpp_binding_functions`

   C API equivalent: :ref:`tutorial_integration_c_calling_functions`
