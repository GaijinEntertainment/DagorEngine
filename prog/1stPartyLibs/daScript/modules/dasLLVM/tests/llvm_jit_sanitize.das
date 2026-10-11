options gen2
require _exe_test_common
require daslib/fio
require daslib/strings_boost
require llvm/daslib/llvm_jit_plan

let FIXTURES = "modules/dasLLVM/tests"

//! `root` under `-jit` with `jit_args` after the separator; `dir` pins the DLL path's -output inside a directory this
//! process owns, and an empty `dir` runs the in-memory engine, which with --jit-dump prints the IR it compiles
def private run_jit(dir : string; root : string; jit_args : array<string>; var out : string&) : int {
    var args <- [das_exe(), "-no-module-cache", "-jit"]
    if (empty(dir)) {
        args |> push("-jit-no-cache")
    } else {
        args |> push_from(["-output", "{dir}/{root}{join(jit_args, "")}"])
    }
    args |> push_from(["{FIXTURES}/{root}", "--"], jit_args)
    return run_and_capture(args, out, 300.0)
}

//! the IR `root` compiles to under `jit_args`; the program then runs interpreted, and a sanitizer host may stop
//! on the interpreter's own copy of the fault - the dump is printed first
def private dump_ir(t : T?; root : string; jit_args : array<string>) : string {
    var out = ""
    var args <- ["--jit-compile-only", "--jit-dump"]
    args |> push_from(jit_args)
    let rc = run_jit("", root, args, out)
    t |> success(rc == 0 || find(out, "runtime error") >= 0 || find(out, "ERROR: AddressSanitizer") >= 0, "{root} compiles: {tail(out)}")
    return out
}

def private run_interpreted(root : string; var out : string&) : int {
    return run_and_capture([das_exe(), "-no-module-cache", "{FIXTURES}/{root}"], out, 300.0)
}

//! the exit code of a child a sanitizer stops; one code for both, since a host with both runtimes applies the last one set
let SANITIZER_EXIT = 77

//! the children a block starts print the JIT's info lines (the IR dump, the announce) and exit with SANITIZER_EXIT
//! when a sanitizer stops them, so a test reads the exit code, not the report text; this process read its
//! sanitizer options at startup and is unaffected
def private with_child_env(blk : block<() : void>) {
    let saved <- [for (name in ["ASAN_OPTIONS", "UBSAN_OPTIONS", "DAS_LOG_LEVEL"]); get_env_variable(name)]
    set_env_variable("ASAN_OPTIONS", "{saved[0]}:exitcode={SANITIZER_EXIT}")
    set_env_variable("UBSAN_OPTIONS", "{saved[1]}:exitcode={SANITIZER_EXIT}")
    set_env_variable("DAS_LOG_LEVEL", "info")
    invoke(blk)
    set_env_variable("ASAN_OPTIONS", saved[0])
    set_env_variable("UBSAN_OPTIONS", saved[1])
    set_env_variable("DAS_LOG_LEVEL", saved[2])
}

def private expect_stop(t : T?; rc : int; out : string; never : string) {
    t |> equal(SANITIZER_EXIT, rc, "a sanitizer stops the run: {tail(out)}")
    t |> success(find(out, never) < 0, "the bad value never reaches print: {tail(out)}")
}

//! `root` runs unchecked under plain -jit, and ubsan stops it under --jit-sanitize=undefined before `never` prints
def private expect_ubsan_stop(t : T?; dir : string; root : string; never : string) {
    t |> run("jitted: {root}") @(t : T?) {
        var plain = ""
        let plain_rc = run_jit(dir, root, ["--jit-sanitize=none"], plain)
        t |> equal(0, plain_rc, "plain -jit runs it unchecked: {tail(plain)}")
        var out = ""
        let rc = run_jit(dir, root, ["--jit-sanitize=undefined"], out)
        expect_stop(t, rc, out, never)
    }
}

def private with_temp_dir(t : T?; tag : string; blk : block<(dir : string) : void>) {
    let dir_r = create_temp_directory_result(tag)
    if (!(dir_r is value)) {
        t |> failure("temp dir failed")
        return
    }
    let dir = unsafe(dir_r.value)
    invoke(blk, dir)
    rmdir_rec_result(dir)
}

