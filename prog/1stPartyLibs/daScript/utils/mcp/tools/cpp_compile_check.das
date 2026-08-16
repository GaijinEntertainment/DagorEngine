options gen2

module cpp_compile_check

require common public
require daslib/json
require daslib/json_boost

// C++ syntax-check (clangd-free) driven by the CMake compile DB
// (build/compile_commands.json, emitted by CMAKE_EXPORT_COMPILE_COMMANDS).
//
// For a translation unit in the DB, take its exact compile command, strip the
// codegen tail (/Fo /Fd -c -o + PCH), append /Zs (MSVC) or -fsyntax-only (GNU),
// and run it. Every include in the DB is absolute, so the only relative tokens
// are the dropped outputs — the syntax-only command is cwd-agnostic.

struct private CompileEntry {
    directory : string
    command : string
    file : string
}

struct CppCompileResult {
    file : string
    success : bool
    errors : string
    warnings : string
}

let private HEADER_EXTS <- [".h", ".hpp", ".hh", ".hxx", ".inl", ".ipp", ".cuh"]

def private is_header(path : string) : bool {
    let lp = to_lower(path)
    for (e in HEADER_EXTS) {
        if (ends_with(lp, e)) return true
    }
    return false
}

// Locate compile_commands.json. `build_dir` (optional) is tried first (accepts
// either a build directory or a direct path to the JSON), then a fixed set of
// common build-dir names under the repo root, then build*/ globs.
def cpp_locate_compile_db(build_dir : string) : string {
    let root = get_das_root()
    let names = ["build", "build-ninja", "build-release", "build-debug", "out/build"]
    var candidates : array<string>
    candidates |> reserve(length(names) + 1)
    if (!empty(build_dir)) {
        let bd = resolve_path(build_dir)
        candidates |> push(ends_with(to_lower(bd), "compile_commands.json") ? bd : path_join(bd, "compile_commands.json"))
    }
    for (name in names) {
        candidates |> push(path_join(root, "{name}/compile_commands.json"))
    }
    for (c in candidates) {
        if (fexist(c)) return c
    }
    for (pat in ["build*/compile_commands.json", "cmake-build-*/compile_commands.json"]) {
        var hits : array<string>
        parse_file_list(path_join(root, pat), hits)
        for (h in hits) {
            if (fexist(h)) return h
        }
    }
    return ""
}

def private obj_str(m : JsonValue?; key : string) : string {
    if (m == null || !(m.value is _object)) return ""
    let v = (m.value as _object)?[key] ?? null
    if (v == null || !(v.value is _string)) return ""
    return v.value as _string
}

// Re-quote an argv element that contains whitespace, so joining the `arguments`
// fallback into a command string round-trips back through split_ws + the shell.
def private shell_quote_arg(s : string) : string {
    if (empty(s)) return "\"\""
    if (starts_with(s, "\"") || (find(s, " ") < 0 && find(s, "\t") < 0)) return s
    return "\"{s}\""
}

// `command` (a single shell string) is what CMake emits; some other tools
// (Bear, compdb) emit an `arguments` array instead — join it as a fallback,
// re-quoting elements with spaces so the rejoin round-trips through split_ws.
def private entry_command(m : JsonValue?) : string {
    let c = obj_str(m, "command")
    if (!empty(c)) return c
    if (m == null || !(m.value is _object)) return ""
    let argv = (m.value as _object)?["arguments"] ?? null
    if (argv == null || !(argv.value is _array)) return ""
    let parts <- [for (a in argv.value as _array); shell_quote_arg(a.value as _string); where a != null && a.value is _string]
    return join(parts, " ")
}

def private cpp_load_compile_db(db_path : string; var entries : array<CompileEntry>; var err : string&) : bool {
    let txt = fread(db_path)
    if (empty(txt)) {
        err := "compile DB is empty or unreadable: {db_path}"
        return false
    }
    var perr : string
    var parsed = read_json(txt, perr)
    if (parsed == null) {
        err := "failed to parse {db_path}: {perr}"
        return false
    }
    if (!(parsed.value is _array)) {
        unsafe { delete parsed; }
        err := "unexpected compile DB format (not a JSON array): {db_path}"
        return false
    }
    entries |> reserve(length(parsed.value as _array))
    for (m in parsed.value as _array) {
        entries |> push(CompileEntry(
            directory = obj_str(m, "directory"),
            command = entry_command(m),
            file = obj_str(m, "file")
        ))
    }
    unsafe { delete parsed; }
    return true
}

def private same_path(a, b : string) : bool {
    return to_lower(to_generic_path(a)) == to_lower(to_generic_path(b))
}

