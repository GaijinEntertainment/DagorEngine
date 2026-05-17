// gc_typedecl.das — GC TypeDecl leak tests.
// Verifies that compiling and destroying programs does not leak TypeDecl nodes.
// The thread-local gc_root count must not grow across compile+destroy cycles.
options gen2
options multiple_contexts
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require daslib/rtti
require daslib/debugger
require dastest/testing_boost public


def compile_and_run(file_path : string) : bool {
    var result = false
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file(file_path, access, unsafe(addr(mg)), cop) $(ok, program, issues) {
                if (!ok) {
                    to_log(LOG_ERROR, "compile failed: {issues}\n")
                    return
                }
                simulate(program) $(sok; context; serrors) {
                    if (!sok) {
                        to_log(LOG_ERROR, "simulate failed: {serrors}\n")
                        return
                    }
                    unsafe {
                        invoke_in_context(context, "test")
                    }
                    result = true
                }
            }
        }
    }
    return result
}


def compile_only(file_path : string) : bool {
    var result = false
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file(file_path, access, unsafe(addr(mg)), cop) $(ok, program, issues) {
                result = ok
                if (!ok) {
                    to_log(LOG_ERROR, "compile failed: {issues}\n")
                }
            }
        }
    }
    return result
}


[test]
def test_no_leak_compile_only(t : T?) {
    t |> run("compile-only does not leak TypeDecl nodes") @(t : T?) {
        let before = gc_thread_root_count()
        let ok = compile_only("{get_das_root()}/tests/gc/_sub/simple.das")
        let after = gc_thread_root_count()
        t |> success(ok, "compilation succeeded")
        t |> equal(after, before, "thread root count unchanged after compile-only+destroy")
    }
}


[test]
def test_no_leak_compile_and_run(t : T?) {
    t |> run("compile+run+destroy does not grow thread root") @(t : T?) {
        let before = gc_thread_root_count()
        let ok = compile_and_run("{get_das_root()}/tests/gc/_sub/simple.das")
        let after = gc_thread_root_count()
        t |> success(ok, "compilation and execution succeeded")
        t |> equal(after, before, "thread root count unchanged (before={before}, after={after})")
    }
}


[test]
def test_no_leak_complex(t : T?) {
    t |> run("compile+run complex program does not leak") @(t : T?) {
        let before = gc_thread_root_count()
        let ok = compile_and_run("{get_das_root()}/tests/gc/_sub/complex.das")
        let after = gc_thread_root_count()
        t |> success(ok, "compilation and execution succeeded")
        t |> equal(after, before, "thread root count unchanged (before={before}, after={after})")
    }
}


[test]
def test_no_leak_repeated(t : T?) {
    t |> run("10 compile+destroy cycles produce zero drift") @(t : T?) {
        let before = gc_thread_root_count()
        for (i in range(10)) {
            let ok = compile_and_run("{get_das_root()}/tests/gc/_sub/simple.das")
            t |> success(ok, "iteration {i} succeeded")
        }
        let after = gc_thread_root_count()
        t |> equal(after, before, "thread root count unchanged after 10 cycles")
    }
}


[test]
def test_no_leak_compile_error(t : T?) {
    t |> run("failed compilation does not leak TypeDecl nodes") @(t : T?) {
        let before = gc_thread_root_count()
        var inscope access <- make_file_access("")
        using() $(var mg : ModuleGroup) {
            using() $(var cop : CodeOfPolicies) {
                cop.threadlock_context = true
                compile_file("{get_das_root()}/tests/gc/_sub/nonexistent.das", access, unsafe(addr(mg)), cop) $(ok, program, issues) {
                    t |> success(!ok, "compilation should fail for nonexistent file")
                }
            }
        }
        let after = gc_thread_root_count()
        t |> equal(after, before, "thread root count unchanged after failed compilation")
    }
}


[test]
def test_gc_total_id_grows(t : T?) {
    t |> run("gc_total_id increases after compilation (TypeDecl nodes are allocated)") @(t : T?) {
        let before = gc_total_id()
        let ok = compile_and_run("{get_das_root()}/tests/gc/_sub/simple.das")
        let after = gc_total_id()
        t |> success(ok, "compilation succeeded")
        t |> success(after > before, "gc_total_id grew after compilation (nodes were allocated)")
    }
}


[test]
def test_thread_root_decrease_investigation(t : T?) {
    t |> run("investigate per-phase thread root changes") @(t : T?) {
        let c0 = gc_thread_root_count()
        var inscope access <- make_file_access("")
        using() $(var mg : ModuleGroup) {
            using() $(var cop : CodeOfPolicies) {
                cop.threadlock_context = true
                compile_file("{get_das_root()}/tests/gc/_sub/simple.das", access, unsafe(addr(mg)), cop) $(ok, program, issues) {
                    let c1 = gc_thread_root_count()
                    to_log(LOG_INFO, "after compile: {c1} (delta from start: {int64(c1)-int64(c0)})\n")
                    t |> success(ok, "compilation succeeded")
                    simulate(program) $(sok; context; serrors) {
                        let c2 = gc_thread_root_count()
                        to_log(LOG_INFO, "after simulate: {c2} (delta from compile: {int64(c2)-int64(c1)})\n")
                        t |> success(sok, "simulation succeeded")
                        unsafe {
                            invoke_in_context(context, "test")
                        }
                        let c3 = gc_thread_root_count()
                        to_log(LOG_INFO, "after invoke: {c3} (delta from simulate: {int64(c3)-int64(c2)})\n")
                    }
                    let c4 = gc_thread_root_count()
                    to_log(LOG_INFO, "after simulate block exit: {c4} (delta from start: {int64(c4)-int64(c0)})\n")
                }
            }
        }
        let c5 = gc_thread_root_count()
        to_log(LOG_INFO, "after ModuleGroup destroy: {c5} (delta from start: {int64(c5)-int64(c0)})\n")
        // Every phase should have zero delta
        t |> equal(c5, c0, "zero net delta across full lifecycle")
    }
}
