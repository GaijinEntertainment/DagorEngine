// This file tests creating JIT executable.
options gen2
options no_aot
require daslib/ast
require daslib/fio
require strings

require dastest/testing_boost

let OUTPUT_DIR = "{get_das_root()}/build/tests"
let SCRIPT_PATH = "{OUTPUT_DIR}/jit_exe_test_hello.das"
let EXE_PATH = "{OUTPUT_DIR}/jit_exe_test_hello"
let GLOBAL_INIT_SCRIPT = "{OUTPUT_DIR}/jit_exe_test_global_init.das"
let GLOBAL_INIT_EXE = "{OUTPUT_DIR}/jit_exe_test_global_init"
let CLONE_INIT_SCRIPT = "{OUTPUT_DIR}/jit_exe_test_clone_init.das"
let CLONE_INIT_EXE = "{OUTPUT_DIR}/jit_exe_test_clone_init"
let FNPTR_GLOBAL_SCRIPT = "{OUTPUT_DIR}/jit_exe_test_fnptr_global.das"
let FNPTR_GLOBAL_EXE = "{OUTPUT_DIR}/jit_exe_test_fnptr_global"

def write_hello_script() : bool {
    var ok = false
    mkdir_rec(OUTPUT_DIR)
    fopen(SCRIPT_PATH, "w") $(f) {
        if (f != null) {
            fprint(f, "options gen2\n")
            fprint(f, "[export] def main() \{ print(\"Hello World!\\n\"); \}\n")
            ok = true
        }
    }
    return ok
}

// Emits a script whose only top-level state is `let MAIN_DIR = get_das_root()`,
// then prints it bracketed so an empty value is observable. Reproducer for
// issue #2582 — without the fix the global initializer's call gets registered
// nowhere, LLVM folds the null function-pointer load, init_globals never runs
// the call, and the printed value is empty.
def write_global_init_script() : bool {
    var ok = false
    mkdir_rec(OUTPUT_DIR)
    fopen(GLOBAL_INIT_SCRIPT, "w") $(f) {
        if (f != null) {
            fprint(f, "options gen2\n")
            fprint(f, "let MAIN_DIR = get_das_root()\n")
            fprint(f, "[export] def main() \{ print(\"MAIN_DIR=[\{MAIN_DIR}]\\n\"); \}\n")
            ok = true
        }
    }
    return ok
}

// Cloned-context [init] reproducer. A standalone -exe creates its context via
// jit_create_standalone_ctx (NOT simulate), so the runtime initFunctions array is
// empty; a CLONED context (jobque worker) relies entirely on jitInitScript to re-run
// init. Without the fix, jitInitScript registered only init_globals, never the [init]
// functions, so g_init_marker (set ONLY by [init]) stays 0 in the clone — the exact
// failure that silenced cross-compiled dasAudio (its limiter is set up by an [init]
// gated on the audio context, which only exists on the clone). Fixed: the clone runs
// the full init -> the value read in the worker is 777, matching the interpreter.
def write_clone_init_script() : bool {
    var ok = false
    mkdir_rec(OUTPUT_DIR)
    fopen(CLONE_INIT_SCRIPT, "w") $(f) {
        if (f != null) {
            fprint(f, "options gen2\n")
            fprint(f, "require jobque\n")
            fprint(f, "require daslib/jobque_boost\n")
            fprint(f, "struct IntVal \{ v : int \}\n")
            fprint(f, "var g_init_marker = 0\n")
            fprint(f, "[init]\n")
            fprint(f, "def mark_init() \{ g_init_marker = 777 \}\n")
            fprint(f, "def read_marker() : int \{ return g_init_marker \}\n")
            fprint(f, "[export]\n")
            fprint(f, "def main() \{\n")
            fprint(f, "    var seen = -1\n")
            fprint(f, "    with_job_que() \{\n")
            fprint(f, "        with_channel(1) $(ch) \{\n")
            fprint(f, "            new_job() @() \{\n")
            fprint(f, "                ch |> push_clone(IntVal(v = read_marker()))\n")
            fprint(f, "                ch |> notify_and_release\n")
            fprint(f, "            \}\n")
            fprint(f, "            ch |> for_each_clone() $(val : IntVal#) \{\n")
            fprint(f, "                seen = val.v\n")
            fprint(f, "            \}\n")
            fprint(f, "        \}\n")
            fprint(f, "    \}\n")
            fprint(f, "    print(\"CLONE_INIT_MARKER=[\{seen\}]\\n\")\n")
            fprint(f, "\}\n")
            ok = true
        }
    }
    return ok
}

