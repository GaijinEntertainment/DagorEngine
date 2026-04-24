.. _tutorial_compile_and_run:

==========================================
Compiling and Running Programs at Runtime
==========================================

.. index::
    single: Tutorial; Compile and Run
    single: Tutorial; Runtime Compilation
    single: Tutorial; invoke_in_context
    single: Tutorial; Multiple Contexts
    single: Tutorial; Virtual File System

This tutorial covers **compiling and running daslang programs from
daslang** — dynamically compiling source code at runtime, simulating
it into a runnable context, and invoking exported functions across
context boundaries.

Prerequisites: :ref:`tutorial_jobque` (Tutorial 35) for the channel
section.

.. code-block:: das

    options gen2
    options multiple_contexts   // required when holding smart_ptr<Context>

    require daslib/rtti
    require daslib/debugger
    require daslib/jobque_boost


Compile from string
===================

The simplest way to compile daslang at runtime is ``compile``, which
takes a module name, source text, and ``CodeOfPolicies``.  The callback
receives ``(ok : bool, program : smart_ptr<Program>, issues : string)``.

Always set ``cop.threadlock_context = true`` — this is required for
``invoke_in_context`` to work:

.. code-block:: das

    using <| $(var cop : CodeOfPolicies) {
        cop.threadlock_context = true
        compile("inline", src, cop) <| $(ok, program, issues) {
            if (!ok) {
                print("compile error: {issues}\n")
                return
            }
            simulate(program) <| $(sok; context; serrors) {
                if (!sok) {
                    print("simulate error: {serrors}\n")
                    return
                }
                unsafe {
                    invoke_in_context(context, "hello")
                }
            }
        }
    }
    // output:
    //   hello from compiled code!


Compile from file
=================

``compile_file`` compiles a ``.das`` file from disk.  It requires a
``FileAccess`` (for file I/O) and a ``ModuleGroup`` (for module
resolution).  Both must stay alive during compile + simulate:

.. code-block:: das

    var inscope access <- make_file_access("")
    using <| $(var mg : ModuleGroup) {
        using <| $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file("tutorials/language/44_helper.das", access,
                         unsafe(addr(mg)), cop) <| $(ok, program, issues) {
                if (!ok) {
                    print("compile error: {issues}\n")
                    return
                }
                simulate(program) <| $(sok; context; serrors) {
                    if (!sok) {
                        print("simulate error: {serrors}\n")
                        return
                    }
                    unsafe {
                        invoke_in_context(context, "main")
                    }
                }
            }
        }
    }
    // output:
    //   hello from 44_helper!


Virtual file system
===================

You can compile code that does not exist on disk by injecting virtual
files into the ``FileAccess`` object with ``set_file_source``.  This is
useful for code generation, REPLs, and eval-like tools:

.. code-block:: das

    var inscope access <- make_file_access("")
    access |> set_file_source("__generated.das", generated_code)

    // Now compile_file("__generated.das", access, ...) will find it.
    // Other modules on disk remain accessible through the same access.


Invoke with arguments
=====================

``invoke_in_context`` can pass up to 10 arguments by value.  It always
returns ``void`` — see later sections for how to retrieve results.

Use ``has_function`` to check whether a function exists before calling it:

.. code-block:: das

    if (has_function(*context, "sum3")) {
        unsafe {
            invoke_in_context(context, "sum3", 10, 20, 30)
        }
    }
    // output:
    //   sum3 = 60

Note: ``has_function`` takes a ``Context`` reference, so you must
dereference the smart pointer with ``*context``.


Reading results via global variables
=====================================

Since ``invoke_in_context`` returns void, one way to get a result is to
have the child store it in a global variable, then read it back with
``get_context_global_variable``.

The pointer returned is into the child context's memory — copy the value
immediately before the context goes out of scope:

.. code-block:: das

    // Call compute(7) — sets global `result` to 7*7+1 = 50
    unsafe {
        invoke_in_context(context, "compute", 7)
    }
    // Read back the global variable "result"
    let ptr = unsafe(get_context_global_variable(context, "result"))
    if (ptr != null) {
        let value = *unsafe(reinterpret<int?> ptr)
        print("result = {value}\n")
    }
    // output:
    //   result = 50


