options gen2

require dastest/testing_boost
require daslib/module_path
require daslib/strings_boost
require daslib/fio
require strings

// The [tune] grid modes are compile-time-env driven (DAS_TUNE_MODE) — a hosted test cannot set
// that for its own compile, so this test spawns a child daslang (the same binary that launched
// this run, argv[0]) on llvm_tune_modes_client.das with the env set, and asserts the streamed
// per-variant results: reference row, per-perm generated values (k0/k1/k2 -> 5/6/7 — distinct
// values prove per-perm args reached the generator), and the declined row falling back to the
// reference body. The manifest round-trip is CLI-gated (gen_tune_probe) — see
// gemm_generator_plan.md M3.

[test]
def test_llvm_tune_grid_mode(t : T?) {
    if (!jit_enabled()) {
        // the generator only fires under the LLVM JIT; the child needs -jit too
        t |> success(true)
        return
    }
    let args <- get_command_line_arguments()  // locked view — do not delete
    let bin = args[0]
    let client = "{get_this_module_dir()}/llvm_tune_modes_client.das"
    let inner = "\"{bin}\" -jit \"{client}\""
    // popen goes through cmd.exe / /bin/sh — set the env inside the command line
    let cmd = get_platform_name() == "windows" ? "set DAS_TUNE_MODE=test&& {inner}" : "DAS_TUNE_MODE=test {inner}"
    var lines : array<string>
    var rc : int
    unsafe {
        rc = popen_timeout(cmd, 300.0) $(f) {
            if (f == null) {
                return
            }
            while (!feof(f)) {
                let ln = fgets(f)
                if (ln |> starts_with("VARIANT ") || ln |> starts_with("KVARIANT ")) {
                    lines |> push("{ln}")
                }
            }
        }
    }
    t |> equal(rc, 0)
    var seen : table<string>
    for (ln in lines) {
        seen |> insert(strip(ln))
    }
    t |> success(seen |> key_exists("VARIANT reference -> 5"))
    t |> success(seen |> key_exists("VARIANT k0 -> 5"))
    t |> success(seen |> key_exists("VARIANT k1 -> 6"))
    t |> success(seen |> key_exists("VARIANT k2 -> 7"))
    t |> success(seen |> key_exists("VARIANT declined -> 5"))
    // the [tune_companion] grid: per-perm args reach the companion generator, and the
    // declined row's companion falls back to its reference body in lockstep
    t |> success(seen |> key_exists("KVARIANT reference -> -1"))
    t |> success(seen |> key_exists("KVARIANT k0 -> 0"))
    t |> success(seen |> key_exists("KVARIANT k1 -> 1"))
    t |> success(seen |> key_exists("KVARIANT k2 -> 2"))
    t |> success(seen |> key_exists("KVARIANT declined -> -1"))
    delete seen
    delete lines
}
