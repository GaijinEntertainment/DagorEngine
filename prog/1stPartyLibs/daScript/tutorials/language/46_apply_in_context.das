// Tutorial 46: Cross-Context Services with apply_in_context
//
// This tutorial covers:
//   - Using a debug agent context as a shared state host
//   - The [apply_in_context] annotation from daslib/apply_in_context
//   - Argument constraints (implicit / temporary types)
//   - Returning values from cross-context functions
//   - Building a cache service shared across contexts
//   - Named agents vs thread-local agents (when to use each)
//
// Prerequisites: Tutorial 45 (Debug Agents)
//
// Key concepts:
//   - A debug agent context can host module-level variables that
//     persist across calls from any context
//   - [apply_in_context(agent_name)] rewrites a function so that
//     its body executes inside the named agent context, while
//     callers invoke it normally with regular syntax
//   - The macro handles marshalling arguments and return values
//     across context boundaries automatically
//   - Arguments that cross context boundaries must be marked
//     `implicit` or use temporary types (`#`)
//   - This pattern is used in production for caches (opengl_cache),
//     shared registries, and cross-context services
//
// Run: daslang.exe tutorials/language/46_apply_in_context.das

options gen2

require daslib/apply_in_context
require daslib/debugger

// ============================================================
// Section 1: Setting up a named context
// ============================================================
// First, we create a debug agent context to host our shared state.
// Unlike Tutorial 45 where agents override DapiDebugAgent methods,
// here we use a plain DapiDebugAgent with no overrides — the agent
// exists solely to own a named context where module-level variables
// live.

var counter : int = 0       // this variable lives in each context
                            // separately — the agent context gets
                            // its own copy

def install_service(ctx : Context) {
    install_new_debug_agent(new DapiDebugAgent(), "counter_service")
}

def init_counter_service() {
    if (!has_debug_agent_context("counter_service")) {
        fork_debug_agent_context(@@install_service)
    }
}

// ============================================================
// Section 2: The [apply_in_context] annotation
// ============================================================
// Functions annotated with [apply_in_context(agent_name)] have
// their body rewritten to execute in the named agent context.
//
// From the caller's perspective, these look like normal functions.
// Under the hood, the macro:
//   1. Verifies the agent context exists
//   2. Marshals arguments across the context boundary
//   3. Calls invoke_in_context to run the body in the agent
//   4. Returns any result back to the caller
//
// The agent context's copy of `counter` is modified — NOT the
// caller's copy.

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

def demo_basic() {
    print("=== basic apply_in_context ===\n")
    init_counter_service()

    // These calls execute in the agent context
    print("  increment() = {increment()}\n")
    print("  increment() = {increment()}\n")
    print("  increment() = {increment()}\n")
    print("  get_counter() = {get_counter()}\n")
    add_to_counter(10)
    print("  after add_to_counter(10) = {get_counter()}\n")

    // Our local counter is still 0 — untouched
    print("  local counter = {counter}\n")

    // output:
    //   increment() = 1
    //   increment() = 2
    //   increment() = 3
    //   get_counter() = 3
    //   after add_to_counter(10) = 13
    //   local counter = 0
}

// ============================================================
// Section 3: Argument constraints
// ============================================================
// Arguments that cross context boundaries must use types that
// can be safely marshalled.  The annotation requires that
// reference-type arguments be marked `implicit`:
//
//   - `string implicit` — strings are reference types in daScript
//   - `var x : int& implicit` — explicit reference parameters
//
// Value types (int, float, bool, etc.) work without annotation.
// Temporary types (`string#`, `int[3]#`) also work.

[apply_in_context(counter_service)]
def set_counter_name(name : string implicit) {
    print("  counter named '{name}', value = {counter}\n")
}

[apply_in_context(counter_service)]
def read_counter(var result : int& implicit) {
    result = counter
}