Passing results via pointer argument
=====================================

Another pattern is to pass a pointer from the host into the child.  The
child writes through the pointer, and the host reads it after
``invoke_in_context`` returns:

.. code-block:: das

    // Child function: def store_via_ptr(val : int; var dst : int?)
    var output = 0
    unsafe {
        invoke_in_context(context, "store_via_ptr", 5, addr(output))
    }
    print("output = {output}\n")    // 5 * 10 = 50


Using channels for cross-context results
=========================================

Channels (from ``daslib/jobque_boost``) are cross-context safe — ideal
for collecting structured results from child contexts.  The host creates
a channel with ``with_channel(1)``, passes it as a ``Channel?`` argument
to the child via ``invoke_in_context``, and the child pushes results with
``push_clone`` + ``notify``.  The host drains the channel with
``for_each_clone``.  Data arrives as a temporary type (``TT#``) ensuring
proper copy from the foreign heap.

Use ``notify``, **not** ``notify_and_release``.  When a lambda captures a
channel, its reference count is incremented, so ``notify_and_release``
releases that extra reference and nulls the variable.  With
``invoke_in_context`` there is no lambda — the child does not own the
channel and no extra reference was added — so plain ``notify`` is correct.

The child script needs ``require daslib/jobque_boost`` to use channel
operations.  Use ``compile_file`` with ``make_file_access("")`` so the
child can resolve daslib modules from disk:

.. code-block:: das

    struct IntResult {
        value : int
    }

    // Child script receives Channel?, pushes result, notifies
    let child_src = "..."   // defines produce(var ch : Channel?)

    var inscope access <- make_file_access("")
    access |> set_file_source("__child.das", child_src)
    // ... compile_file + simulate ...
    with_channel(1) $(ch) {
        unsafe {
            invoke_in_context(context, "produce", ch)
        }
        ch |> for_each_clone() $(val : IntResult#) {
            print("channel received: {val.value}\n")
        }
    }


Error handling
==============

Compilation and simulation can fail.  Always check the ``ok`` / ``sok``
flags.  Runtime errors in the child context can be caught with
``try``/``recover``:

.. code-block:: das

    // 1) Compilation error
    compile("bad", bad_src, cop) <| $(ok, program, issues) {
        if (!ok) {
            print("compile error: {issues}\n")
            return
        }
    }

    // 2) Runtime error
    try {
        unsafe {
            invoke_in_context(context, "crash")
        }
    } recover {
        print("runtime error caught\n")
    }


Quick reference
===============

==========================================  ================================================
``compile(name, src, cop) <| ...``          Compile from source string
``compile_file(name, access, mg, cop)``     Compile from file via FileAccess + ModuleGroup
``simulate(program) <| ...``                Simulate a program into a Context
``invoke_in_context(ctx, name, ...)``       Call an [export] function (returns void)
``has_function(*ctx, name)``                Check if function exists in context
``get_context_global_variable(ctx, n)``     Read global variable pointer from context
``make_file_access("")``                    Create disk-backed FileAccess
``set_file_source(access, name, src)``      Inject a virtual file
``with_channel(N) $(ch) { ... }``           Create channel; pass ``ch`` to child context
``push_clone`` / ``notify``                 Child pushes results, notifies host
``for_each_clone``                          Host drains channel results as ``TT#``
``options multiple_contexts``               Required when holding smart_ptr<Context>
``cop.threadlock_context = true``           Required for invoke_in_context
==========================================  ================================================


.. seealso::

   :ref:`Job Queue <tutorial_jobque>` — channels, lock boxes, and threading
   (Tutorial 35).

   :ref:`Contexts <contexts>` — language reference for context semantics.

   Full source: :download:`tutorials/language/44_compile_and_run.das <../../../../tutorials/language/44_compile_and_run.das>`

   Helper file: :download:`tutorials/language/44_helper.das <../../../../tutorials/language/44_helper.das>`

   Previous tutorial: :ref:`tutorial_interfaces`

   Next tutorial: :ref:`tutorial_debug_agents`
