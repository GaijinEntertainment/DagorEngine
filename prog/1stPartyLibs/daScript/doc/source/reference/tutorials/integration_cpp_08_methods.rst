.. _tutorial_integration_cpp_methods:

.. index::
   single: Tutorial; C++ Integration; Binding Methods

============================================
 C++ Integration: Binding Methods
============================================

This tutorial shows how to expose C++ member functions to daslang.
Topics covered:

* ``DAS_CALL_MEMBER`` — wrapping a member function as a static callable
* ``DAS_CALL_METHOD`` — using the wrapper as an ``addExtern`` template argument
* ``DAS_CALL_MEMBER_CPP`` — providing the AOT-compatible name
* Const vs non-const methods and ``SideEffects``
* Pipe syntax for method-like calls in daslang


Prerequisites
=============

* Tutorial 07 completed (:ref:`tutorial_integration_cpp_callbacks`).
* Familiarity with ``addExtern``, ``ManagedStructureAnnotation``, and
  ``MAKE_TYPE_FACTORY``.


How methods work in daslang
==============================

daslang does not have member functions.  "Methods" are free functions
whose first parameter is the struct instance (``self``).  Pipe syntax
(``obj |> method()``) provides method-call ergonomics.


``DAS_CALL_MEMBER``
=====================

This macro generates a static wrapper function that forwards to the
C++ member function.  Create a ``using`` alias for each method:

.. code-block:: cpp

   // Step 1: Create wrapper aliases
   using method_increment = DAS_CALL_MEMBER(Counter::increment);
   using method_get       = DAS_CALL_MEMBER(Counter::get);
   using method_add       = DAS_CALL_MEMBER(Counter::add);

The wrapper's ``invoke`` method has the signature
``invoke(Counter &, args...)`` — the first argument is the object.


``DAS_CALL_METHOD`` and ``DAS_CALL_MEMBER_CPP``
==================================================

Register the wrapper with ``addExtern``:

.. code-block:: cpp

   // Step 2: Register methods
   addExtern<DAS_CALL_METHOD(method_increment)>(*this, lib, "increment",
       SideEffects::modifyArgument,
       DAS_CALL_MEMBER_CPP(Counter::increment))
           ->args({"self"});

   addExtern<DAS_CALL_METHOD(method_add)>(*this, lib, "add",
       SideEffects::modifyArgument,
       DAS_CALL_MEMBER_CPP(Counter::add))
           ->args({"self", "amount"});

   addExtern<DAS_CALL_METHOD(method_get)>(*this, lib, "get",
       SideEffects::none,
       DAS_CALL_MEMBER_CPP(Counter::get))
           ->args({"self"});

Key points:

* **Non-const methods** (``increment``, ``add``, ``reset``) use
  ``SideEffects::modifyArgument`` because they mutate the object
* **Const methods** (``get``, ``isZero``) use ``SideEffects::none``
* ``DAS_CALL_MEMBER_CPP(Counter::method)`` provides the mangled name
  string for AOT compilation


Factory functions
==================

Since handled types require ``unsafe`` for mutable local variables,
provide factory functions that return by value so scripts can use
``let``:

.. code-block:: cpp

   Counter make_counter(int32_t initial, int32_t step) {
       Counter c;
       c.value = initial;
       c.step = step;
       return c;
   }

   addExtern<DAS_BIND_FUN(make_counter),
             SimNode_ExtFuncCallAndCopyOrMove>(
       *this, lib, "make_counter",
       SideEffects::none, "make_counter")
           ->args({"initial", "step"});


Calling methods from daslang
===============================

The bound methods are called as free functions.  Because handled types
need mutable access under ``unsafe``, the pattern is:

.. code-block:: das

   options gen2
   require tutorial_08_cpp

   [export]
   def test() {
       unsafe {
           var c = make_counter(0, 1)
           increment(c)
           increment(c)
           add(c, 10)
           print("value = {get(c)}\n")  // 12
       }

       // Pipe syntax — idiomatic daslang
       unsafe {
           var c = make_counter(100, 10)
           c |> decrement()
           c |> decrement()
           print("100 - 20 = {c |> get()}\n")  // 80
       }


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_08
   bin\Release\integration_cpp_08.exe

Expected output::

   === Counter ===
   initial: 0
   after 3 increments: 3
   after decrement: 2
   after add(10): 12
   is_positive: true
   is_zero: false
   after set_step(5) + increment: 17
   after reset: 0
   is_zero: true

   === Counter with pipe syntax ===
   100 - 3*10 = 70

   === StringBuffer ===
   empty: true
   content: Hello, World!
   length: 13
   after clear, empty: true
   content: daslang


.. seealso::

   Full source:
   :download:`08_methods.cpp <../../../../tutorials/integration/cpp/08_methods.cpp>`,
   :download:`08_methods.das <../../../../tutorials/integration/cpp/08_methods.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_callbacks`

   Next tutorial: :ref:`tutorial_integration_cpp_operators_and_properties`