[test]
def test_jit_sanitize_options(t : T?) {
    with_child_env() {
        t |> run("--jit-sanitize=address instruments the emitted IR on a module that carries the target triple") @(t : T?) {
            let out = dump_ir(t, "_sanitize_root.das", ["--jit-sanitize=address"])
            t |> success(find(out, "--jit-sanitize=address") >= 0, "the knob announces itself: {tail(out)}")
            t |> success(find(out, "@__asan_") >= 0 && find(out, "sanitize_address") >= 0, "the functions are instrumented: {tail(out)}")
            t |> success(find(out, "target triple = ") >= 0, "the triple the shadow mapping is read from: {tail(out)}")
        }
        t |> run("--jit-sanitize=none leaves the IR uninstrumented, on any host") @(t : T?) {
            let out = dump_ir(t, "_sanitize_root.das", ["--jit-sanitize=none"])
            t |> success(find(out, "@__asan_") < 0 && find(out, "sanitize_address") < 0, "no asan in the dump: {tail(out)}")
        }
        t |> run("with no list, a sanitizer host instruments with its own sanitizer and a plain host with none") @(t : T?) {
            let out = dump_ir(t, "_sanitize_root.das", [])
            let asan = JIT_HOST_ASAN
            let ubsan = host_has_symbol(JIT_UBSAN_RUNTIME_SYMBOL)
            t |> equal(asan || ubsan, find(out, "(the host's sanitizer") >= 0, "the host's own sanitizer is the default: {tail(out)}")
            t |> equal(asan, find(out, "@__asan_") >= 0, "asan instrumentation exactly on an asan host: {tail(out)}")
        }
        t |> run("address and undefined combine") @(t : T?) {
            let out = dump_ir(t, "_sanitize_root.das", ["--jit-sanitize=address,undefined"])
            t |> success(find(out, "--jit-sanitize=address,undefined") >= 0, "both announce: {tail(out)}")
        }
        t |> run("an unknown sanitizer name is an error") @(t : T?) {
            var out = ""
            let rc = run_jit("", "_sanitize_root.das", ["--jit-sanitize=bogus", "--jit-compile-only"], out)
            t |> success(rc != 0, "the run fails: {tail(out)}")
            t |> success(find(out, "names no sanitizer") >= 0, "the error names the flag: {tail(out)}")
        }
        t |> run("the in-memory engine refuses address") @(t : T?) {
            var out = ""
            let rc = run_jit("", "_sanitize_root.das", ["--jit-sanitize=address"], out)
            t |> success(rc != 0, "the run fails: {tail(out)}")
            t |> success(find(out, "runs on the JIT DLL path only") >= 0, "the error names the DLL path: {tail(out)}")
        }
        t |> run("--jit-sanitize=undefined bounds-checks a heap object of known size") @(t : T?) {
            let bounds = dump_ir(t, "_ubsan_heap_bounds_root.das", ["--jit-sanitize=undefined"])
            t |> success(find(bounds, "allocsize(0)") >= 0, "the heap helper states its object size: {tail(bounds)}")
            t |> success(find(bounds, "@__ubsan_handle_local_out_of_bounds_abort") >= 0, "the read past the object is checked: {tail(bounds)}")
        }
    }
}

[test]
def test_asan_runtime(t : T?) {
    if (!JIT_HOST_ASAN) {
        with_temp_dir(t, "llvm_jit_sanitize") $(dir) {
            var out = ""
            let rc = run_jit(dir, "_sanitize_root.das", ["--jit-sanitize=address"], out)
            t |> success(rc != 0, "a host without the asan runtime refuses address: {tail(out)}")
            t |> success(find(out, "needs the sanitizer runtime") >= 0, "the error names the missing runtime: {tail(out)}")
        }
        return
    }
    with_child_env() {
        with_temp_dir(t, "llvm_jit_sanitize") $(dir) {
            var stack_out = ""
            let stack_rc = run_jit(dir, "_sanitize_root.das", ["--jit-sanitize=address"], stack_out)
            expect_stop(t, stack_rc, stack_out, "READ_PAST")
            let cases = [
                ("_asan_heap_overflow_root.das", "READ_PAST"),
                ("_asan_container_overflow_root.das", "READ_PAST"),
                ("_asan_linear_use_after_delete_root.das", "READ_AFTER_DELETE"),
                ("_asan_use_after_delete_root.das", "READ_AFTER_DELETE")]
            for (c in cases) {
                t |> run("interpreted and jitted: {c._0}") @(t : T?) {
                    var iout = ""
                    let irc = run_interpreted(c._0, iout)
                    expect_stop(t, irc, iout, c._1)
                    var jout = ""
                    let rc = run_jit(dir, c._0, ["--jit-sanitize=address"], jout)
                    expect_stop(t, rc, jout, c._1)
                }
            }
            t |> run("heap_collect keeps its frame's live object, interpreted and under -jit") @(t : T?) {
                var iout = ""
                let irc = run_interpreted("_gc_collect_carrier_root.das", iout)
                t |> equal(0, irc, "the interpreted collect runs: {tail(iout)}")
                t |> success(find(iout, "WORKER 7") >= 0, "the object survives the interpreted collect: {tail(iout)}")
                var jout = ""
                let jrc = run_jit(dir, "_gc_collect_carrier_root.das", ["--jit-sanitize=address"], jout)
                t |> equal(0, jrc, "the collect carrier runs clean under -jit: {tail(jout)}")
                t |> success(find(jout, "WORKER 7") >= 0, "the object survives the collect under -jit: {tail(jout)}")
            }
        }
    }
}

[test]
def test_ubsan_runtime(t : T?) {
    if (!host_has_symbol(JIT_UBSAN_RUNTIME_SYMBOL)) {
        t |> skip("the ubsan handlers exist only in a daslang built with -DDAS_USE_SANITIZER=undefined")
        return
    }
    with_child_env() {
        with_temp_dir(t, "llvm_jit_sanitize_ubsan") $(dir) {
            expect_ubsan_stop(t, dir, "_ubsan_heap_bounds_root.das", "READ_PAST")
            expect_ubsan_stop(t, dir, "_sanitize_root.das", "READ_PAST")
        }
    }
}
