options gen2
require dastest/testing_boost public
require daslib/fio
require daslib/strings_boost
require strings
require math

// the hit path and its misses: modules/dasLLVM/ARCHITECTURE_JIT_ENTRY.md sec.1-2

def private das_exe() : string {
    let argv <- get_command_line_arguments()
    return find(to_generic_path(argv[0]), "/") < 0 ? argv[0] : get_full_file_name(normalize(argv[0]))
}

def private tail(s : string) : string {
    return slice(s, max(0, length(s) - 600))
}

//! the fixture prints its answer and whether the emitter module is in the process; `options
//! log_compile_time` is what turns the backend's phase line on
def private fixture(bound : int) : string {
    return "options gen2\noptions log_compile_time\nrequire daslib/rtti\n\n[export]\ndef main \{\n    var s = 0\n    for (i in range({bound})) \{\n        s += i * i\n    \}\n    print(\"PROBE \{s\}\\n\")\n    print(\"EMITTER \{has_module(\"llvm_macro\")\}\\n\")\n\}\n"
}

//! a helper the DLL holds; `no_jit` on it after the cold run makes the DLL hold a function the
//! plan disables, which the probe calls a miss
def private fixture_with_helper(no_jit_helper : bool) : string {
    let ann = no_jit_helper ? "[no_jit]\n" : ""
    return "options gen2\noptions log_compile_time\nrequire daslib/rtti\n\n{ann}def sq(i : int) : int \{\n    return i * i\n\}\n\n[export]\ndef main \{\n    var s = 0\n    for (i in range(10)) \{\n        s += sq(i)\n    \}\n    print(\"PROBE \{s\}\\n\")\n    print(\"EMITTER \{has_module(\"llvm_macro\")\}\\n\")\n\}\n"
}

//! nothing to jit: the only function is `[no_jit]`, so the plan's candidate set is empty
def private fixture_nothing_to_jit() : string {
    return "options gen2\noptions log_compile_time\nrequire daslib/rtti\n\n[no_jit, export]\ndef main \{\n    print(\"PROBE none\\n\")\n    print(\"EMITTER \{has_module(\"llvm_macro\")\}\\n\")\n\}\n"
}

def private jit_argv(script, cache, pinned : string; in_memory : bool) : array<string> {
    var argv <- [das_exe(), "-jit", "-log-compile-time", "-module-cache", cache, "-output", pinned, script]
    if (in_memory) {
        argv |> push("-jit-no-cache")
    }
    return <- argv
}

def private count_prefix(dir : string; prefix : string) : int {
    var n = 0
    fio::dir(dir) $(name) {
        if (name |> starts_with(prefix)) {
            n++
        }
    }
    return n
}