def private find_entry(entries : array<CompileEntry>; abs_file : string) : int {
    for (i in range(length(entries))) {
        if (same_path(entries[i].file, abs_file)) return i
    }
    return -1
}

// Whitespace tokenizer that respects double-quotes, so a quoted argument with
// spaces (e.g. `/I"C:\Program Files\inc"` or `-isystem "/path with spaces"`)
// stays one token. Quotes are preserved — the tokens are rejoined and handed to
// the shell, which needs them.
def private split_ws(s : string) : array<string> {
    var out : array<string>
    peek_data(s) $(bytes) {
        let n = length(bytes)
        var i = 0
        while (i < n) {
            while (i < n && (bytes[i] == uint8(' ') || bytes[i] == uint8('\t'))) {
                i++
            }
            if (i >= n) break
            let start = i
            var in_quote = false
            while (i < n) {
                let ch = bytes[i]
                if (ch == uint8('"')) {
                    in_quote = !in_quote
                } elif (!in_quote && (ch == uint8(' ') || ch == uint8('\t'))) {
                    break
                }
                i++
            }
            out |> push(slice(s, start, i))
        }
    }
    return <- out
}

// Detect by the compiler, not a "/Fo" substring (a GNU include path like
// .../Foo/... would false-match). cl.exe and clang-cl are the MSVC drivers.
def private command_is_msvc(cmd : string) : bool {
    let comp = to_lower(base_name(compiler_of(cmd)))
    return comp == "cl.exe" || comp == "cl" || comp == "clang-cl.exe" || comp == "clang-cl"
}

// Strip a matching pair of surrounding double-quotes. split_ws keeps quotes on
// tokens, and a compiler path under "C:\Program Files\..." is quoted — without
// this, base_name() would keep the trailing quote and MSVC detection would fail.
def private unquote(s : string) : string {
    return (length(s) >= 2 && starts_with(s, "\"") && ends_with(s, "\"")) ? slice(s, 1, length(s) - 1) : s
}

def private compiler_of(cmd : string) : string {
    let toks <- split_ws(cmd)
    return empty(toks) ? "" : unquote(toks[0])
}

// Drop the codegen-output tail (object/pdb/asm/PCH outputs and the compile-only
// flag) and append the syntax-only flag. The source TU and all (absolute)
// include/define flags survive, so the result is a self-contained syntax check.
def private to_syntax_only(cmd : string; is_msvc : bool) : string {
    let toks <- split_ws(cmd)
    var out : array<string>
    out |> reserve(length(toks) + 1)
    var skip_next = false
    for (tok in toks) {
        if (skip_next) {
            skip_next = false
            continue
        }
        if (is_msvc) {
            if (starts_with(tok, "/Fo") || starts_with(tok, "/Fd") || starts_with(tok, "/Fa")
                || starts_with(tok, "/FA") || starts_with(tok, "/Fe") || starts_with(tok, "/Fp")
                || starts_with(tok, "/Yu") || starts_with(tok, "/Yc")
                || tok == "-c" || tok == "/c") continue
        } else {
            if (tok == "-c") continue
            if (tok == "-o" || tok == "-include-pch") {
                skip_next = true
                continue
            }
            if (starts_with(tok, "-o") && length(tok) > 2) continue
        }
        out |> push(tok)
    }
    out |> push(is_msvc ? "/Zs" : "-fsyntax-only")
    return join(out, " ")
}

// Strip invocation-noise that's never about the user's code: cl echoes the
// source basename to stdout; "Command line warning Dxxxx" reports build-system
// flag conflicts (e.g. D9025 /GR vs /GR-); clang's "argument unused during
// compilation" notes a flag it ignored. Real code diagnostics (Cxxxx / error:)
// are preserved.
def private filter_compiler_noise(output, src_base : string) : string {
    var keep : array<string>
    for (ln in split(output, "\n")) {
        let s = strip(ln)
        if (empty(s) || s == src_base
            || find(s, "Command line warning") >= 0
            || find(s, "argument unused during compilation") >= 0) continue
        keep |> push(s)
    }
    return join(keep, "\n")
}

// popen captures stdout only; compiler diagnostics go to stderr — merge with 2>&1.
// Run from the entry's `directory`: most compile DBs use absolute paths (so cwd
// is moot), but some producers emit relative include/source paths that only
// resolve from there.
def private run_syntax_check(work_dir, cmd : string; var output : string&) : int {
    // `cd /d` (Windows) handles a different drive and accepts the DB's
    // generic-path `directory` as-is; `cd` is the POSIX form.
    let cd = get_platform_name() == "windows" ? "cd /d" : "cd"
    let full = empty(work_dir) ? "{cmd} 2>&1" : "{cd} \"{work_dir}\" && {cmd} 2>&1"
    var captured : string
    let rc = unsafe(popen_timeout(full, 180.0, $(f) {
        if (f == null) return
        captured = build_string() $(var w) {
            while (!feof(f)) {
                write(w, fgets(f))
            }
        }
    }))
    output := captured
    return rc
}

