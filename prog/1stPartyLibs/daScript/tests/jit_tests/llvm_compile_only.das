options gen2

require dastest/testing_boost
require daslib/module_path
require daslib/strings_boost
require strings

// --jit-compile-only: build + verify + optimize the module, then stop — no DLL written or
// loaded, no jitted function installed, the program runs interpreted. This is the
// cross-emission smoke rail (gemm_generator_plan.md slice F); here we pin the host-triple
// mechanics via a child daslang (argv[0] pattern — see
// modules/dasLLVM/tests/llvm_tune_modes.das).

[test]
def test_jit_compile_only(t : T?) {
    if (!jit_enabled()) {
        // the flag only means something under the LLVM JIT; the child needs -jit too
        t |> success(true)
        return
    }
    let args <- get_command_line_arguments()  // locked view — do not delete
    let bin = args[0]
    let client = "{get_this_module_dir()}/llvm_compile_only_client.das"
    // cmd.exe /c strips the first and last quote of a line that starts with a quote —
    // wrap in a sacrificial outer pair on windows (same trap as the daspkg popen incident)
    var cmd = "\"{bin}\" -jit \"{client}\" -- --jit-compile-only"
    if (get_platform_name() == "windows") {
        cmd = "\"{cmd}\""
    }
    var sawCompileOnly = false
    var sawResult = false
    var sawDllCache = false
    var rc : int
    unsafe {
        rc = popen_timeout(cmd, 300.0) $(f) {
            if (f == null) {
                return
            }
            while (!feof(f)) {
                let ln = fgets(f)
                sawCompileOnly ||= find(ln, "LLVM JIT: compile-only") >= 0
                sawResult ||= find(ln, "COMPILE_ONLY_RESULT 402") >= 0
                sawDllCache ||= find(ln, "DLL cache") >= 0
            }
        }
    }
    t |> equal(rc, 0)
    t |> success(sawCompileOnly)   // the compile-only path ran to the escape
    t |> success(sawResult)        // interpreted execution still produced the right answer
    t |> success(!sawDllCache)     // the DLL cache was neither probed nor written
}
