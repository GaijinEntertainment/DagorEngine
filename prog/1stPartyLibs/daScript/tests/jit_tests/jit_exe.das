// This file tests creating JIT executable.
options gen2
options no_aot
require daslib/fio
require daslib/rtti

require dastest/testing_boost

let OUTPUT_DIR = "{get_das_root()}/build/tests"
let SCRIPT_PATH = "{OUTPUT_DIR}/jit_exe_test_hello.das"
let EXE_PATH = "{OUTPUT_DIR}/jit_exe_test_hello"

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

def test_hello_world(t : T?) {
    if (!jit_enabled() || !das_is_dll_build()) {
        // Let's run this test only when JIT enabled.
        // In this case it won't fail if library not exist.
        return
    }
    t |> equal(write_hello_script(), true)

    var inscope access <- make_file_access("")
    using <| $(var mg : ModuleGroup) {
        using <| $(var cop : CodeOfPolicies) {
            cop.jit_enabled = true
            cop.jit_exe_mode = true
            cop.jit_dll_mode = false
            access |> add_extra_module("just_in_time", "{get_das_root()}/daslib/just_in_time.das")
            cop.jit_output_path := EXE_PATH
            compile_file(SCRIPT_PATH, access, unsafe(addr(mg)), cop) <| $(ok, program, errors) {
                t |> equal(ok, true)
                simulate(program) <| $(sok, context, serrors) {
                    t |> equal(sok, true)
                    // No need to execute, we already created binary.
                }
            }
        }
    }

    var output = ""
    unsafe {
        popen("{EXE_PATH}.exe") <| $(f) {
            while (!feof(f)) {
                output += fgets(f)
            }
        }
    }
    t |> equal("Hello World!\n", output)
}

[test]
def test_executable(t : T?) {
    // // Disable JIT test until we find a good way to link on Windows
    // // (Windows missing RPATH and can't find `libDaScript.dll`)
    // t |> run("static label") <| @(t : T?) {
    //     test_hello_world(t)
    // }
}