def private no_db_message() : string {
    return "No compile_commands.json found. Configure a CMake build with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON (the Ninja and Makefile generators emit it; the Visual Studio generator does NOT — use a side Ninja build dir on Windows). Pass build_dir to point at a specific build directory."
}

def do_cpp_compile_check(file : string; build_dir : string = ""; json : bool = false) : string {
    if (empty(file)) return make_tool_result("'file' argument is required", true)
    let abs_file = resolve_path(file)
    if (!fexist(abs_file)) return make_tool_result("file not found: {file}", true)
    let db_path = cpp_locate_compile_db(build_dir)
    if (empty(db_path)) return make_tool_result(no_db_message(), true)
    var entries : array<CompileEntry>
    var lerr : string
    if (!cpp_load_compile_db(db_path, entries, lerr)) return make_tool_result(lerr, true)
    let rel_db = make_relative_path(db_path, get_das_root())
    let idx = find_entry(entries, abs_file)
    if (idx < 0) {
        if (is_header(abs_file)) {
            return make_tool_result("'{file}' is a header — headers are not translation units in the compile DB ({rel_db}). To syntax-check the code that uses it, pass a .cpp/.cc that #includes it.", true)
        }
        return make_tool_result("no compile DB entry for '{file}' in {rel_db} — is it part of the configured build? Try reconfiguring, or pass build_dir.", true)
    }
    let e = entries[idx]
    let is_msvc = command_is_msvc(e.command)
    var output : string
    let rc = run_syntax_check(e.directory, to_syntax_only(e.command, is_msvc), output)
    let timed_out = rc == popen_timed_out
    let success = rc == 0 && !timed_out
    let rel = make_relative_path(abs_file, get_das_root())
    let filtered = filter_compiler_noise(output, base_name(abs_file))
    // On a failure with nothing left after filtering, fall back to raw output so
    // we never report an empty error.
    let diag = (!success && empty(filtered)) ? strip(output) : filtered
    if (json) {
        var result = CppCompileResult(file = rel, success = success)
        if (timed_out) {
            result.errors = "syntax check timed out:\n{diag}"
        } elif (!success) {
            result.errors = diag
        } elif (!empty(diag)) {
            result.warnings = diag
        }
        return make_tool_result(sprint_json(result, false), !success)
    }
    if (timed_out) return make_tool_result("Compile check TIMED OUT for {rel}:\n{diag}", true)
    if (!success) return make_tool_result("Compile check FAILED for {rel} (exit {rc}):\n{diag}", true)
    if (!empty(diag)) return make_tool_result("Compile check OK for {rel} (with warnings):\n{diag}")
    return make_tool_result("Compile check OK for {rel}")
}

def do_cpp_build_info(file : string; build_dir : string = "") : string {
    if (empty(file)) return make_tool_result("'file' argument is required", true)
    let abs_file = resolve_path(file)
    if (!fexist(abs_file)) return make_tool_result("file not found: {file}", true)
    let db_path = cpp_locate_compile_db(build_dir)
    if (empty(db_path)) return make_tool_result(no_db_message(), true)
    var entries : array<CompileEntry>
    var lerr : string
    if (!cpp_load_compile_db(db_path, entries, lerr)) return make_tool_result(lerr, true)
    let rel_db = make_relative_path(db_path, get_das_root())
    let idx = find_entry(entries, abs_file)
    if (idx < 0) return make_tool_result("no compile DB entry for '{file}' in {rel_db} — is it part of the configured build?", true)
    let e = entries[idx]
    let is_msvc = command_is_msvc(e.command)
    let output = build_string() $(var w) {
        w |> write("file: {make_relative_path(abs_file, get_das_root())}\n")
        w |> write("compile DB: {rel_db}\n")
        w |> write("compiler: {compiler_of(e.command)}\n")
        w |> write("driver: {is_msvc ? "msvc" : "gnu"}\n")
        w |> write("build directory: {e.directory}\n")
        w |> write("\nfull compile command:\n{e.command}\n")
        w |> write("\nsyntax-only command (what cpp_compile_check runs):\n{to_syntax_only(e.command, is_msvc)}\n")
    }
    return make_tool_result(output)
}
