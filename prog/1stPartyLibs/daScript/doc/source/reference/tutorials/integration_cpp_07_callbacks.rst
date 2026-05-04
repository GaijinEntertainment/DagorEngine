.. _tutorial_integration_cpp_callbacks:

.. index::
   single: Tutorial; C++ Integration; Callbacks

============================================
 C++ Integration: Callbacks
============================================

This tutorial shows how C++ code can receive and invoke daslang
closures.  Topics covered:

* ``TBlock<Ret, Args...>`` — typed block parameters (stack-bound closures)
* ``das_invoke<Ret>::invoke()`` — invoking a block from C++
* ``Func`` — function pointer parameters
* ``das_invoke_function<Ret>::invoke()`` — invoking a function pointer
* Lambda overview — ``TLambda<>``, ``das_invoke_lambda<>``
* Practical patterns: iteration, reduction


Prerequisites
=============

* Tutorial 06 completed (:ref:`tutorial_integration_cpp_interop`).
* Familiarity with ``addExtern`` and ``SideEffects``.


Blocks — ``TBlock<Ret, Args...>``
===================================

Blocks are stack-bound closures — the primary callback mechanism in
daslang.  They are only valid **during the C++ call** that receives them.
Never store a block for later use.

The template ``TBlock<ReturnType, ArgType1, ArgType2, ...>`` provides
type safety: daslang's compiler checks the argument types at compile
time.

.. code-block:: cpp

   void with_values(int32_t a, int32_t b,
                    const TBlock<void, int32_t, int32_t> & blk,
                    Context * context, LineInfoArg * at) {
       das_invoke<void>::invoke(context, at, blk, a, b);
   }

A predicate block returning ``bool``:

.. code-block:: cpp

   int32_t count_matching(const TArray<int32_t> & arr,
                          const TBlock<bool, int32_t> & pred,
                          Context * context, LineInfoArg * at) {
       int32_t count = 0;
       for (uint32_t i = 0; i < arr.size; ++i) {
           if (das_invoke<bool>::invoke(context, at, pred, arr[i])) {
               count++;
           }
       }
       return count;
   }

Registration uses ``SideEffects::invoke`` because the function invokes
script code:

.. code-block:: cpp

   addExtern<DAS_BIND_FUN(count_matching)>(*this, lib, "count_matching",
       SideEffects::invoke, "count_matching")
           ->args({"arr", "pred", "context", "at"});


Function pointers — ``Func``
===============================

``Func`` is a reference to a daslang-side function.  Unlike blocks,
function pointers can be stored and invoked multiple times (within the
same context).  In daslang, ``@@function_name`` creates a ``Func``
value.

.. code-block:: cpp

   void call_function_twice(Func fn, int32_t value,
                            Context * context, LineInfoArg * at) {
       das_invoke_function<void>::invoke(context, at, fn, value);
       das_invoke_function<void>::invoke(context, at, fn, value * 2);
   }


Lambdas — ``Lambda`` / ``TLambda<>``
=======================================

Lambdas are heap-allocated closures that can capture variables.
``TLambda<Ret, Args...>`` is the typed variant (analogous to ``TBlock``
and ``TFunc``).  Invoke with ``das_invoke_lambda<Ret>::invoke()``.

.. note::

   The untyped ``Lambda`` maps to ``lambda<>`` in daslang and will not
   match typed lambdas like ``lambda<(x:int):int>``.  Use
   ``TLambda<Ret, Args...>`` for type-safe lambda acceptance.


Practical pattern: C++ iterates, script processes
====================================================

A common embedding pattern is having C++ own the iteration/generation
logic while the script provides the processing callback:

.. code-block:: cpp

   void for_each_fibonacci(int32_t count,
                           const TBlock<void, int32_t, int32_t> & blk,
                           Context * context, LineInfoArg * at) {
       int32_t a = 0, b = 1;
       for (int32_t i = 0; i < count; ++i) {
           das_invoke<void>::invoke(context, at, blk, i, a);
           int32_t next = a + b;
           a = b;
           b = next;
       }
   }

And the accumulator/reduce pattern:

.. code-block:: cpp

   int32_t reduce_range(int32_t from, int32_t to, int32_t init,
                        const TBlock<int32_t, int32_t, int32_t> & blk,
                        Context * context, LineInfoArg * at) {
       int32_t acc = init;
       for (int32_t i = from; i < to; ++i) {
           acc = das_invoke<int32_t>::invoke(context, at, blk, acc, i);
       }
       return acc;
   }


Calling from daslang
=======================

Blocks use the ``<|`` pipe syntax with ``$()`` lambda prefix.
Function pointers use ``@@function_name``:

.. code-block:: das

   options gen2
   require tutorial_07_cpp

   def print_value(x : int) {
       print("fn({x})\n")
   }

   [export]
   def test() {
       // Block callback
       with_values(10, 20) $(a, b) {
           print("a + b = {a + b}\n")
       }

       // Function pointer
       call_function_twice(@@print_value, 5)

       // Fibonacci iteration
       for_each_fibonacci(5) $(index, value) {
           print("fib({index}) = {value}\n")
       }

       // Reduce — sum 1..10
       let total = reduce_range(1, 11, 0) $(acc, i) {
           return acc + i
       }
       print("sum = {total}\n")

       // Reduce — factorial 10
       let fact = reduce_range(1, 11, 1) $(acc, i) {
           return acc * i
       }
       print("10! = {fact}\n")


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_07
   bin\Release\integration_cpp_07.exe

Expected output::

   === Block: with_values ===
     a + b = 30

   === Block: count_matching ===
     even numbers: 5

   === Function pointer: call_function_twice ===
     fn(5)
     fn(10)

   === Practical: for_each_fibonacci ===
     fib(0) = 0
     fib(1) = 1
     fib(2) = 1
     fib(3) = 2
     fib(4) = 3
     fib(5) = 5
     fib(6) = 8
     fib(7) = 13

   === Practical: reduce_range (sum 1..10) ===
     sum = 55

   === Practical: reduce_range (factorial 10) ===
     10! = 3628800


.. seealso::

   Full source:
   :download:`07_callbacks.cpp <../../../../tutorials/integration/cpp/07_callbacks.cpp>`,
   :download:`07_callbacks.das <../../../../tutorials/integration/cpp/07_callbacks.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_interop`

   Next tutorial: :ref:`tutorial_integration_cpp_methods`
