options gen2
require dastest/testing_boost public
require daslib/fio
require daslib/strings_boost
require strings
require math

// A standalone exe registers its functions and globals one at a time and seals the lookup tables
// before its init script runs; by-name lookup - has_function, invoke by name, a global by name -
// then answers from the sealed tables, the same way it does in the interpreter.

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
    let root = "modules/dasLLVM/tests/_name_lookup_root.das"
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
    t |> success(find(out, "HAS_FOO true") >= 0, "{what}: has_function finds an overloaded name: {tail(out)}")
    t |> success(find(out, "HAS_BUMP true") >= 0, "{what}: has_function finds a unique name: {tail(out)}")
    t |> success(find(out, "HAS_NOPE false") >= 0, "{what}: has_function misses an unknown name: {tail(out)}")
    t |> success(find(out, "BUMP 8") >= 0, "{what}: invoke by name reaches the unique function: {tail(out)}")
    t |> success(find(out, "G_COUNTER 8") >= 0, "{what}: a global resolves by name after the bump: {tail(out)}")
    t |> success(find(out, "G_NOPE true") >= 0, "{what}: an unknown global name answers null: {tail(out)}")
}

[test]
def test_exe_name_lookup(t : T?) {
    let dir_r = create_temp_directory_result("llvm_exe_name_lookup")
    if (!(dir_r is value)) {
        t |> failure("temp dir failed")
        return
    }
    let dir = unsafe(dir_r.value)
    t |> run("the fixture answers the same by-name lookups interpreted and as a standalone exe") @(t : T?) {
        var hout = ""
        let hrc = run_and_capture([das_exe(), "-no-module-cache", "modules/dasLLVM/tests/_name_lookup_root.das"], hout, 300.0)
        t |> equal(0, hrc, "the fixture runs interpreted on the host: {tail(hout)}")
        expect_lines(t, hout, "interpreted")
        let out = build_and_run(t, dir, "lookup")
        expect_lines(t, out, "exe")
    }
    rmdir_rec_result(dir)
}
