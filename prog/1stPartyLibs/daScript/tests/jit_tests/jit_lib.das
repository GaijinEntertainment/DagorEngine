options gen2
options no_aot

require dastest/testing_boost
require daslib/fio
require daslib/strings_boost
require daslib/clargs
require strings


let OUTPUT_DIR = "{get_das_root()}/build/tests"
let FIXTURE_DIR = "{get_das_root()}/tests/jit_tests"
let SHARED_FIXTURE = "{get_das_root()}/tests-cpp/big/standalone_ctx/standalone_init_fixture.das"


def private spawn(cmd : string; var lines : array<string>) : int {
    let rc = unsafe(popen_timeout("{cmd} 2>&1", 600.0) $(f) {
        if (f == null) {
            return
        }
        while (!feof(f)) {
            let ln = strip(fgets(f))
            if (!(ln |> empty())) {
                lines |> push("{ln}")
            }
        }
    })
    return rc
}


def private shared_artifact(stem : string) : string {
    let plat = get_platform_name()
    if (plat == "windows") {
        return "{stem}.dll"
    }
    return plat == "darwin" ? "{stem}.dylib" : "{stem}.so"
}


def private static_artifact(stem : string) : string {
    return get_platform_name() == "windows" ? "{stem}.lib" : "{stem}.a"
}


def private build_lib(bin, source, stem, flags, jit_flags : string; var lines : array<string>) : int {
    mkdir_rec(OUTPUT_DIR)
    return spawn("cd \"{get_das_root()}\" && \"{bin}\" -lib \"{source}\" -output \"{OUTPUT_DIR}/{stem}\" {flags}" +
        " -- {jit_flags} --jit-lib-bindings \"{OUTPUT_DIR}/_{stem}_c.das\"", lines)
}


//! the host is copied beside its generated bindings, because `require` resolves relative to the
//! requiring file - and because writing either into tests/jit_tests would have the sweep reading
//! that directory while this test writes it, which Windows answers with a sharing violation
def private run_host(bin, host : string; var lines : array<string>) : int {
    mkdir_rec(OUTPUT_DIR)
    var copy_error : string
    copy_file("{FIXTURE_DIR}/{host}", "{OUTPUT_DIR}/{host}", true, copy_error)
    if (!empty(copy_error)) {
        lines |> push("cannot stage {host}: {copy_error}")
        return -1
    }
    return spawn("cd \"{get_das_root()}\" && \"{bin}\" \"{OUTPUT_DIR}/{host}\"", lines)
}


def private report(lines : array<string>; what : string) {
    for (ln in lines) {
        to_log(LOG_ERROR, "{what}: {ln}\n")
    }
}


def private lib_tier_ready(t : T?) : bool {
    if (!jit_enabled() || !das_is_dll_build()) {
        to_log(LOG_WARNING, "jit_lib: SKIPPED - -lib needs the LLVM backend and a shared runtime\n")
        t |> success(true, "-lib needs the LLVM backend and a shared runtime to link against")
        return false
    }
    return true
}


[test]
def test_jit_lib_shared(t : T?) {
    if (!lib_tier_ready(t)) {
        return
    }
    let bin = get_host_binary()
    var lines : array<string>
    let rc = build_lib(bin, "{FIXTURE_DIR}/_jit_lib_probe.das", "jit_lib_probe", "", "", lines)
    report(lines, "build")
    t |> success(rc == 0, "daslang -lib exits clean")
    t |> success(fexist("{OUTPUT_DIR}/jit_lib_probe.h"), "the C header is written beside the library")
    t |> success(fexist(shared_artifact("{OUTPUT_DIR}/jit_lib_probe")), "the shared library is written")
    var host_lines : array<string>
    let host_rc = run_host(bin, "_jit_lib_probe_host.das", host_lines)
    report(host_lines, "host")
    t |> success(host_rc == 0, "a struct, a renamed call and a lambda-using body all answer through C")
}


