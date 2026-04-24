.. _tutorial_apply_in_context:

=============================================
Cross-Context Services with apply_in_context
=============================================

.. index::
    single: Tutorial; apply_in_context
    single: Tutorial; Cross-Context Services
    single: Tutorial; Named Context
    single: Tutorial; Shared State
    single: Tutorial; Thread-Local Debug Agent

This tutorial covers ``[apply_in_context]`` — a macro annotation that
rewrites functions so their bodies execute in a named debug agent
context, while callers invoke them with normal function syntax.

Prerequisites: :ref:`tutorial_debug_agents` (Tutorial 45) for
``fork_debug_agent_context`` and ``install_new_debug_agent``.

.. code-block:: das

    options gen2

    require daslib/apply_in_context
    require daslib/debugger


Setting up a named context
===========================

First, create a debug agent context to host shared state.
A plain ``DapiDebugAgent`` with no overrides is sufficient —
the agent exists solely to own a named context:

.. code-block:: das

    var counter : int = 0

    def install_service(ctx : Context) {
        install_new_debug_agent(new DapiDebugAgent(), "counter_service")
    }

    def init_counter_service() {
        if (!has_debug_agent_context("counter_service")) {
            fork_debug_agent_context(@@install_service)
        }
    }


The [apply_in_context] annotation
==================================

Functions annotated with ``[apply_in_context(agent_name)]`` have
their body rewritten to execute in the named agent context.
From the caller's perspective, these look like normal functions:

.. code-block:: das

    [apply_in_context(counter_service)]
    def increment() : int {
        counter++
        return counter
    }

    [apply_in_context(counter_service)]
    def get_counter() : int {
        return counter
    }

    [apply_in_context(counter_service)]
    def add_to_counter(amount : int) {
        counter += amount
    }

    init_counter_service()
    print("  increment() = {increment()}\n")
    print("  increment() = {increment()}\n")
    print("  get_counter() = {get_counter()}\n")
    add_to_counter(10)
    print("  after add = {get_counter()}\n")
    print("  local counter = {counter}\n")
    // output:
    //   increment() = 1
    //   increment() = 2
    //   get_counter() = 2
    //   after add = 12
    //   local counter = 0

The agent context's ``counter`` is modified — the caller's local
copy stays at zero.


Argument constraints
=====================

Arguments that cross context boundaries must use types that
can be safely marshalled.  Reference-type arguments must be
marked ``implicit``:

- ``string implicit`` — strings are reference types in daslang
- ``var x : int& implicit`` — explicit reference parameters

Value types (``int``, ``float``, ``bool``) work without annotation:

.. code-block:: das

    [apply_in_context(counter_service)]
    def set_counter_name(name : string implicit) {
        print("  counter named '{name}', value = {counter}\n")
    }

    [apply_in_context(counter_service)]
    def read_counter(var result : int& implicit) {
        result = counter
    }

    set_counter_name("my_counter")
    // output:
    //   counter named 'my_counter', value = 12

    var val = 0
    read_counter(val)
    print("  read_counter() -> val = {val}\n")
    // output:
    //   read_counter() -> val = 12


A cache service
================

A practical use case: a shared cache backed by a table that lives
in the agent context.  Any module or context can call put / get / has
without worrying about which context they're in:

.. code-block:: das

    var cache : table<string; int>

    def install_cache(ctx : Context) {
        install_new_debug_agent(new DapiDebugAgent(), "my_cache")
    }

    def init_cache_service() {
        if (!has_debug_agent_context("my_cache")) {
            fork_debug_agent_context(@@install_cache)
        }
    }

    [apply_in_context(my_cache)]
    def cache_put(key : string implicit; value : int) {
        cache |> insert(key, value)
    }

    [apply_in_context(my_cache)]
    def cache_get(key : string implicit) : int {
        return cache?[key] ?? -1
    }

    [apply_in_context(my_cache)]
    def cache_has(key : string implicit) : bool {
        return key_exists(cache, key)
    }

    [apply_in_context(my_cache)]
    def cache_size() : int {
        return length(cache)
    }

    init_cache_service()
    cache_put("width", 1920)
    cache_put("height", 1080)
    print("  cache size = {cache_size()}\n")
    print("  width = {cache_get("width")}\n")
    print("  local cache length = {length(cache)}\n")
    // output:
    //   cache size = 2
    //   width = 1920
    //   local cache length = 0


Thread-local agents vs named agents
======================================

``[apply_in_context]`` requires a **named** agent context.  For
modules that do not need a public name (e.g., the profiler), the
**thread-local** agent is preferred.  There can be only **one**
thread-local agent per thread — that is why it needs no name.
It is installed with ``install_new_thread_local_debug_agent`` and
communicated with via ``invoke_debug_agent_method("", ...)``.

The thread-local path is faster because it skips the global agent
map lookup entirely.

Choose based on your use case:

- **Named agent + [apply_in_context]** — best for shared services
  (caches, registries) that multiple modules discover by name.
  There can be many named agents simultaneously.
- **Thread-local agent + invoke_debug_agent_method("", ...)** — best
  for a single performance-critical module (profiler, logger).
  Only one per thread; fastest dispatch.

See :ref:`tutorial_debug_agents` (Section: Thread-local debug agents)
for examples.


How it works under the hood
============================

The ``[apply_in_context]`` macro rewrites each function into
three parts:

1. **Caller function** (keeps original name) — verifies the
   agent exists, creates a result variable (if needed), then
   calls ``invoke_in_context`` to dispatch into the agent context.

2. **CONTEXT\`func_name** (runs in the agent context) —
   verifies it's in the correct context, then calls the clone.

3. **CONTEXT_CLONE\`func_name** (the original body) —
   contains the implementation code.

For a function with a return value, the expansion is similar to:

.. code-block:: das

    def get_counter() : int {
        verify(has_debug_agent_context("counter_service"))
        var __res__ : int
        unsafe {
            invoke_in_context(
                get_debug_agent_context("counter_service"),
                "counter_service`get_counter",
                addr(__res__)
            )
        }
        return __res__
    }

The annotation adds ``[pinvoke]`` to the generated context
function automatically, enabling the context mutex required
for cross-context calls.


Quick reference
===============

====================================================  ===================================================
``[apply_in_context(name)]``                          Rewrite function to execute in named agent context
``string implicit``                                   Required for string arguments crossing contexts
``var x : T& implicit``                               Required for reference arguments crossing contexts
``require daslib/apply_in_context``                   Import the annotation macro
``fork_debug_agent_context`` + ``install_new_...``    Set up the named context (see Tutorial 45)
====================================================  ===================================================


.. seealso::

   Full source: :download:`tutorials/language/46_apply_in_context.das <../../../../tutorials/language/46_apply_in_context.das>`

   :ref:`tutorial_debug_agents` — creating and installing debug
   agents (Tutorial 45).

   :ref:`tutorial_compile_and_run` — ``invoke_in_context`` basics
   (Tutorial 44).

   :ref:`Contexts <contexts>` — language reference for context semantics.

   Previous tutorial: :ref:`tutorial_debug_agents`

   Next tutorial: :ref:`tutorial_data_walker`
