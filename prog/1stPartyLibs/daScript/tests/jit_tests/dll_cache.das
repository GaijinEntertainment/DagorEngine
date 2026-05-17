// Tests JIT DLL content-addressed caching:
//   - recompile with identical source        -> cache hit  (same DLL filename)
//   - recompile with different source        -> cache miss (new filename, old artifacts GC'd)
// Runs only under -jit on a dll-build binary; in-process via compile_file + simulate.
options gen2
options no_aot

require daslib/fio
require daslib/rtti
require strings
require dastest/testing_boost

let OUTPUT_DIR = "{get_das_root()}/build/tests"
let PROBE_PATH = "{OUTPUT_DIR}/dll_cache_probe.das"

def write_probe(body_expr : string) : bool {
    mkdir_rec(OUTPUT_DIR)
    var ok = false
    fopen(PROBE_PATH, "w") $(f) {
        if (f != null) {
            fprint(f, "options gen2\n")
            fprint(f, "require daslib/just_in_time\n\n")
            fprint(f, "[jit, sideeffects, export]\n")
            fprint(f, "def probe(x : int) : int \{\n")
            fprint(f, "    return {body_expr}\n")
            fprint(f, "\}\n")
            fprint(f, "\n[export]\ndef main() \{\n    let _ = probe(0)\n\}\n")
            ok = true
        }
    }
    return ok
}

def compile_probe() : string {
    var ns : string
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.jit_enabled = true
            cop.jit_dll_mode = true
            access |> add_extra_module("just_in_time", "{get_das_root()}/daslib/just_in_time.das")
            compile_file(PROBE_PATH, access, unsafe(addr(mg)), cop) $(ok, program, errors) {
                if (ok && program != null) {
                    ns = string(program.thisNamespace)
                    simulate(program) $(sok, context, serrors) {}
                }
            }
        }
    }
    return ns
}

def list_dlls(subdir : string) : array<string> {
    var res : array<string>
    dir(subdir) $(fname) {
        if (fname |> ends_with(".dll")) {
            res |> push(fname)
        }
    }
    return <- res
}

def clear_subdir(subdir : string) {
    dir(subdir) $(fname) {
        if (fname == "." || fname == "..") {
            return
        }
        var err : string
        remove("{subdir}/{fname}", err)
    }
}

def test_dll_cache(t : T?) {
    if (!jit_enabled() || !das_is_dll_build()) {
        return
    }
    // first compile seeds the namespace so we can locate the per-script subdir
    t |> equal(write_probe("x + 1"), true)
    let ns = compile_probe()
    t |> equal(empty(ns), false)
    let subdir = ".jitted_scripts/{ns}"
    clear_subdir(subdir)

    // compile 1: cache miss, one DLL appears
    compile_probe()
    let dlls1 <- list_dlls(subdir)
    t |> equal(length(dlls1), 1)
    let first_dll = dlls1[0]

    // compile 2 with same source: cache hit, same filename, count still 1
    compile_probe()
    let dlls2 <- list_dlls(subdir)
    t |> equal(length(dlls2), 1)
    t |> strictEqual(dlls2[0], first_dll, "cache hit keeps same DLL filename")

    // compile 3 with different body: miss, new DLL, old artifacts GC'd
    t |> equal(write_probe("x + 2"), true)
    compile_probe()
    let dlls3 <- list_dlls(subdir)
    t |> equal(length(dlls3), 1)
    t |> equal(dlls3[0] != first_dll, true)
}

[test]
def test_main(t : T?) {
    t |> run("dll_cache") @(t : T?) {
        test_dll_cache(t)
    }
}
