// This file tests creating JIT executable.
options gen2
options no_aot
require daslib/fio
require daslib/rtti
require strings

require dastest/testing_boost

let OUTPUT_DIR = "{get_das_root()}/build/tests"
let SCRIPT_PATH = "{OUTPUT_DIR}/jit_exe_test_hello.das"
let EXE_PATH = "{OUTPUT_DIR}/jit_exe_test_hello"
let GLOBAL_INIT_SCRIPT = "{OUTPUT_DIR}/jit_exe_test_global_init.das"
let GLOBAL_INIT_EXE = "{OUTPUT_DIR}/jit_exe_test_global_init"

def write_hello_script() : bool {
    var ok = false
    mkdir_rec(OUTPUT_DIR)
    fopen(SCRIPT_PATH, "w") <| $(f) {
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
    fopen(GLOBAL_INIT_SCRIPT, "w") <| $(f) {
        if (f != null) {
            fprint(f, "options gen2\n")
            fprint(f, "let MAIN_DIR = get_das_root()\n")
            fprint(f, "[export] def main() \{ print(\"MAIN_DIR=[\{MAIN_DIR}]\\n\"); \}\n")
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
    using <| $(var mg : ModuleGroup) {
        using <| $(var cop : CodeOfPolicies) {
            cop.jit_enabled = true
            cop.jit_exe_mode = true
            cop.jit_dll_mode = false
            access |> add_extra_module("just_in_time", "{get_das_root()}/daslib/just_in_time.das")
            cop.jit_output_path := out
            compile_file(src, access, unsafe(addr(mg)), cop) <| $(ok, program, errors) {
                t |> equal(ok, true)
                simulate(program) <| $(sok, context, serrors) {
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
        popen("{exe}.exe") <| $(f) {
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

[test]
def test_executable(t : T?) {
    // Skip on Windows: standalone exes built here link against the daslang
    // shared lib, and Windows lacks RPATH so the spawned process can't find
    // libDaScript.dll. Skip without daslang JIT or a shared-lib build for the
    // same reason — there's no library to call into at runtime.
    if (get_platform_name() == "windows") { return ; }
    if (!jit_enabled() || !das_is_dll_build()) { return ; }
    t |> run("hello world") <| @(t : T?) {
        test_hello_world(t)
    }
    t |> run("global init builtin (issue #2582)") <| @(t : T?) {
        test_global_init_builtin(t)
    }
}