// A function-pointer global initialized with `@@fn` (an ExprAddr) is a UID-bearing node inside a
// global-variable initializer. A standalone -exe codegens those (generate_globals_initialization_fn),
// unlike -jit where the interpreter runs global inits. Without the fix, create_uid_nodes never gave
// the ExprAddr an id and get_base panicked "Key not found"; with it, the exe runs and prints 42.
def write_fnptr_global_script() : bool {
    var ok = false
    mkdir_rec(OUTPUT_DIR)
    fopen(FNPTR_GLOBAL_SCRIPT, "w") $(f) {
        if (f != null) {
            fprint(f, "options gen2\n")
            fprint(f, "def my_kernel(x : int) : int \{ return x + 1 \}\n")
            fprint(f, "var g_fn = @@my_kernel\n")
            fprint(f, "[export] def main() \{ print(\"FNPTR_RESULT=[\{invoke(g_fn, 41)}]\\n\"); \}\n")
            ok = true
        }
    }
    return ok
}

// Returns true if the standalone .exe was actually produced. Returns false
// when the link step failed — typically on the sanitizer matrix (the runtime
// .so is built with -fsanitize=ubsan/asan/tsan and references __ubsan_handle_*
// etc. that daslang's -exe link command doesn't propagate).
def compile_to_exe(t : T?; src : string; out : string) : bool {
    let exe_path = "{out}.exe"
    // Wipe any leftover from a previous run so fexist below reliably reflects
    // whether THIS compile produced a binary.
    if (fexist(exe_path)) {
        remove_result(exe_path)
    }
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.jit_enabled = true
            cop.jit_exe_mode = true
            cop.jit_dll_mode = false
            access |> add_extra_module("just_in_time", "{get_das_root()}/daslib/just_in_time.das")
            cop.jit_output_path := out
            compile_file(src, access, unsafe(addr(mg)), cop) $(ok, program, _errors) {
                t |> equal(ok, true)
                simulate(program) $(sok, _context, _serrors) {
                    t |> equal(sok, true)
                    // No need to execute, we already created binary.
                }
            }
        }
    }
    return fexist(exe_path)
}

def run_exe(exe : string) : string {
    var output = ""
    unsafe {
        popen("{exe}.exe") $(f) {
            while (!feof(f)) {
                output += fgets(f)
            }
        }
    }
    return output
}

def test_hello_world(t : T?) {
    t |> equal(write_hello_script(), true)
    if (!compile_to_exe(t, SCRIPT_PATH, EXE_PATH)) { return ; }
    t |> equal("Hello World!\n", run_exe(EXE_PATH))
}

// Issue #2582: top-level `let X = builtin()` initializers must register their
// builtins so init_globals can call them. Without the fix, `output` would be
// "MAIN_DIR=[]\n" (the global initializer's call is constant-folded away).
def test_global_init_builtin(t : T?) {
    t |> equal(write_global_init_script(), true)
    if (!compile_to_exe(t, GLOBAL_INIT_SCRIPT, GLOBAL_INIT_EXE)) { return ; }
    let output = run_exe(GLOBAL_INIT_EXE)
    t |> success(starts_with(output, "MAIN_DIR=["))
    t |> success(!starts_with(output, "MAIN_DIR=[]"))
}

// A [init] function must run in a CLONED context (jobque worker) in a standalone
// -exe, matching the interpreter. Regression for the cross-compiled-dasAudio
// silence: without the fix the worker reads g_init_marker == 0 (clone never ran
// [init]); with it, 777.
def test_clone_init_in_thread(t : T?) {
    t |> equal(write_clone_init_script(), true)
    if (!compile_to_exe(t, CLONE_INIT_SCRIPT, CLONE_INIT_EXE)) { return ; }
    let output = run_exe(CLONE_INIT_EXE)
    t |> success(find(output, "CLONE_INIT_MARKER=[777]") != -1)
    t |> success(find(output, "CLONE_INIT_MARKER=[0]") == -1)
}

// A function-pointer global (`var g = @@fn`) must codegen + run in a standalone -exe. Regression for
// the "Key not found" / "unresolved expression" panic on UID-bearing nodes in global initializers.
def test_fnptr_global(t : T?) {
    t |> equal(write_fnptr_global_script(), true)
    if (!compile_to_exe(t, FNPTR_GLOBAL_SCRIPT, FNPTR_GLOBAL_EXE)) { return ; }
    let output = run_exe(FNPTR_GLOBAL_EXE)
    t |> success(find(output, "FNPTR_RESULT=[42]") != -1)
}

[test]
def test_executable(t : T?) {
    // Skip on Windows: standalone exes built here link against the daslang
    // shared lib, and Windows lacks RPATH so the spawned process can't find
    // libDaScript.dll. Skip without daslang JIT or a shared-lib build for the
    // same reason — there's no library to call into at runtime.
    if (get_platform_name() == "windows" || !jit_enabled() || !das_is_dll_build()) { return ; }
    t |> run("hello world") @(t : T?) {
        test_hello_world(t)
    }
    t |> run("global init builtin (issue #2582)") @(t : T?) {
        test_global_init_builtin(t)
    }
    t |> run("[init] runs in cloned context") @(t : T?) {
        test_clone_init_in_thread(t)
    }
    t |> run("function-pointer global (@@fn in global init)") @(t : T?) {
        test_fnptr_global(t)
    }
}
