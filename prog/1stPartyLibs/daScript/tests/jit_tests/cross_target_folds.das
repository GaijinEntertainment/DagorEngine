// The target-side folds: get_target_triple / get_target_architecture_name answer for the artifact
// a compile EMITS (the --jit-target triple), the host's values with no triple, and a static_if
// over the triple selects the cross arm (the tune framework's inert gate makes the same test), so
// a wasm or console artifact built on a tuned box carries reference bodies and no sidecar stamps.
// Compile-only keeps the cross run on this host: the folds are decided from argv before the
// first module parses, and main runs interpreted. The children run with the default module
// cache on purpose: a --jit-target after the separator keys the cache, or a cached native
// compile serves the cross one with the host's folds.
options gen2
options no_aot

require dastest/testing_boost
require daslib/fio
require daslib/strings_boost
require strings

let FIXTURE = "{get_das_root()}/tests/jit_tests/_cross_target_fixture.das"

def private spawn_child(cmd : string; var lines : array<string>) : int {
    var rc : int
    var full = "{cmd} 2>&1"
    if (get_platform_name() == "windows") {
        full = "\"{full}\""   // cmd.exe strips the first and last quote of a line that opens with one
    }
    rc = unsafe(popen_timeout(full, 300.0) $(f) {
        if (f == null) {
            return
        }
        while (!feof(f)) {
            let ln = strip(fgets(f))
            if (!empty(ln)) {
                lines |> push("{ln}")
            }
        }
    })
    return rc
}

def private has(lines : array<string>; needle : string) : bool {
    for (ln in lines) {
        if (find(ln, needle) >= 0) return true
    }
    return false
}

[test]
def test_cross_target_folds(t : T?) {
    if (!jit_enabled()) {
        t |> success(true)
        return
    }
    let args <- get_command_line_arguments()  // locked view — do not delete
    let bin = args[0]
    t |> run("a native compile answers the host and keeps the framework live") @(t : T?) {
        var ln : array<string>
        spawn_child("\"{bin}\" -jit \"{FIXTURE}\"", ln)
        t |> success(ln |> has("TRIPLE=[]"), "no triple on a native compile: {join(ln, " | ")}")
        t |> success(ln |> has("TARGET_ARCH=[{get_architecture_name()}]"), "the target arch is the host's")
        t |> success(ln |> has("CROSS_PLATFORM=[]"), "no cross platform")
        t |> success(ln |> has("CROSS=[no]"), "a native compile takes the native arm")
        t |> success(ln |> has("LET_WASM=[false]"), "a module-level let over the target folds false natively")
    }
    t |> run("a wasm64 cross target answers wasm64 and turns the framework inert") @(t : T?) {
        var ln : array<string>
        spawn_child("\"{bin}\" -jit \"{FIXTURE}\" -- --jit-target=wasm64-unknown-emscripten --jit-compile-only", ln)
        t |> success(ln |> has("TRIPLE=[wasm64-unknown-emscripten]"), "the triple folds: {join(ln, " | ")}")
        t |> success(ln |> has("TARGET_ARCH=[wasm64]"), "the target arch is the triple's")
        t |> success(ln |> has("HOST_ARCH=[{get_architecture_name()}]"), "the host arch stays the host's")
        t |> success(ln |> has("CROSS_PLATFORM=[emscripten]"), "the cross platform is emscripten")
        t |> success(ln |> has("CROSS=[yes]"), "a cross compile takes the cross arm")
        t |> success(ln |> has("LET_WASM=[true]"), "a module-level let over the target folds true for wasm64")
    }
    t |> run("the last --jit-target on the command line wins, as every other argv reader takes it") @(t : T?) {
        var ln : array<string>
        spawn_child("\"{bin}\" -jit \"{FIXTURE}\" -- --jit-target=x86_64-unknown-linux-gnu --jit-target=wasm64-unknown-emscripten --jit-compile-only", ln)
        t |> success(ln |> has("TRIPLE=[wasm64-unknown-emscripten]"), "the last triple folds: {join(ln, " | ")}")
    }
    t |> run("an x86_64 linux cross target answers x86_64") @(t : T?) {
        var ln : array<string>
        spawn_child("\"{bin}\" -jit \"{FIXTURE}\" -- --jit-target=x86_64-unknown-linux-gnu --jit-compile-only", ln)
        t |> success(ln |> has("TARGET_ARCH=[x86_64]"), "the target arch is the triple's: {join(ln, " | ")}")
        t |> success(ln |> has("CROSS_PLATFORM=[]"), "linux is not a cross platform name")
        t |> success(ln |> has("CROSS=[yes]"), "any cross target takes the cross arm")
    }
}
