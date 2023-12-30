.. contexts:

*******
Context
*******

.. index::
    single: contexts

Daslang environments are organized into contexts. Compiling Daslang program produces the 'Program' object, which can then be simulated into the 'Context'.

`Context` consists of
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

In some sense `Context` can be viewed as Daslang virtual machine. It is the object that is responsible for executing the code and keeping the state.
It can also be viewed as an instance of the class, which methods can be accessed when marked as [export].

Function code, constant string heap, runtime debug information, and shared global variables are shared between cloned contexts.
That allows to keep relatively small profile for the context instance.

Stack can be optionally shared between multiple contexts of different type, to keep memory profile even smaller.

===========================
Initialization and shutdown
===========================

Through its lifetime `Context` goes through the initialization and the shutdown.
Context initialization is implemented in `Context::runInitScript` and shutdown is implemented in `Context::runShutdownScript`.
These functions are called automatically when `Context` is created, cloned, and destroyed.
Depending on the user application and the `CodeOfPolicies`, they may also be called when `Context::restart`  or `Context::restartHeaps` is called.

Its initialized in the following order.
    1. All global variables are initialized in order they are declared per module.
    2. All functions tagged as [init] are called in order they are declared per module, except for specifically ordered ones.
    3. All specifically ordered functions tagged as [init] are called in order they appear after the topological sort.

The topological sort order for the init functions is specified in the init annotation.
    * `tag` attribute specifies that function will appear during the specified pass
    * `before` attribute specifies that function will appear before the specified pass
    * `after` attribute specifies that function will appear after the specified pass

Consider the following example::

    [init(before="middle")]
    def a
        order |> push("a")
    [init(tag="middle")]
    def b
        order |> push("b")
    [init(tag="middle")]
    def c
        order |> push("c")
    [init(after="middle")]
    def d
        order |> push("d")

Functions will appear in the order of
    1. d
    2. b or c, in any order
    3. a

Context shuts down runs all functions marked as [finalize] in the order they are declared per module.

==============
Macro contexts
==============

For each module which contains macros individual context is created and initialized.
On top of regular functions, functions tagged as [macro] or [_macro] are called during the initialization.

Functions tagged as [macro_function] are excluded from the regular context, and only appear in the macro contexts.

Unless macro module is marked as shared, it will be shutdown after the compilation.
Shared macro modules are initialized during their first compilation, and are shut down during the environment shutdown.

=======
Locking
=======

Context contains `recursive_mutex`, and can be specifically locked and unlocked with the `lock_context` or `lock_this_context` RAII block.
Cross context calls `invoke_in_context` automatically lock the target context.

=======
Lookups
=======

Global variables and functions can be looked up by name or by mangled name hash on both Daslang and C++ side.

