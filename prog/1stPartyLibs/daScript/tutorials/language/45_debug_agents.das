// Tutorial 45: Debug Agents
//
// This tutorial covers:
//   - What debug agents are and why they exist
//   - Creating and installing a debug agent (fork + install pattern)
//   - Intercepting log output with onLog
//   - Calling functions in the agent context with invoke_in_context
//   - Calling agent class methods with invoke_debug_agent_method
//   - Overriding DapiDebugAgent methods (onCollect, onVariable)
//   - The auto-start module pattern with [_macro] guards
//   - Shutting down agents with delete_debug_agent_context
//   - Thread-local agents with install_new_thread_local_debug_agent
//   - Using "" (empty category) for thread-local dispatch
//
// Prerequisites: Tutorial 44 (Compile and Run) for fork_debug_agent_context
//
// Key concepts:
//   - A debug agent is a daScript object that lives in its own
//     separate "agent context"  a cloned context that stays
//     resident as long as the program is running
//   - `fork_debug_agent_context` clones the current context and
//     calls a setup function inside it
//   - `install_new_debug_agent` registers a DapiDebugAgent subclass
//     under a name, making it globally accessible
//   - DapiDebugAgent has 22 overridable methods for intercepting
//     breakpoints, context lifecycle events, allocations, etc.
//   - `invoke_in_context` calls [export, pinvoke] functions in the
//     agent context from any other context
//   - `invoke_debug_agent_method` calls class methods on the agent
//
// Run: daslang.exe tutorials/language/45_debug_agents.das

options gen2

require daslib/debugger
require daslib/rtti

// ============================================================
// Section 1: Creating and installing a debug agent
// ============================================================
// The fundamental pattern for debug agents is:
//   1. Define a class that extends DapiDebugAgent
//   2. Write a setup function that creates the agent and installs
//      it under a name with `install_new_debug_agent`
//   3. Call `fork_debug_agent_context(@@setup)` to clone the
//      current context and run the setup function in it
//
// The agent lives in its own "agent context"  a separate copy
// of the program that stays resident.  The name you give the
// agent (e.g. "counter") acts as a global key that any context
// can use to find and communicate with this agent.

class CounterAgent : DapiDebugAgent {
    count : int = 0
}

def install_counter(ctx : Context) {
    install_new_debug_agent(new CounterAgent(), "counter")
}

def demo_create_agent() {
    print("=== creating a debug agent ===\n")

    // Before installation
    print("  has 'counter' = {has_debug_agent_context("counter")}\n")

    // Fork + install
    fork_debug_agent_context(@@install_counter)

    // Now the agent is alive
    print("  has 'counter' = {has_debug_agent_context("counter")}\n")

    // output:
    //   has 'counter' = false
    //   has 'counter' = true
}

// ============================================================
// Section 2: Intercepting log output with onLog
// ============================================================
// DapiDebugAgent has an `onLog` method called whenever any
// context prints or logs.  If onLog returns true, the default
// output to stdout is suppressed.  If it returns false, output
// proceeds normally.
//
// This is how profiling tools, IDE log panels, and custom
// loggers intercept program output.

var log_intercept_count : int = 0