[test]
def test_jit_link(t : T?) {
    var terr : string
    let tmp = create_temp_directory("das_jit_link", terr)
    if (empty(tmp)) {
        t |> failure("create_temp_directory: {terr}")
        return
    }
    let script = path_join(tmp, "use_jit.das")
    let cache = path_join(tmp, "mc.dascache")
    let pinned = path_join(tmp, "jit_probe")
    fwrite(script, fixture(10))
    t |> run("a cold run misses the pinned DLL, brings the emitter in and emits") @(t : T?) {
        var out = ""
        let rc = run_and_capture(jit_argv(script, cache, pinned, false), out, 300.0)
        t |> equal(0, rc, "the fixture runs: {tail(out)}")
        t |> success(find(out, "PROBE 285") >= 0, "the fixture answered: {tail(out)}")
        t |> success(find(out, "LLVM JIT: DLL cache miss") >= 0 && find(out, "(codegen") >= 0, "the cold run is a miss that emits: {tail(out)}")
        t |> success(find(out, "EMITTER true") >= 0, "the emitter module joined the process for the miss: {tail(out)}")
        t |> equal(0, count_prefix(tmp, "late~"), "the emitter's modules are records of the script's own module cache, not a cache of their own")
        t |> success(find(out, "LLVM JIT time: plan ") >= 0 && find(out, " emitter load ") >= 0, "the link side's plan and the emitter's load print their times: {tail(out)}")
    }
    t |> run("a warm run binds the pinned DLL from the link module alone") @(t : T?) {
        var out = ""
        let rc = run_and_capture(jit_argv(script, cache, pinned, false), out, 300.0)
        t |> equal(0, rc, "the fixture runs: {tail(out)}")
        t |> success(find(out, "PROBE 285") >= 0, "the fixture answered: {tail(out)}")
        t |> success(find(out, "LLVM JIT: DLL cache hit") >= 0 && find(out, "(cache") >= 0, "the warm run is a hit: {tail(out)}")
        t |> success(find(out, "EMITTER false") >= 0, "the emitter module never entered the process on the hit: {tail(out)}")
        t |> success(find(out, "LLVM JIT time: hash ") >= 0 && find(out, " probe ") >= 0 && find(out, " install ") >= 0, "the hit path prints its phases: {tail(out)}")
    }
    t |> run("an in-memory run takes the emitter path and its finalizer frees the engine there") @(t : T?) {
        var out = ""
        let rc = run_and_capture(jit_argv(script, cache, pinned, true), out, 300.0)
        t |> equal(0, rc, "the fixture runs and exits cleanly: {tail(out)}")
        t |> success(find(out, "PROBE 285") >= 0 && find(out, "EMITTER true") >= 0 && find(out, "(codegen") >= 0, "in-memory codegen ran in the emitter: {tail(out)}")
    }
    t |> run("a changed fixture makes the pinned DLL stale, and the miss re-emits") @(t : T?) {
        fwrite(script, fixture(11))
        var out = ""
        let rc = run_and_capture(jit_argv(script, cache, pinned, false), out, 300.0)
        t |> equal(0, rc, "the fixture runs: {tail(out)}")
        t |> success(find(out, "PROBE 385") >= 0, "the new body answered: {tail(out)}")
        t |> success(find(out, "LLVM JIT: DLL cache miss") >= 0 && find(out, "EMITTER true") >= 0, "the stale DLL is a miss that emits: {tail(out)}")
    }
    t |> run("a `[no_jit]` function the DLL holds is a miss, and a program with nothing to jit never loads the emitter") @(t : T?) {
        let helper = path_join(tmp, "use_helper.das")
        let helperPin = path_join(tmp, "jit_helper")
        fwrite(helper, fixture_with_helper(false))
        var out = ""
        var rc = run_and_capture(jit_argv(helper, path_join(tmp, "mc_helper.dascache"), helperPin, false), out, 300.0)
        t |> equal(0, rc, "the helper fixture runs: {tail(out)}")
        t |> success(find(out, "PROBE 285") >= 0 && find(out, "(codegen") >= 0, "the cold run emits the helper: {tail(out)}")
        fwrite(helper, fixture_with_helper(true))
        rc = run_and_capture(jit_argv(helper, path_join(tmp, "mc_helper.dascache"), helperPin, false), out, 300.0)
        t |> equal(0, rc, "the fixture with the disabled helper runs: {tail(out)}")
        t |> success(find(out, "LLVM JIT: DLL cache miss") >= 0 && find(out, "EMITTER true") >= 0, "the DLL holding a now-disabled function is a miss: {tail(out)}")
        t |> success(find(out, "LLVM JIT time: plan ") >= 0 && find(out, " probe ") >= 0 && find(out, " emitter load ") >= 0, "the miss prints the link side's plan and probe times with the emitter's load: {tail(out)}")
        let nothing = path_join(tmp, "use_nothing.das")
        fwrite(nothing, fixture_nothing_to_jit())
        rc = run_and_capture(jit_argv(nothing, path_join(tmp, "mc_nothing.dascache"), path_join(tmp, "jit_nothing"), false), out, 300.0)
        t |> equal(0, rc, "the fixture with nothing to jit runs: {tail(out)}")
        t |> success(find(out, "PROBE none") >= 0 && find(out, "0 functions to jit") >= 0 && find(out, "EMITTER false") >= 0, "an empty candidate set is answered in the link module, no emitter loaded: {tail(out)}")
    }
    t |> run("a pinned DLL another program wrote holds none of this one's functions, and that is a miss") @(t : T?) {
        let other = path_join(tmp, "use_other.das")
        fwrite(other, "options gen2\nrequire daslib/rtti\n\n[export]\ndef main \{\n    var p = 1\n    for (i in range(12)) \{\n        p = (p * 3) % 1000\n    \}\n    print(\"OTHER \{p\}\\n\")\n    print(\"EMITTER \{has_module(\"llvm_macro\")\}\\n\")\n\}\n")
        var out = ""
        let rc = run_and_capture(jit_argv(other, path_join(tmp, "mc_other.dascache"), pinned, false), out, 300.0)
        t |> equal(0, rc, "the other program runs: {tail(out)}")
        t |> success(find(out, "OTHER ") >= 0, "the other program answered: {tail(out)}")
        t |> success(find(out, "LLVM JIT: DLL cache miss") >= 0 && find(out, "EMITTER true") >= 0 && find(out, "0 functions in") < 0, "the DLL of another program is a miss that emits, not a hit binding nothing: {tail(out)}")
    }
    var err : string
    rmdir_rec(tmp, err)
}
