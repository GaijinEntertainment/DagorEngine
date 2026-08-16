options gen2

module cpp_format_file

require common public
require daslib/json_boost

// C++ formatting via clang-format. Safe-scoped: only formats when a
// .clang-format is discoverable by walking up from the target file. The
// daScript tree ships none, so this is a clean no-op-with-message here; it's
// for external C++ consumers that carry a .clang-format. Adopting a repo style
// is explicitly out of scope.

struct CppFormatResult {
    file : string
    status : string   // "formatted" | "no_config" | "unavailable" | "error"
    message : string
}

var private clang_format_cmd : string

def private try_clang_format(cmd : string) : bool {
    var ok = false
    let rc = unsafe(popen_argv([cmd, "--version"], 5.0, $(f) {
        if (f == null) return
        let line = fread(f)
        if (find(line, "clang-format") >= 0) {
            ok = true
        }
    }))
    return ok && rc == 0
}

def detect_clang_format() : string {
    for (c in ["clang-format", "clang-format.exe"]) {
        if (try_clang_format(c)) return c
    }
    return ""
}

def get_clang_format_cmd() : string {
    if (empty(clang_format_cmd)) {
        clang_format_cmd = detect_clang_format()
    }
    return clang_format_cmd
}

// Walk up from `start_dir` looking for a .clang-format / _clang-format file.
// Returns the full path to the config file found, or "" if none up to the root.
def private find_clang_format_config(start_dir : string) : string {
    var dir = start_dir
    while (!empty(dir)) {
        let dot = path_join(dir, ".clang-format")
        if (fexist(dot)) return dot
        let under = path_join(dir, "_clang-format")
        if (fexist(under)) return under
        let parent = dir_name(dir)
        if (parent == dir || empty(parent)) break
        dir = parent
    }
    return ""
}

def private one_result(r : CppFormatResult; is_error : bool = false) : string {
    return make_tool_result(sprint_json([r], false), is_error)
}

def do_cpp_format_file(file : string) : string {
    if (empty(file)) return one_result(CppFormatResult(file = file, status = "error", message = "'file' argument is required"), true)
    let abs_file = resolve_path(file)
    if (!fexist(abs_file)) return one_result(CppFormatResult(file = file, status = "error", message = "file not found: {file}"), true)
    let rel = make_relative_path(abs_file, get_das_root())
    let cfg_path = find_clang_format_config(dir_name(abs_file))
    if (empty(cfg_path)) {
        return one_result(CppFormatResult(file = rel, status = "no_config",
            message = "no .clang-format found walking up from the file — skipped (no style to apply). Expected in the daScript tree, which ships none."))
    }
    let cf = get_clang_format_cmd()
    if (empty(cf)) {
        return one_result(CppFormatResult(file = rel, status = "unavailable",
            message = "clang-format not found on PATH — install it to enable C++ formatting."), true)
    }
    var output : string
    let rc = run_and_capture([cf, "-i", "-style=file", abs_file], output, 60.0)
    if (rc != 0) {
        return one_result(CppFormatResult(file = rel, status = "error",
            message = "clang-format failed (exit {rc}): {strip(output)}"), true)
    }
    return one_result(CppFormatResult(file = rel, status = "formatted",
        message = "formatted in place using style from {make_relative_path(cfg_path, get_das_root())}"))
}
