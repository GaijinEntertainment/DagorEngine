options gen2
require dastest/testing_boost public
require daslib/fio
require daslib/strings_boost
require strings
require math

// A standalone exe registers its functions with no argument types, so a job or thread lambda's
// capture block used to get a null type in its header and the first collect the thread ran
// walked it: the exe hands its context the capture type of every lambda body at init, and the
// thread's collect keeps the captured array.

def private das_exe() : string {
    let argv <- get_command_line_arguments()
    return find(to_generic_path(argv[0]), "/") < 0 ? argv[0] : get_full_file_name(normalize(argv[0]))
}

def private tail(s : string) : string {
    return slice(s, max(0, length(s) - 400))
}

def private run_capture(cmd : string; var out : string&) : int {
    return unsafe(popen_timeout(cmd, 300.0, $(f) {
        if (f != null) {
            out = fread(f)
        }
    }))
}

def private build_and_run(t : T?; dir : string; tag : string) : string {
    let root = "modules/dasLLVM/tests/_exe_thread_collect_root.das"
    let exe = path_join(dir, "{tag}.exe")
    var bout = ""
    let argv <- [das_exe(), "-jit", "-exe", "-output", path_join(dir, tag), root]
    let brc = run_and_capture(argv, bout, 300.0)
    t |> equal(0, brc, "the -exe build ({tag}) succeeds: {tail(bout)}")
    if (brc != 0 || !stat(exe).is_valid) {
        t |> success(stat(exe).is_valid, "the exe artifact exists at -output ({tag})")
        return ""
    }
    let bin_dir = dir_name(das_exe())
    var cmd = "PATH=\"{bin_dir}:$PATH\" \"{exe}\" 2>&1"
    if (get_platform_name() == "windows") {
        cmd = "set \"PATH={bin_dir};%PATH%\"&& \"{exe}\" 2>&1"
    }
    var out = ""
    let rc = run_capture(cmd, out)
    t |> equal(0, rc, "the exe ({tag}) runs clean: {tail(out)}")
    return out
}

def private expect_lines(t : T?; out : string; what : string) {
    t |> success(find(out, "THREAD_COLLECT_SUM 2016") >= 0, "{what}: the captured array survives the thread's collect: {tail(out)}")
    t |> success(find(out, "JOB_COLLECT_SUM 992") >= 0, "{what}: the captured array survives the job's collect: {tail(out)}")
    t |> success(find(out, "THREAD_COLLECT_DONE") >= 0, "{what}: the thread is joined: {tail(out)}")
}

[test]
def test_exe_thread_lambda_collect(t : T?) {
    let dir_r = create_temp_directory_result("llvm_exe_thread_collect")
    if (!(dir_r is value)) {
        t |> failure("temp dir failed")
        return
    }
    let dir = unsafe(dir_r.value)
    t |> run("a thread lambda's capture survives the collect it runs, interpreted and as a standalone exe") @(t : T?) {
        var hout = ""
        let hrc = run_and_capture([das_exe(), "-no-module-cache", "modules/dasLLVM/tests/_exe_thread_collect_root.das"], hout, 300.0)
        t |> equal(0, hrc, "the fixture runs interpreted on the host: {tail(hout)}")
        expect_lines(t, hout, "interpreted")
        let out = build_and_run(t, dir, "thread_collect")
        expect_lines(t, out, "exe")
    }
    rmdir_rec_result(dir)
}