class LogAgent : DapiDebugAgent {
    def override onLog(context : Context?; at : LineInfo const?; level : int; text : string#) : bool {
        log_intercept_count++
        return false  // don't suppress  let output reach stdout
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

def demo_on_log() {
    print("\n=== onLog interception ===\n")
    fork_debug_agent_context(@@install_log_agent)

    // Each print/to_log call now triggers our agent's onLog
    print("  hello through agent\n")
    to_log(LOG_INFO, "  info message\n")

    // Read the count from the agent context
    var count = 0
    unsafe {
        invoke_in_context(get_debug_agent_context("log_watcher"), "read_log_count", addr(count))
    }
    // count includes the prints above plus internal dastest output
    print("  log_intercept_count >= 2: {count >= 2}\n")

    // output:
    //   hello through agent
    //   info message
    //   log_intercept_count >= 2: true
}

// ============================================================
// Section 3: Calling functions in the agent context
// ============================================================
// To communicate with the agent, use `invoke_in_context` to call
// [export, pinvoke] functions in the agent context.
//
// `get_debug_agent_context(name)` returns the agent's Context.
// Functions called via invoke_in_context run in that context,
// so they see the agent's copy of module-level variables.
//
// The [pinvoke] annotation is required  it enables the context
// mutex needed for cross-context invocation.
//
// To return values, pass a pointer to a result variable.

// Module-level variable  each context gets its own copy.
// The agent context's copy is the "shared" state.
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
    print("\n=== invoke_in_context ===\n")

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

    // Our local copy is untouched
    print("  agent_counter (local)    = {agent_counter}\n")

    // output:
    //   agent_counter (in agent) = 3
    //   agent_counter (local)    = 0
}

// ============================================================
// Section 4: Calling agent methods with invoke_debug_agent_method
// ============================================================
// `invoke_debug_agent_method` calls a method on the agent's
// class instance directly  no need for [export, pinvoke]
// helper functions.  The agent's `self` is passed automatically.
//
// Syntax:  invoke_debug_agent_method("agent_name", "method", args...)
//
// Use `unsafe { ... }` around the call.

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

def demo_invoke_method() {
    print("\n=== invoke_debug_agent_method ===\n")
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
}

// ============================================================
// Section 5: State collection agent  onCollect & onVariable
// ============================================================
// `onCollect` is called when `collect_debug_agent_state` is
// triggered.  The agent can inspect the calling context and
// report custom variables.  `onVariable` receives each reported
// variable  this pair is how IDE debuggers show custom watch
// variables, ECS entity state, or application-specific data.
//
// `report_context_state` sends a variable (name, type, value)
// from the agent back to the debug system.

class StateAgent : DapiDebugAgent {
    collection_count : int = 0
    def override onCollect(var ctx : Context; at : LineInfo) : void {
        collection_count++
        // Report collection count as a custom variable
        unsafe {
            let tinfo = typeinfo rtti_typeinfo(collection_count)
            report_context_state(ctx, "Diagnostics", "collection_count",
                unsafe(addr(tinfo)), unsafe(addr(collection_count)))
        }
        // Report the stack depth of the calling context
        let depth = stack_depth(ctx)
        unsafe {
            let tinfo = typeinfo rtti_typeinfo(depth)
            report_context_state(ctx, "Diagnostics", "stack_depth",
                unsafe(addr(tinfo)), unsafe(addr(depth)))
        }
    }
    // onVariable is called for each variable reported above
    def override onVariable(var ctx : Context; category, name : string; info : TypeInfo; data : void?) : void {
        unsafe {
            let value = sprint_data(data, addr(info), print_flags.singleLine)
            print("  {category}: {name} = {value}\n")
        }
    }
}

def install_state_agent(ctx : Context) {
    install_new_debug_agent(new StateAgent(), "my_state")
}

def demo_state_collection() {
    print("\n=== state collection (onCollect + onVariable) ===\n")
    fork_debug_agent_context(@@install_state_agent)

    // Trigger collection  the agent sees the calling context
    collect_debug_agent_state(this_context(), get_line_info(1))
    print("  (second collection)\n")
    collect_debug_agent_state(this_context(), get_line_info(1))

    // output:
    //   Diagnostics: collection_count = 1
    //   Diagnostics: stack_depth = ...
    //   (second collection)
    //   Diagnostics: collection_count = 2
    //   Diagnostics: stack_depth = ...
}

// ============================================================
// Section 6: Agent existence and context access
// ============================================================
// `has_debug_agent_context(name)` checks if a named agent exists.
// `get_debug_agent_context(name)` returns the agent's Context.
//
// Always check existence before accessing the context to avoid
// panics.  Agent contexts stay alive for the program's lifetime
// unless explicitly shut down.

def demo_existence_checks() {
    print("\n=== agent existence checks ===\n")

    // Agents installed in previous sections are still alive
    print("  has 'counter'  = {has_debug_agent_context("counter")}\n")
    print("  has 'my_state' = {has_debug_agent_context("my_state")}\n")
    print("  has 'missing'  = {has_debug_agent_context("missing")}\n")

    // output:
    //   has 'counter'  = true
    //   has 'my_state' = true
    //   has 'missing'  = false
}

// ============================================================
// Section 7: Auto-start module pattern
// ============================================================
// In modules (shared libraries), you often want the agent to be
// installed automatically when the module is loaded  without
// requiring the user to call an init function.
//
// The pattern uses a [_macro] function that runs at compile time.
// Four guards ensure safe, single installation:
//
//   1. is_compiling_macros_in_module("module_name")
//       only runs when THIS module is being compiled
//
//   2. !is_in_completion()
//       skips when the IDE is doing code completion
//      (avoids side effects during auto-complete)
//
//   3. !is_in_debug_agent_creation()
//       prevents recursive agent creation if the agent
//      module itself requires another agent module
//
//   4. !has_debug_agent_context("agent_name")
//       avoids duplicate installation if the module is
//      compiled more than once
//
// Here is the canonical pattern (as used in production modules
// like tbotlog.das and profiler.das):
//
//   [_macro]
//   def private auto_start() {
//       if (is_compiling_macros_in_module("my_module") && !is_in_completion()) {
//           if (!is_in_debug_agent_creation()) {
//               if (!has_debug_agent_context("my_agent")) {
//                   fork_debug_agent_context(@@my_agent_setup)
//               }
//           }
//       }
//   }
//
// We cannot demonstrate [_macro] in a standalone tutorial script
// (it requires a named `module`), but here is the equivalent
// runtime pattern that achieves the same idempotent installation:

def ensure_agent(name : string; setup : function<(ctx : Context) : void>) {
    if (!is_in_debug_agent_creation()) {
        if (!has_debug_agent_context(name)) {
            fork_debug_agent_context(setup)
        }
    }
}

class ServiceAgent : DapiDebugAgent {}

def install_service(ctx : Context) {
    install_new_debug_agent(new ServiceAgent(), "service")
}

def demo_auto_start() {
    print("\n=== auto-start pattern (idempotent) ===\n")

    // First call installs the agent
    ensure_agent("service", @@install_service)
    print("  after first:  has 'service' = {has_debug_agent_context("service")}\n")

    // Second call is a no-op (agent already exists)
    ensure_agent("service", @@install_service)
    print("  after second: has 'service' = {has_debug_agent_context("service")}\n")

    // output:
    //   after first:  has 'service' = true
    //   after second: has 'service' = true
}

// ============================================================
// Section 8: Plain agent as named context host
// ============================================================
// A common pattern is to create a plain DapiDebugAgent (no
// overrides) just to own a named context.  Module-level variables
// in that context become shared state accessible via
// invoke_in_context.  This is the foundation of the
// [apply_in_context] pattern covered in Tutorial 46.

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

def demo_named_context() {
    print("\n=== plain agent as named context host ===\n")
    fork_debug_agent_context(@@install_data_host)

    // Multiple calls accumulate in the agent's copy
    unsafe {
        invoke_in_context(get_debug_agent_context("data_host"), "add_data", 10)
        invoke_in_context(get_debug_agent_context("data_host"), "add_data", 20)
        invoke_in_context(get_debug_agent_context("data_host"), "add_data", 30)
    }

    var result = 0
    unsafe {
        invoke_in_context(get_debug_agent_context("data_host"), "get_data", addr(result))
    }
    print("  shared_data (in agent) = {result}\n")
    print("  shared_data (local)    = {shared_data}\n")

    // output:
    //   shared_data (in agent) = 60
    //   shared_data (local)    = 0
}

// ============================================================
// Section 9: Thread-local debug agents
// ============================================================
// A thread-local agent is installed with
// `install_new_thread_local_debug_agent` instead of
// `install_new_debug_agent`.  There can be only ONE
// thread-local agent per thread — that is why it has no
// name: there is nothing to distinguish it from.
//
// Access it by passing an empty string `""` as the category
// to `invoke_debug_agent_method` or
// `invoke_debug_agent_function`.
//
// The thread-local agent exists for speed — dispatching
// through it skips the global agent map lookup entirely.
// The profiler module uses this pattern: it installs itself
// as the thread-local agent and communicates via
// `invoke_debug_agent_method("", ...)`.
//
// Advantages over named agents:
//   - Faster dispatch — no global map lookup
//   - Simpler API — no name needed (there is only one)

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

def demo_thread_local_agent() {
    print("\n=== thread-local debug agent ===\n")
    fork_debug_agent_context(@@install_thread_local_agent)

    // Use "" (empty string) to target the thread-local agent
    unsafe {
        invoke_debug_agent_method("", "set_value", 42)
    }

    var result = 0
    unsafe {
        invoke_debug_agent_method("", "get_value", addr(result))
    }
    print("  thread-local value = {result}\n")

    // invoke_debug_agent_function("", ...) also works with ""
    // but the target function receives the agent's classPtr
    // as its first argument (same as invoke_debug_agent_method)

    // output:
    //   thread-local value = 42
}

// ============================================================
// Section 10: Shutting down a debug agent
// ============================================================
// `delete_debug_agent_context` removes an agent by name.  It
// notifies all other agents via onUninstall, then safely
// destroys the agent and its context.
//
// This is the clean way to remove an agent when it is no longer
// needed — for example, when a profiling session ends or a
// debug tool is closed.
//
// After deletion, `has_debug_agent_context` returns false and
// `get_debug_agent_context` would panic.

def demo_delete_agent() {
    print("\n=== deleting a debug agent ===\n")

    // The "data_host" agent from section 8 is still alive
    print("  has 'data_host' (before) = {has_debug_agent_context("data_host")}\n")

    // Remove it by name
    delete_debug_agent_context("data_host")

    print("  has 'data_host' (after)  = {has_debug_agent_context("data_host")}\n")

    // Deleting a non-existent agent is a no-op
    delete_debug_agent_context("data_host")
    print("  double delete is safe    = true\n")

    // output:
    //   has 'data_host' (before) = true
    //   has 'data_host' (after)  = false
    //   double delete is safe    = true
}

// ============================================================
// Main
// ============================================================

[export]
def main() {
    demo_create_agent()
    demo_on_log()
    demo_invoke_in_context()
    demo_invoke_method()
    demo_state_collection()
    demo_existence_checks()
    demo_auto_start()
    demo_named_context()
    demo_thread_local_agent()
    demo_delete_agent()

    // Clean up all remaining agents
    delete_debug_agent_context("counter")
    delete_debug_agent_context("log_watcher")
    delete_debug_agent_context("calc")
    delete_debug_agent_context("my_state")
    delete_debug_agent_context("service")
}
