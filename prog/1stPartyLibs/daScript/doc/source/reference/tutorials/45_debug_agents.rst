.. _tutorial_debug_agents:

=============
Debug Agents
=============

.. index::
    single: Tutorial; Debug Agents
    single: Tutorial; DapiDebugAgent
    single: Tutorial; fork_debug_agent_context
    single: Tutorial; invoke_in_context
    single: Tutorial; invoke_debug_agent_method
    single: Tutorial; invoke_debug_agent_function
    single: Tutorial; delete_debug_agent_context
    single: Tutorial; onLog
    single: Tutorial; Named Context
    single: Tutorial; Thread-Local Debug Agent

This tutorial covers **debug agents** — persistent objects that live
in their own separate context and can intercept runtime events,
collect state, and host shared data accessible from any context.

Prerequisites: :ref:`tutorial_compile_and_run` (Tutorial 44) for
``invoke_in_context`` basics.

.. code-block:: das

    options gen2

    require daslib/debugger
    require daslib/rtti


Creating and installing a debug agent
======================================

A debug agent is a class that extends ``DapiDebugAgent``.  The
fundamental pattern is:

1. Define a class that extends ``DapiDebugAgent``
2. Write a setup function that creates the agent and installs
   it under a name with ``install_new_debug_agent``
3. Call ``fork_debug_agent_context(@@setup)`` to clone the
   current context and run the setup function in it

The agent lives in its own "agent context" — a separate copy
of the program that stays resident:

