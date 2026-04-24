// Tutorial 44: Compiling and Running daScript Programs from daScript
//
// This tutorial covers:
//   - Compiling daScript source code at runtime (from string and from file)
//   - Building virtual file systems with set_file_source
//   - Simulating compiled programs and invoking exported functions
//   - Reading global variables from a child context
//   - Passing results back via pointer arguments
//   - Using channels and lock boxes for cross-context communication
//   - Error handling for compilation and simulation failures
//
// Prerequisites: Tutorial 35 (Job Queues)
//
// Key concepts:
//   - `compile` (from rtti) compiles a source string into a Program
//   - `compile_file` compiles from a file via FileAccess + ModuleGroup
//   - `simulate` converts a Program into a runnable Context
//   - `invoke_in_context` (from debugapi) calls an [export] function
//     in a child context; it always returns void
//   - `get_context_global_variable` reads a global variable from a context
//   - Channel and LockBox (from jobque_boost) are cross-context safe
//
// Run: daslang.exe tutorials/language/44_compile_and_run.das

options gen2
options multiple_contexts   // required when holding smart_ptr<Context>

require daslib/rtti
require daslib/debugger
require daslib/jobque_boost

// ============================================================
// Section 1: Compile from string
// ============================================================
// The simplest way to compile daScript at runtime is `compile`,
// which takes a module name, source text, and CodeOfPolicies.
// The callback receives (ok : bool, program : smart_ptr<Program>,
// issues : string).

def compile_from_string() {
    print("=== compile from string ===\n")
    let src = "
options gen2
[export]
def hello() \{
    print(\"  hello from compiled code!\\n\")
\}
"
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
}

// ============================================================
// Section 2: Compile from file
// ============================================================
// `compile_file` compiles a .das file from disk.  It requires a
// FileAccess (for file I/O) and a ModuleGroup (for module resolution).
// Both must stay alive during compile + simulate.

def compile_from_file() {
    print("\n=== compile from file ===\n")
    var inscope access <- make_file_access("")
    using <| $(var mg : ModuleGroup) {
        using <| $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file("tutorials/language/44_helper.das", access, unsafe(addr(mg)), cop) <| $(ok, program, issues) {
                if (!ok) {
                    print("compile error: {issues}\n")
                    return
                }
                simulate(program) <| $(sok; context; serrors) {
                    if (!sok) {
                        print("simulate error: {serrors}\n")
                        return
                    }
                    // Call "main" in the child context
                    unsafe {
                        invoke_in_context(context, "main")
                    }
                }
            }
        }
    }
    // output:
    //   hello from 44_helper!
}

// ============================================================
// Section 3: Virtual file system
// ============================================================
// You can compile code that does not exist on disk by injecting
// virtual files into the FileAccess object with `set_file_source`.
// This is useful for code generation, REPLs, and eval-like tools.

def virtual_file_system() {
    print("\n=== virtual file system ===\n")
    let generated_code = "
options gen2
[export]
def greet(name : string) \{
    print(\"  greetings, \{name\}!\\n\")
\}
"
    var inscope access <- make_file_access("")
    access |> set_file_source("__generated.das", generated_code)
    using <| $(var mg : ModuleGroup) {
        using <| $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file("__generated.das", access, unsafe(addr(mg)), cop) <| $(ok, program, issues) {
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
                        invoke_in_context(context, "greet", "World")
                    }
                }
            }
        }
    }
    // output:
    //   greetings, World!
}

// ============================================================
// Section 4: Invoke with arguments
// ============================================================
// `invoke_in_context` can pass up to 10 arguments.
// It always returns void — see later sections for result patterns.
// You can also use `has_function` to check if a function exists.

def invoke_with_args() {
    print("\n=== invoke with arguments ===\n")
    let src = "
options gen2
[export]
def sum3(a, b, c : int) \{
    print(\"  sum3 = \{a + b + c\}\\n\")
\}
"
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
                // Check if function exists first
                if (has_function(*context, "sum3")) {
                    print("  has_function(\"sum3\") = true\n")
                }
                if (!has_function(*context, "nonexistent")) {
                    print("  has_function(\"nonexistent\") = false\n")
                }
                unsafe {
                    invoke_in_context(context, "sum3", 10, 20, 30)
                }
            }
        }
    }
    // output:
    //   has_function("sum3") = true
    //   has_function("nonexistent") = false
    //   sum3 = 60
}

// ============================================================
// Section 5: Reading results via global variables
// ============================================================
// Since invoke_in_context returns void, one way to get a result
// is to have the child store it in a global variable, then read
// it back with `get_context_global_variable`.
// The pointer returned is into the child context's memory, so
// copy the value immediately — use a temporary type (`#`) or
// dereference + copy before the context goes out of scope.

def read_global_variable() {
    print("\n=== read global variable ===\n")
    var inscope access <- make_file_access("")
    using <| $(var mg : ModuleGroup) {
        using <| $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file("tutorials/language/44_helper.das", access, unsafe(addr(mg)), cop) <| $(ok, program, issues) {
                if (!ok) {
                    print("compile error: {issues}\n")
                    return
                }
                simulate(program) <| $(sok; context; serrors) {
                    if (!sok) {
                        print("simulate error: {serrors}\n")
                        return
                    }
                    // Call compute(7) — sets global `result` to 7*7+1 = 50
                    unsafe {
                        invoke_in_context(context, "compute", 7)
                    }
                    // Read back the global variable "result"
                    let ptr = unsafe(get_context_global_variable(context, "result"))
                    if (ptr != null) {
                        let value = *unsafe(reinterpret<int?> ptr)
                        print("  result = {value}\n")
                    }
                }
            }
        }
    }
    // output:
    //   result = 50
}