def demo_arguments() {
    print("\n=== argument constraints ===\n")

    // String arguments need `implicit` annotation
    set_counter_name("my_counter")

    // Reference parameters also need `implicit`
    var val = 0
    read_counter(val)
    print("  read_counter() -> val = {val}\n")

    // output:
    //   counter named 'my_counter', value = 13
    //   read_counter() -> val = 13
}

// ============================================================
// Section 4: A cache service
// ============================================================
// A practical use case: a shared cache backed by a table that
// lives in the agent context.  Any module or context can call
// cache_put / cache_get / cache_has without worrying about
// which context they're in — the data lives in one place.

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
def cache_has(key : string implicit) : bool {
    return key_exists(cache, key)
}

[apply_in_context(my_cache)]
def cache_get(key : string implicit) : int {
    return cache?[key] ?? -1
}

[apply_in_context(my_cache)]
def cache_size() : int {
    return length(cache)
}

def demo_cache() {
    print("\n=== cache service ===\n")
    init_cache_service()

    cache_put("width", 1920)
    cache_put("height", 1080)
    cache_put("fps", 60)

    print("  cache size = {cache_size()}\n")
    print("  has 'width' = {cache_has("width")}\n")
    print("  has 'color' = {cache_has("color")}\n")
    print("  width = {cache_get("width")}\n")
    print("  height = {cache_get("height")}\n")
    print("  fps = {cache_get("fps")}\n")
    print("  missing = {cache_get("missing")}\n")

    // The local `cache` table is empty — all data lives in the agent
    print("  local cache length = {length(cache)}\n")

    // output:
    //   cache size = 3
    //   has 'width' = true
    //   has 'color' = false
    //   width = 1920
    //   height = 1080
    //   fps = 60
    //   missing = -1
    //   local cache length = 0
}

// ============================================================
// Section 5: Thread-local agents vs named agents
// ============================================================
// [apply_in_context] requires a named agent context.  For
// modules that do not need a public name (e.g., the profiler),
// the thread-local agent is preferred.  There can be only ONE
// thread-local agent per thread — that is why it has no name.
// It is installed with `install_new_thread_local_debug_agent`
// and communicated with via `invoke_debug_agent_method("", ...)`.
//
// The thread-local path is faster because it skips the global
// agent map lookup entirely.
//
// Choose based on your use case:
//   - Named agent + [apply_in_context]:
//       best for shared services (caches, registries) that
//       multiple modules need to discover by name.
//       There can be many named agents simultaneously
//   - Thread-local agent + invoke_debug_agent_method("", ...):
//       best for a single performance-critical module
//       (profiler, logger). Only one per thread; fastest dispatch
//
// See Tutorial 45 (Section 9) for thread-local agent examples.

// ============================================================
// Section 6: How it works under the hood
// ============================================================
// The [apply_in_context] macro rewrites each annotated function
// into three parts:
//
//   1. The caller function (keeps the original name) —
//      verifies the agent exists, then calls invoke_in_context
//      to dispatch into the agent context
//
//   2. A CONTEXT`func_name function (runs IN the agent context) —
//      verifies it's actually running in the correct context,
//      then calls the clone function
//
//   3. A CONTEXT_CLONE`func_name function (the original body) —
//      contains the actual implementation code
//
// For functions with return values, the macro creates a result
// pointer parameter that is passed through invoke_in_context
// and written to by the clone function.
//
// This is equivalent to manually writing:
//
//   def get_counter() : int {
//       verify(has_debug_agent_context("counter_service"))
//       var __res__ : int
//       unsafe {
//           invoke_in_context(
//               get_debug_agent_context("counter_service"),
//               @@CONTEXT`get_counter,
//               addr(__res__)
//           )
//       }
//       return __res__
//   }
//
// The annotation saves you from writing this boilerplate for
// every cross-context function.

// ============================================================
// Main
// ============================================================

[export]
def main() {
    demo_basic()
    demo_arguments()
    demo_cache()
}
