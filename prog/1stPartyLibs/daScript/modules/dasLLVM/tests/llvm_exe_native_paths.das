options gen2
require dastest/testing_boost public
require daslib/fio
require daslib/strings_boost
require strings
require math

// A standalone exe carries the host's require-resolver rows (register_native_path) only when it
// links the whole compiler library - the resolver they feed lives there. A runtime-only exe has
// no compiler, so it registers none; a --jit-register-all-modules exe (the dastest.exe shape,
// which compiles at run time) still carries every row.

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

//! builds the fixture root with the given extra script args and runs the artifact; returns the run's stdout
def private build_and_run(t : T?; dir : string; tag : string; extra_args : array<string>) : string {
    let root = "modules/dasLLVM/tests/_exe_native_paths_root.das"
    let exe = path_join(dir, "{tag}.exe")   //! daslang -exe appends `.exe` to -output itself
    var bout = ""
    var argv <- [das_exe(), "-jit", "-exe", "-output", path_join(dir, tag), root]   //! argv, no shell: a path with spaces needs no quoting
    argv |> push_from(extra_args)
    let brc = run_and_capture(argv, bout, 300.0)
    t |> equal(0, brc, "the -exe build ({tag}) succeeds: {tail(bout)}")
    if (brc != 0 || !stat(exe).is_valid) {
        t |> success(stat(exe).is_valid, "the exe artifact exists at -output ({tag})")
        return ""
    }
    //! the artifact links the producing daslang's runtime DLLs, which resolve through PATH on Windows; the
    //! command opens with `set` or an assignment, never a quote, which cmd.exe would strip
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

def private native_path_rows(out : string) : int {
    let at = find(out, "NATIVE_PATHS ")
    if (at < 0) {
        return -1
    }
    let rest = slice(out, at + length("NATIVE_PATHS "))
    let eol = find(rest, "\n")
    return to_int(eol < 0 ? rest : slice(rest, 0, eol))
}

[test]
def test_exe_native_paths(t : T?) {
    let dir_r = create_temp_directory_result("llvm_exe_native_paths")
    if (!(dir_r is value)) {
        t |> failure("temp dir failed")
        return
    }
    let dir = unsafe(dir_r.value)
    t |> run("a runtime-only exe registers no require-resolver rows") @(t : T?) {
        let out = build_and_run(t, dir, "plain", [])
        t |> equal(0, native_path_rows(out), "no register_native_path row at startup: {tail(out)}")
    }
    t |> run("a whole-lib exe (--jit-register-all-modules) carries the host's rows, each once") @(t : T?) {
        let out = build_and_run(t, dir, "whole", ["--", "--jit-register-all-modules"])
        var hout = ""
        let hrc = run_and_capture([das_exe(), "-no-module-cache", "modules/dasLLVM/tests/_exe_native_paths_root.das"], hout, 300.0)
        t |> equal(0, hrc, "the fixture runs interpreted on the host: {tail(hout)}")
        t |> success(native_path_rows(hout) > 0, "the host scan registered rows: {tail(hout)}")
        t |> equal(native_path_rows(hout), native_path_rows(out), "the exe registers the host's row count - two emission sites would double it: {tail(out)}")
    }
    rmdir_rec_result(dir)
}