[test]
def test_jit_lib_static_and_export_all(t : T?) {
    if (!lib_tier_ready(t)) {
        return
    }
    let bin = get_host_binary()
    var static_lines : array<string>
    let static_rc = build_lib(bin, "{FIXTURE_DIR}/_jit_lib_probe.das", "jit_lib_probe_s",
        "-lib-export-all", "--jit-lib-static", static_lines) // nolint:LINT029 - lines carries the build transcript for the failure report
    report(static_lines, "static build")
    t |> success(static_rc == 0, "a static export-all build exits clean")
    t |> success(fexist(static_artifact("{OUTPUT_DIR}/jit_lib_probe_s")), "the archive is written")

    var lines : array<string>
    let rc = build_lib(bin, "{FIXTURE_DIR}/_jit_lib_probe.das", "jit_lib_probe_all",
        "-lib-export-all", "", lines)
    report(lines, "build")
    t |> success(rc == 0, "a shared export-all build exits clean")
    var host_lines : array<string>
    let host_rc = run_host(bin, "_jit_lib_export_all_host.das", host_lines)
    report(host_lines, "host")
    t |> success(host_rc == 0, "export-all reaches a bare public function, under the output name's prefix")
}


[test]
def test_jit_lib_result_slot_is_aligned(t : T?) {
    if (!lib_tier_ready(t)) {
        return
    }
    let bin = get_host_binary()
    var lines : array<string>
    let rc = build_lib(bin, "{FIXTURE_DIR}/_jit_lib_align.das", "jit_lib_align", "", "", lines)
    report(lines, "build")
    t |> success(rc == 0, "the alignment fixture builds")
    var host_lines : array<string>
    let host_rc = run_host(bin, "_jit_lib_align_host.das", host_lines)
    report(host_lines, "host")
    t |> success(host_rc == 0, "a structure whose alignment exceeds the argument frame's crosses intact")
}


[test]
def test_jit_lib_binds_into_a_daslang_host(t : T?) {
    if (!lib_tier_ready(t)) {
        return
    }
    let bin = get_host_binary()
    var lines : array<string>
    let rc = build_lib(bin, "{FIXTURE_DIR}/_jit_lib_bind.das", "bindlib", "", "", lines)
    report(lines, "build")
    t |> success(rc == 0, "the bind library builds")
    t |> success(fexist("{OUTPUT_DIR}/_bindlib_c.das"), "the daslang bindings are written")
    var host_lines : array<string>
    let host_rc = run_host(bin, "_jit_lib_bind_host.das", host_lines)
    report(host_lines, "host")
    t |> success(host_rc == 0, "a global, a struct, a string and a panic all behave through the bindings")
}


[test]
def test_jit_lib_runs_the_standalone_fixture(t : T?) {
    if (!lib_tier_ready(t)) {
        return
    }
    let bin = get_host_binary()
    var lines : array<string>
    let rc = build_lib(bin, SHARED_FIXTURE, "standalone_init_fixture", "", "--jit-lib-export-marked", lines)
    report(lines, "build")
    t |> success(rc == 0, "the AOT standalone fixture also builds as a JIT library")
    var host_lines : array<string>
    let host_rc = run_host(bin, "_jit_lib_shared_host.das", host_lines)
    report(host_lines, "host")
    t |> success(host_rc == 0, "both tiers answer the same values from the same source")
}


[test]
def test_jit_lib_is_a_guest_of_a_populated_runtime(t : T?) {
    if (!lib_tier_ready(t)) {
        return
    }
    let bin = get_host_binary()
    var lines : array<string>
    let rc = build_lib(bin, "{FIXTURE_DIR}/_jit_lib_guest.das", "jit_lib_guest", "", "", lines)
    report(lines, "build")
    t |> success(rc == 0, "the guest library builds")
    var host_lines : array<string>
    let host_rc = run_host(bin, "_jit_lib_guest_host.das", host_lines)
    report(host_lines, "host")
    t |> success(host_rc == 0, "a library reaching its own modules survives a populated runtime")
}


[test]
def test_jit_lib_carries_two_module_sets(t : T?) {
    if (!lib_tier_ready(t)) {
        return
    }
    let bin = get_host_binary()
    var fio_lines : array<string>
    let fio_rc = build_lib(bin, "{get_das_root()}/tests-cpp/big/standalone_ctx/standalone_modules_fixture.das",
        "standalone_modules_fixture", "", "--jit-lib-export-marked", fio_lines)
    report(fio_lines, "fio library")
    t |> success(fio_rc == 0, "the AOT modules fixture also builds as a JIT library")
    var guest_lines : array<string>
    let guest_rc = build_lib(bin, "{FIXTURE_DIR}/_jit_lib_guest.das", "jit_lib_guest", "", "", guest_lines)
    report(guest_lines, "ast library")
    t |> success(guest_rc == 0, "the ast library builds")
    var host_lines : array<string>
    let host_rc = run_host(bin, "_jit_lib_two_module_sets_host.das", host_lines)
    report(host_lines, "host")
    t |> success(host_rc == 0, "two libraries with different module sets answer in one process")
}