// ============================================================
// Section 6: Passing results via pointer argument
// ============================================================
// Another pattern: pass a pointer from the host into the child.
// The child writes through the pointer, and the host reads it
// after invoke_in_context returns.  The pointer must use a
// temporary type on the receiving end to ensure proper scoping.

def result_via_pointer() {
    print("\n=== result via pointer ===\n")
    var inscope access <- make_file_access("")
    using <| $(var mg : ModuleGroup) {
        using <| $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file("tutorials/language/44_helper.das", access, unsafe(addr(mg)), cop) <| $(ok, program, issues) {
                if (!ok) {
                    print("compile error: {issues}\n")
                    return
                }
                simulate(program) <| $(sok; context; serrors) {
                    if (!sok) {
                        print("simulate error: {serrors}\n")
                        return
                    }
                    // store_via_ptr(val, dst) writes val*10 into *dst
                    var output = 0
                    unsafe {
                        invoke_in_context(context, "store_via_ptr", 5, addr(output))
                    }
                    print("  output = {output}\n")    // 5 * 10 = 50
                }
            }
        }
    }
    // output:
    //   output = 50
}

// ============================================================
// Section 7: Using channels for cross-context results
// ============================================================
// Channels (from jobque_boost) are cross-context safe — ideal for
// collecting structured results from child contexts.
// The host creates a channel, passes it as a `Channel?` argument
// to the child via `invoke_in_context`, and the child pushes
// results with `push_clone` + `notify`.  The host drains the
// channel with `for_each_clone`.
//
// Note: use `notify`, NOT `notify_and_release`.  When a lambda
// captures a channel, its reference count is incremented, so
// `notify_and_release` releases that extra reference and nulls
// the variable.  With `invoke_in_context` there is no lambda —
// the child does not own the channel and no extra reference was
// added — so plain `notify` is correct.
//
// The child script needs `require daslib/jobque_boost` to use
// channel operations.  Use `compile_file` with `make_file_access`
// so the child can resolve daslib modules from disk.

struct IntResult {
    value : int
}

def result_via_channel() {
    print("\n=== result via channel ===\n")
    // Child script: receives a Channel?, pushes a result, notifies
    let child_src = "
options gen2
require daslib/jobque_boost

struct IntResult \{
    value : int
\}

[export]
def produce(var ch : Channel?) \{
    ch |> push_clone(IntResult(value = 42))
    ch |> notify()
\}
"
    // Use make_file_access("") so the child can resolve daslib imports
    var inscope access <- make_file_access("")
    access |> set_file_source("__channel_child.das", child_src)
    using <| $(var mg : ModuleGroup) {
        using <| $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file("__channel_child.das", access, unsafe(addr(mg)), cop) <| $(ok, program, issues) {
                if (!ok) {
                    print("compile error: {issues}\n")
                    return
                }
                simulate(program) <| $(sok; context; serrors) {
                    if (!sok) {
                        print("simulate error: {serrors}\n")
                        return
                    }
                    // Create a channel, pass it to the child, drain results
                    with_channel(1) $(ch) {
                        unsafe {
                            invoke_in_context(context, "produce", ch)
                        }
                        ch |> for_each_clone() $(val : IntResult#) {
                            print("  channel received: {val.value}\n")
                        }
                    }
                }
            }
        }
    }
    // output:
    //   channel received: 42
}

// ============================================================
// Section 8: Error handling
// ============================================================
// Compilation and simulation can fail.  Always check the `ok` /
// `sok` flags.  Runtime errors in the child context can be caught
// with try/recover.

def error_handling() {
    print("\n=== error handling ===\n")

    // 1) Compilation error
    let bad_src = "
options gen2
def broken( \{
    this is not valid
\}
"
    using <| $(var cop : CodeOfPolicies) {
        cop.threadlock_context = true
        compile("bad", bad_src, cop) <| $(ok, program, issues) {
            if (!ok) {
                print("  compile error caught (expected)\n")
            }
        }
    }

    // 2) Runtime error (panic in child)
    let panic_src = "
options gen2
[export]
def crash() \{
    panic(\"intentional crash\")
\}
"
    using <| $(var cop : CodeOfPolicies) {
        cop.threadlock_context = true
        compile("panic_mod", panic_src, cop) <| $(ok, program, issues) {
            if (!ok) {
                print("compile error: {issues}\n")
                return
            }
            simulate(program) <| $(sok; context; serrors) {
                if (!sok) {
                    print("simulate error: {serrors}\n")
                    return
                }
                try {
                    unsafe {
                        invoke_in_context(context, "crash")
                    }
                } recover {
                    print("  runtime error caught (expected)\n")
                }
            }
        }
    }
    // output:
    //   compile error caught (expected)
    //   runtime error caught (expected)
}

// ============================================================
// Main
// ============================================================

[export]
def main() {
    compile_from_string()
    compile_from_file()
    virtual_file_system()
    invoke_with_args()
    read_global_variable()
    result_via_pointer()
    result_via_channel()
    error_handling()
}
