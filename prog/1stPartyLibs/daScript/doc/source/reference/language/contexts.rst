.. _contexts:

=======
Context
=======

.. index::
    single: contexts

Daslang environments are organized into contexts. Compiling a Daslang program produces a ``Program`` object, which can then be simulated into a ``Context``.

``Context`` consists of
    * name and flags
    * functions code
    * global variables data
    * shared global variable data
    * stack
    * dynamic memory heap
    * dynamic string heap
    * constant string heap
    * runtime debug information
    * locks
    * miscellaneous lookup infrastructure

In some sense, a ``Context`` can be viewed as a Daslang virtual machine — the object responsible for executing code and maintaining state.
It can also be viewed as an instance of a class whose methods can be accessed when marked with ``[export]``.

Function code, the constant string heap, runtime debug information, and shared global variables are shared between cloned contexts.
This allows each context instance to maintain a relatively small memory profile.

The stack can optionally be shared between multiple contexts of different types, keeping the memory profile even smaller.

Initialization and shutdown
---------------------------

Throughout its lifetime, a ``Context`` goes through initialization and shutdown phases.
Context initialization is implemented in ``Context::runInitScript``, and shutdown is implemented in ``Context::runShutdownScript``.
These functions are called automatically when a ``Context`` is created, cloned, or destroyed.
Depending on the application and the ``CodeOfPolicies``, they may also be called when ``Context::restart`` or ``Context::restartHeaps`` is called.

It is initialized in the following order:
    1. All global variables are initialized in the order they are declared, per module.
    2. All functions tagged with ``[init]`` are called in the order they are declared, per module, except for specifically ordered ones.
    3. All specifically ordered functions tagged with ``[init]`` are called in the order they appear after topological sort.

The topological sort order for the init functions is specified in the init annotation.
    * ``tag`` attribute specifies that function will appear during the specified pass
    * ``before`` attribute specifies that function will appear before the specified pass
    * ``after`` attribute specifies that function will appear after the specified pass

Consider the following example:

.. code-block:: das

    [init(before="middle")]
    def a {
        order |> push("a")
    }
    [init(tag="middle")]
    def b {
        order |> push("b")
    }
    [init(tag="middle")]
    def c {
        order |> push("c")
    }
    [init(after="middle")]
    def d {
        order |> push("d")
    }

The functions will execute in the following order:
    1. d
    2. b or c, in any order
    3. a

During shutdown, the context runs all functions marked with ``[finalize]`` in the order they are declared, per module.

Macro contexts
--------------

For each module that contains macros, an individual context is created and initialized.
In addition to regular functions, functions tagged with ``[macro]`` or ``[_macro]`` are called during initialization.

Functions tagged with ``[macro_function]`` are excluded from the regular context and only appear in macro contexts.

Unless a macro module is marked as shared, it will be shut down after compilation.
Shared macro modules are initialized during their first compilation, and are shut down during the environment shutdown.

Locking
-------

A context contains a ``recursive_mutex`` and can be locked and unlocked with the ``lock_context`` or ``lock_this_context`` RAII blocks.
Cross-context calls via ``invoke_in_context`` automatically lock the target context.

Lookups
-------

Global variables and functions can be looked up by name or by mangled name hash on both Daslang and C++ side.

Memory allocation and garbage collection
----------------------------------------

Memory allocation strategies for both the string heap and the regular heap are specified in the ``CodeOfPolicies`` and options.

To allow garbage collection from inside the context, the following options are necessary:

.. code-block:: das

    options persistent_heap // this one enables garbage-collectable heap
    options gc              // this one enables garbage collection for the variables on the stack

To collect garbage, from the inside of the context:

.. code-block:: das

    var collect_string_heap = true
    var validate_after_collect = false
    heap_collect(collect_string_heap, validate_after_collect)

To do the same from the C++ side:

.. code-block:: cpp

    context->collectHeap(dummy_line_info_ptr, collect_string_heap, validate_after_collect);

.. seealso::

    :ref:`Annotations <annotations>` for ``[init]``, ``[finalize]``, and ``[export]`` annotations,
    :ref:`Program structure <program_structure>` for the overall compilation and initialization lifecycle,
    :ref:`Locks <locks>` for context and container locking details,
    :ref:`Options <options>` for ``persistent_heap``, ``gc``, and memory allocation policies,
    :ref:`Macros <macros>` for macro context initialization.


