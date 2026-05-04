.. _tutorial_integration_cpp_coroutines:

.. index::
   single: Tutorial; C++ Integration; Coroutines

===================================
 C++ Integration: Coroutines
===================================

This tutorial shows how to consume a daslang **generator** (coroutine)
from C++.  A daslang function returns a ``generator<int>`` via
``return <-``.  The C++ host receives a ``Sequence`` iterator and steps
through it one value at a time.

Topics covered:

* ``Sequence`` — the C++ type that wraps a daslang generator
* ``evalWithCatch`` with a third ``&Sequence`` parameter to capture generators
* ``builtin_iterator_iterate`` — single-step the generator
* ``builtin_iterator_close`` — clean up generator resources


Prerequisites
=============

* Tutorial 01 completed (:ref:`tutorial_integration_cpp_hello_world`).
* Familiarity with daslang generators (``generator<T>`` and ``yield``).


The daslang side
=================

The script defines a function that returns a generator of integers.
Each ``yield`` pauses execution and passes a value to the host:

.. code-block:: das

   options gen2

   [export]
   def test {
       return <- generator<int>() <| $() {
           for (i in range(5)) {
               print("  [das] yielding {i}\n")
               yield i
           }
           print("  [das] generator finished\n")
           return false
       }
   }

The function creates the generator with ``generator<int>() <| $()``,
fills it with a loop, and transfers ownership to the caller with
``return <-``.


Consuming a generator from C++
================================

After compiling and simulating the script, we find the ``test`` function
and capture its returned generator into a ``Sequence``:

.. code-block:: cpp

   Sequence it;
   ctx.evalWithCatch(fnTest, nullptr, &it);

When the third argument of ``evalWithCatch`` is a pointer to ``Sequence``,
the runtime fills it with the generator returned by the function.


Stepping through values
==========================

``builtin_iterator_iterate`` advances the generator to its next ``yield``
and writes the yielded value into a caller-provided buffer:

.. code-block:: cpp

   int32_t value = 0;
   int step = 0;
   while (builtin_iterator_iterate(it, &value, &ctx)) {
       tout << "  [c++] step " << step << " => value " << value << "\n";
       step++;
   }

Each call resumes the daslang generator, which runs until the next
``yield`` (or returns ``false`` to finish).


Cleanup
=======

Always call ``builtin_iterator_close`` when done — even if the generator
has already finished:

.. code-block:: cpp

   builtin_iterator_close(it, &value, &ctx);

This releases any resources held by the ``Sequence``.  If you break out
of the iteration early (before the generator returns ``false``), this
call is essential.


Build & run
===========

.. code-block:: bash

   cmake --build build --config Release --target integration_cpp_17
   bin/Release/integration_cpp_17

Expected output::

     [das] yielding 0
     [c++] step 0 => value 0
     [das] yielding 1
     [c++] step 1 => value 1
     [das] yielding 2
     [c++] step 2 => value 2
     [das] yielding 3
     [c++] step 3 => value 3
     [das] yielding 4
     [c++] step 4 => value 4
     [das] generator finished
   Generator produced 5 values total

The output is interleaved: each ``yield`` in daslang produces a
``[das]`` line, and the C++ iteration loop produces a ``[c++]`` line.


.. seealso::

   Full source:
   :download:`17_coroutines.cpp <../../../../tutorials/integration/cpp/17_coroutines.cpp>`,
   :download:`17_coroutines.das <../../../../tutorials/integration/cpp/17_coroutines.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_sandbox`

   Next tutorial: :ref:`tutorial_integration_cpp_dynamic_scripts`

   Related: :ref:`generators`