.. code-block:: das

    class CounterAgent : DapiDebugAgent {
        count : int = 0
    }

    def install_counter(ctx : Context) {
        install_new_debug_agent(new CounterAgent(), "counter")
    }

    def demo_create_agent() {
        print("  has 'counter' = {has_debug_agent_context("counter")}\n")
        fork_debug_agent_context(@@install_counter)
        print("  has 'counter' = {has_debug_agent_context("counter")}\n")
    // output:
    //   has 'counter' = false
    //   has 'counter' = true


Intercepting log output with onLog
====================================

``DapiDebugAgent`` has an ``onLog`` method called whenever any
context prints or logs.  If ``onLog`` returns ``true``, the default
output to stdout is suppressed.  If it returns ``false``, output
proceeds normally.

This is how profiling tools, IDE log panels, and custom loggers
intercept program output:

.. code-block:: das

    var log_intercept_count : int = 0

    class LogAgent : DapiDebugAgent {
        def override onLog(context : Context?; at : LineInfo const?;
                           level : int; text : string#) : bool {
            log_intercept_count++
            return false  // don't suppress — let output reach stdout
        }
    }

    def install_log_agent(ctx : Context) {
        install_new_debug_agent(new LogAgent(), "log_watcher")
    }

    [export, pinvoke]
    def read_log_count(var result : int?) {
        unsafe {
            *result = log_intercept_count
        }
    }

    // After forking + installing, each print/to_log triggers onLog
    fork_debug_agent_context(@@install_log_agent)
    print("  hello through agent\n")
    to_log(LOG_INFO, "  info message\n")

    var count = 0
    unsafe {
        invoke_in_context(get_debug_agent_context("log_watcher"),
            "read_log_count", addr(count))
    }
    print("  log_intercept_count >= 2: {count >= 2}\n")
    // output:
    //   hello through agent
    //   info message
    //   log_intercept_count >= 2: true


Calling functions in the agent context
=======================================

Use ``invoke_in_context`` to call ``[export, pinvoke]`` functions
in the agent context.  ``get_debug_agent_context(name)`` returns the
agent's ``Context``.  Functions run in that context, so they see the
agent's copy of module-level variables — not the caller's.

The ``[pinvoke]`` annotation is required — it enables the context
mutex needed for cross-context invocation.

To return values, pass a pointer to a result variable:

.. code-block:: das

    var agent_counter : int = 0

    [export, pinvoke]
    def agent_increment() {
        agent_counter++
    }

    [export, pinvoke]
    def agent_get(var result : int?) {
        unsafe {
            *result = agent_counter
        }
    }

    def demo_invoke_in_context() {
        unsafe {
            invoke_in_context(get_debug_agent_context("counter"), "agent_increment")
            invoke_in_context(get_debug_agent_context("counter"), "agent_increment")
            invoke_in_context(get_debug_agent_context("counter"), "agent_increment")
        }
        var result = 0
        unsafe {
            invoke_in_context(get_debug_agent_context("counter"), "agent_get", addr(result))
        }
        print("  agent_counter (in agent) = {result}\n")
        print("  agent_counter (local)    = {agent_counter}\n")
    // output:
    //   agent_counter (in agent) = 3
    //   agent_counter (local)    = 0


Calling agent methods with invoke_debug_agent_method
======================================================

``invoke_debug_agent_method`` calls a method on the agent's class
instance directly — no ``[export, pinvoke]`` helper functions needed.
The agent's ``self`` is passed automatically.

Syntax: ``invoke_debug_agent_method("agent_name", "method", args...)``

.. code-block:: das

    class CalcAgent : DapiDebugAgent {
        accumulator : int = 0
        def add(amount : int) {
            self.accumulator += amount
        }
        def get_result(var result : int?) {
            unsafe {
                *result = self.accumulator
            }
        }
    }

    def install_calc_agent(ctx : Context) {
        install_new_debug_agent(new CalcAgent(), "calc")
    }

    fork_debug_agent_context(@@install_calc_agent)

    unsafe {
        invoke_debug_agent_method("calc", "add", 10)
        invoke_debug_agent_method("calc", "add", 20)
        invoke_debug_agent_method("calc", "add", 12)
    }

    var result = 0
    unsafe {
        invoke_debug_agent_method("calc", "get_result", addr(result))
    }
    print("  accumulator = {result}\n")
    // output:
    //   accumulator = 42


State collection — onCollect and onVariable
============================================

``onCollect`` is called when ``collect_debug_agent_state`` is triggered.
The agent can report custom variables via ``report_context_state``.
``onVariable`` receives each reported variable — this is how IDE
debuggers show custom watch variables and application diagnostics:

.. code-block:: das

    class StateAgent : DapiDebugAgent {
        collection_count : int = 0
        def override onCollect(var ctx : Context; at : LineInfo) : void {
            collection_count++
            unsafe {
                let tinfo = typeinfo rtti_typeinfo(collection_count)
                report_context_state(ctx, "Diagnostics", "collection_count",
                    unsafe(addr(tinfo)), unsafe(addr(collection_count)))
            }
        def override onVariable(var ctx : Context; category, name : string;
                                info : TypeInfo; data : void?) : void {
            unsafe {
                let value = sprint_data(data, addr(info), print_flags.singleLine)
                print("  {category}: {name} = {value}\n")
            }
    }

    // Trigger collection
    collect_debug_agent_state(this_context(), get_line_info(1))
    // output:
    //   Diagnostics: collection_count = 1


Agent existence checks
=======================

``has_debug_agent_context(name)`` checks if a named agent exists.
Always check before accessing the context to avoid panics:

.. code-block:: das

    print("  has 'counter'  = {has_debug_agent_context("counter")}\n")
    print("  has 'missing'  = {has_debug_agent_context("missing")}\n")
    // output:
    //   has 'counter'  = true
    //   has 'missing'  = false


Auto-start module pattern
==========================

In modules, agents are installed automatically via a ``[_macro]``
function.  Four guards ensure safe, single installation:

.. code-block:: das

    [_macro]
    def private auto_start() {
        if (is_compiling_macros_in_module("my_module") && !is_in_completion()) {
            if (!is_in_debug_agent_creation()) {
                if (!has_debug_agent_context("my_agent")) {
                    fork_debug_agent_context(@@my_agent_setup)
                }
            }
        }
    }

The guards prevent:

- Running outside the module's own compilation
- Running during IDE code completion
- Recursive agent creation
- Duplicate installation


Plain agent as named context host
==================================

A common pattern is to create a plain ``DapiDebugAgent`` (no
overrides) just to own a named context.  Module-level variables
in that context become shared state accessible via
``invoke_in_context``.  This is the foundation of the
``[apply_in_context]`` pattern (Tutorial 46):

.. code-block:: das

    var shared_data : int = 0

    [export, pinvoke]
    def add_data(amount : int) {
        shared_data += amount
    }

    [export, pinvoke]
    def get_data(var result : int?) {
        unsafe {
            *result = shared_data
        }
    }

    def install_data_host(ctx : Context) {
        install_new_debug_agent(new DapiDebugAgent(), "data_host")
    }

    // Multiple calls accumulate in the agent's copy
    unsafe {
        invoke_in_context(get_debug_agent_context("data_host"), "add_data", 10)
        invoke_in_context(get_debug_agent_context("data_host"), "add_data", 20)
    }
    // output:
    //   shared_data (in agent) = 30
    //   shared_data (local)    = 0


Shutting down a debug agent
============================

``delete_debug_agent_context`` removes an agent by name.  It notifies all other
agents via ``onUninstall``, then safely destroys the agent and its context:

.. code-block:: das

    // Remove the agent
    delete_debug_agent_context("data_host")

    print("has 'data_host' = {has_debug_agent_context("data_host")}\n")
    // output: has 'data_host' = false

    // Deleting a non-existent agent is a safe no-op
    delete_debug_agent_context("data_host")

Use this when a profiling session ends, a debug tool is closed, or
during test teardown to ensure agents do not leak across test files.


Thread-local debug agents
==========================

A **thread-local** agent is installed with
``install_new_thread_local_debug_agent`` instead of
``install_new_debug_agent``.  There can be only **one** thread-local
agent per thread — that is why it needs no name: there is nothing
to distinguish it from.  Access it by passing an **empty string**
``""`` as the category to ``invoke_debug_agent_method`` or
``invoke_debug_agent_function``.

The thread-local agent exists for **speed** — dispatching through it
skips the global agent map lookup entirely.  The ``profiler`` module
uses this pattern: it installs itself as the thread-local agent and
communicates via ``invoke_debug_agent_method("", ...)``.

Advantages over named agents:

- Faster dispatch — no global map lookup
- Simpler API — no name needed (there is only one)

.. code-block:: das

    class ThreadLocalAgent : DapiDebugAgent {
        value : int = 0
        def set_value(val : int) {
            self.value = val
        }
        def get_value(var result : int?) {
            unsafe {
                *result = self.value
            }
        }
    }

    def install_thread_local_agent(ctx : Context) {
        install_new_thread_local_debug_agent(new ThreadLocalAgent())
    }

    fork_debug_agent_context(@@install_thread_local_agent)

    // Use "" to target the thread-local agent
    unsafe {
        invoke_debug_agent_method("", "set_value", 42)
    }

    var result = 0
    unsafe {
        invoke_debug_agent_method("", "get_value", addr(result))
    }
    print("  thread-local value = {result}\n")
    // output:
    //   thread-local value = 42


Quick reference
===============

=======================================================  =====================================================
``fork_debug_agent_context(@@fn)``                       Clone context and call setup function in clone
``install_new_debug_agent(agent, name)``                 Register agent under a global name
``install_new_thread_local_debug_agent(agent)``          Install agent on the current thread (no name)
``has_debug_agent_context(name)``                        Check if named agent exists
``get_debug_agent_context(name)``                        Get agent's Context for invoke_in_context
``delete_debug_agent_context(name)``                     Remove agent by name (safe no-op if missing)
``invoke_in_context(ctx, fn_name, ...)``                 Call [export, pinvoke] function in agent context
``invoke_debug_agent_method(name, meth, ...)``           Call a method on the agent's class instance
``invoke_debug_agent_method("", meth, ...)``             Call a method on the thread-local agent
``invoke_debug_agent_function(name, fn, ...)``           Call a function in the agent's context
``invoke_debug_agent_function("", fn, ...)``             Call a function in the thread-local agent
``collect_debug_agent_state(ctx, line)``                 Trigger onCollect on all agents
``report_context_state(ctx, cat, name, ...)``            Report variable from onCollect to onVariable
``is_in_debug_agent_creation()``                         True during fork_debug_agent_context
``to_log(level, text)``                                  Log message — routed through onLog if agents exist
``[pinvoke]``                                            Annotation enabling context mutex
=======================================================  =====================================================


.. seealso::

   Full source: :download:`tutorials/language/45_debug_agents.das <../../../../tutorials/language/45_debug_agents.das>`

   :ref:`tutorial_apply_in_context` — cross-context services via
   ``[apply_in_context]`` annotation (Tutorial 46).

   :ref:`tutorial_compile_and_run` — compiling and running programs
   at runtime (Tutorial 44).

   :ref:`Contexts <contexts>` — language reference for context semantics.

   Previous tutorial: :ref:`tutorial_compile_and_run`

   Next tutorial: :ref:`tutorial_apply_in_context`
