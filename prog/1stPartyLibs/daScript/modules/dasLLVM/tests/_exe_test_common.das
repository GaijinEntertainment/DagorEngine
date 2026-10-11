options gen2
module _exe_test_common shared public

require dastest/testing_boost public
require daslib/fio
require daslib/strings_boost
require strings
require math

// the exe tests' shared helpers: the daslang under test, a log tail, a shell capture, and the
// build-then-run of a fixture root as a standalone exe

def das_exe() : string {
    let argv <- get_command_line_arguments()
    return find(to_generic_path(argv[0]), "/") < 0 ? argv[0] : get_full_file_name(normalize(argv[0]))
}

def tail(s : string) : string {
    return slice(s, max(0, length(s) - 400))
}

def run_capture(cmd : string; var out : string&) : int {
    return unsafe(popen_timeout(cmd, 300.0, $(f) {
        if (f != null) {
            out = fread(f)
        }
    }))
}

//! the major in a clang driver's `--version` line, 0 when the text carries none
def driver_major(version_text : string) : int {
    let at = find(version_text, "clang version ")
    if (at < 0) return 0
    let rest = slice(version_text, at + length("clang version "))
    let dot = find(rest, ".")
    return to_int(dot < 0 ? rest : slice(rest, 0, dot))
}

//! the major of the LLVM this tree bundles, read from the module's CMake pin; 0 when the file is not where an in-repo run keeps it
def bundled_llvm_major() : int {
    var text = ""
    fopen("modules/dasLLVM/CMakeLists.txt", "rb") $(f) {
        if (f != null) {
            text = fread(f)
        }
    }
    let key = "DAS_LLVM_TARGET_VERSION \""
    let at = find(text, key)
    if (at < 0) return 0
    let rest = slice(text, at + length(key))
    let dot = find(rest, ".")
    return to_int(dot < 0 ? rest : slice(rest, 0, dot))
}

//! why the POSIX LTO link cannot run on this box, or "" when it can: the driver is the clang++ on PATH, and it reads
//! the bundled LLVM's bitcode only when its own LLVM is not older (Apple's clang numbers its LLVM its own way and never qualifies)
def lto_driver_missing() : string {
    if (get_platform_name() == "windows") return ""   // the windows host links LTO through the bundled lld-link and needs no driver on PATH
    if (!executable_on_path("clang++")) return "no clang++ on PATH: the POSIX LTO link needs a clang driver"
    var version_text = ""
    run_capture("clang++ --version 2>&1", version_text)
    if (find(version_text, "Apple") >= 0) return "Apple clang on PATH: its LLVM numbering does not say whether it reads the bundled LLVM's bitcode"
    let major = driver_major(version_text)
    let bundled = bundled_llvm_major()
    if (major < bundled) return "clang++ {major} on PATH is older than the bundled LLVM {bundled}: it reads no bitcode of a newer LLVM"
    return ""
}

//! builds `root` as `<dir>/<tag>.exe` with `jit_args` after the separator, checks the build output for
//! each line of `build_says` (the form the build announces, so a dropped flag cannot pass), and runs
//! the exe with the daslang's runtime beside it on PATH; the run's output, or "" once a build or run
//! failure was reported
def build_and_run(t : T?; dir : string; tag : string; root : string; jit_args : array<string>; build_says : array<string>) : string {
    let exe = path_join(dir, "{tag}.exe")
    var bout = ""
    var argv <- [das_exe(), "-jit", "-exe", "-output", path_join(dir, tag), root]
    if (!empty(jit_args)) {
        argv |> push("--")
        argv |> push_from(jit_args)
    }
    let brc = run_and_capture(argv, bout, 300.0)
    t |> equal(0, brc, "the -exe build ({tag}) succeeds: {tail(bout)}")
    for (says in build_says) {
        t |> success(find(bout, says) >= 0, "the build ({tag}) announces `{says}`: {tail(bout)}")
    }
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
    return rc == 0 ? out : ""
}
