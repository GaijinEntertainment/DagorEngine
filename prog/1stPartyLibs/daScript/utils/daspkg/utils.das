options gen2
options indenting = 4
options no_global_variables = false

module utils shared public

require daslib/fio
require strings

let COPY_WINDOW_BYTES = 1024 * 1024

// ── Logging ──────────────────────────────────────────────────────────

var log_file_path : string
var log_verbose_flag = false
var run_cmd_counter = 0

def log_init(root : string) {
    let modules_dir = "{root}/modules"
    mkdir(modules_dir)
    log_file_path = "{modules_dir}/.daspkg.log"
    fopen(log_file_path, "ab") $(f) {
        if (f != null) {
            fprint(f, "\n--- daspkg ---\n")
        }
    }
}

def private log_to_file(msg : string) {
    if (empty(log_file_path)) return
    fopen(log_file_path, "ab") $(f) {
        if (f != null) {
            fprint(f, msg)
        }
    }
}

// screen + file
def log(msg : string) {
    print(msg)
    log_to_file(msg)
}

// file always, screen only with --verbose
def log_debug(msg : string) {
    let line = "  [debug] {msg}\n"
    if (log_verbose_flag) {
        print(line)
    }
    log_to_file(line)
}

def run_cmd(cmd : string; var output : string&) : int {
    // Clear caller's buffer at entry so a no-output run doesn't leak stale
    // text from a prior call (and so the empty(output) gate below is reliable).
    output = ""
    log_debug("$ {cmd}")
    run_cmd_counter ++
    // The capture file lives in the system temp dir, NOT under das_root: the SDK
    // root may be read-only (installed SDK, the buildd sandbox's ro-mounted
    // worktree). das_root only as a last-resort fallback.
    var tmp_err = ""
    var tmp_out = create_temp_file("daspkg_cmd_{run_cmd_counter}_", ".txt", tmp_err)
    if (empty(tmp_out)) {
        tmp_out = "{get_das_root()}/_daspkg_cmd_output_{run_cmd_counter}.txt"
    }
    // Windows _popen invokes cmd.exe /c <our-cmd>. When <our-cmd> starts with a
    // quoted exe path AND contains `>`, cmd.exe's old-behavior rule strips the
    // first AND last `"`, mangling the inner quoting. Wrapping the whole thing
    // in an extra outer pair of `"` gives cmd.exe sacrificial quotes to strip,
    // leaving the inner command intact. POSIX shells don't have this quirk and
    // would treat the outer `"` literally — only do this on Windows.
    var full_cmd = "{cmd} > \"{tmp_out}\" 2>&1"
    if (get_platform_name() == "windows") {
        full_cmd = "\"{full_cmd}\""
    }
    var pipe_output : string
    let exit_code = unsafe(popen(full_cmd) $(f) {
        if (f != null) {
            pipe_output = fread(f)
        }
    })
    let tmp_existed = fexist(tmp_out)
    if (tmp_existed) {
        fopen(tmp_out, "rb") $(f) {
            if (f != null) {
                output = fread(f)
            }
        }
        remove(tmp_out)
    }
    // Merge pipe output (cmd.exe / shell-level errors that bypass our redirect)
    // into the returned output. The redirect captures the child's stdout+stderr;
    // the pipe captures anything the shell itself produced before/around it.
    if (!empty(pipe_output)) {
        if (empty(output)) {
            output = pipe_output
        } else {
            output = "{output}\n--- shell pipe ---\n{pipe_output}"
        }
    }
    // Synthesize a diagnostic when nothing was captured but the command failed —
    // otherwise callers print "(rc=N):\n\n" with no signal. Includes the exact
    // cmd line, the wrapped form sent to popen, and whether the redirect file
    // was even created (false => redirect/popen failure, not a silent child).
    if (exit_code != 0 && empty(output)) {
        output = "<no output captured>\n  cmd:      {cmd}\n  full_cmd: {full_cmd}\n  tmp_file_existed: {tmp_existed}"
    }
    if (exit_code != 0) {
        log_debug("exit={exit_code}")
    }
    if (!empty(output)) {
        log_to_file("  output: {output}\n")
    }
    return exit_code
}

// force-remove a directory tree, handling read-only files (e.g. git objects on Windows)
def force_rmdir(path : string) {
    if (!fexist(path)) return
    // rmdir_rec can't remove read-only files (git objects on Windows)
    // use shell rm -rf which handles this correctly
    var output : string
    let exit_code = run_cmd("rm -rf \"{path}\"", output)
    if (exit_code != 0) {
        // fallback: try builtin rmdir_rec
        rmdir_rec(path)
    }
}

def package_name_from_source(source : string) : string {
    // "github.com/user/dasImgui" -> "dasImgui"
    // "/path/to/mymod" -> "mymod"
    // "C:\path\to\mymod" -> "mymod"
    var last_sep = rfind(source, "/")
    let last_back = rfind(source, "\\")
    if (last_back > last_sep) {
        last_sep = last_back
    }
    if (last_sep >= 0) return slice(source, last_sep + 1)
    return source
}

def is_local_path(source : string) : bool {
    if (starts_with(source, "/") || starts_with(source, "./") || starts_with(source, "../")
        || starts_with(source, ".\\") || starts_with(source, "..\\")) return true
    return length(source) > 2 && character_at(source, 1) == ':'  // nolint:PERF003 — single-byte check on short prefix
}

// --- Version parsing and constraint matching ---

struct SemVer {
    major : int
    minor : int
    patch : int
    valid : bool
}

def parse_version(ver : string) : SemVer {
    var s = ver
    // strip leading "v" or "V"
    if (starts_with(s, "v") || starts_with(s, "V")) {
        s = slice(s, 1)
    }
    var result = SemVer(valid = false)
    // split on "."
    let dot1 = find(s, ".")
    if (dot1 < 0) {
        // just major
        let maj = to_int(s)
        if ("{maj}" == s) {
            result.major = maj
            result.valid = true
        }
        return result
    }
    let major_str = slice(s, 0, dot1)
    let rest = slice(s, dot1 + 1)
    let dot2 = find(rest, ".")
    if (dot2 < 0) {
        // major.minor
        let maj = to_int(major_str)
        let min = to_int(rest)
        if ("{maj}" == major_str && "{min}" == rest) {
            result.major = maj
            result.minor = min
            result.valid = true
        }
        return result
    }
    let minor_str = slice(rest, 0, dot2)
    let patch_str = slice(rest, dot2 + 1)
    let maj = to_int(major_str)
    let min = to_int(minor_str)
    let pat = to_int(patch_str)
    if ("{maj}" == major_str && "{min}" == minor_str && "{pat}" == patch_str) {
        result.major = maj
        result.minor = min
        result.patch = pat
        result.valid = true
    }
    return result
}

// returns -1, 0, or 1
def compare_versions(a, b : SemVer) : int {
    if (a.major != b.major) return a.major < b.major ? -1 : 1
    if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1
    if (a.patch != b.patch) return a.patch < b.patch ? -1 : 1
    return 0
}

// check a single constraint like ">=1.0" or "<2.0" or "1.2.3"
def private check_single_constraint(version : SemVer; constraint : string) : bool {
    var op = ""
    var ver_str = constraint
    if (starts_with(constraint, ">=")) {
        op = ">="
        ver_str = strip(slice(constraint, 2))
    } elif (starts_with(constraint, "<=")) {
        op = "<="
        ver_str = strip(slice(constraint, 2))
    } elif (starts_with(constraint, ">")) {
        op = ">"
        ver_str = strip(slice(constraint, 1))
    } elif (starts_with(constraint, "<")) {
        op = "<"
        ver_str = strip(slice(constraint, 1))
    } elif (starts_with(constraint, "=")) {
        op = "="
        ver_str = strip(slice(constraint, 1))
    } else {
        op = "="
    }
    let target = parse_version(ver_str)
    if (!target.valid) return false
    let cmp = compare_versions(version, target)
    if (op == ">=") {
        return cmp >= 0
    } elif (op == "<=") {
        return cmp <= 0
    } elif (op == ">") {
        return cmp > 0
    } elif (op == "<") {
        return cmp < 0
    } else {
        return cmp == 0
    }
}

// check a version against a constraint string (comma-separated AND)
// examples: "", ">=1.0", "1.2.3", ">=1.0,<2.0"
def satisfies_constraint(version_str, constraint : string) : bool {
    if (empty(constraint)) return true
    let ver = parse_version(version_str)
    if (!ver.valid) return false
    // split on comma
    var ok = true
    peek_data(constraint) $(d) {
        let n = length(d)
        var pos = 0
        while (pos < n) {
            let comma = find(d, ",", pos)
            let part = strip(comma >= 0 ? slice(d, pos, comma) : slice(d, pos))
            pos = comma >= 0 ? comma + 1 : n
            if (!empty(part) && !check_single_constraint(ver, part)) {
                ok = false
                break
            }
        }
    }
    return ok
}

// ── .gitignore management ───────────────────────────────────────────

def gitignore_add(root, pattern : string) {
    // skip if not a git repository
    if (!fexist("{root}/.git")) return
    let gitignore_path = "{root}/.gitignore"
    // read existing content and check for duplicates
    var content = ""
    fopen(gitignore_path, "rb") $(f) {
        if (f != null) {
            content = fread(f)
        }
    }
    // check if pattern already present (exact line match)
    var found = false
    let plen = length(pattern)
    peek_data(content) $(d) {
        let n = length(d)
        var pos = 0
        while (pos < n) {
            let nl = find(d, "\n", pos)
            var stop = nl >= 0 ? nl : n
            // strip trailing \r
            if (stop > pos && int(d[stop - 1]) == '\r') {
                stop--
            }
            // same length + same prefix is an exact line match, with no window to allocate
            if (stop - pos == plen && starts_with(d, pos, pattern)) {
                found = true
                break
            }
            pos = nl >= 0 ? nl + 1 : n
        }
    }
    if (found) return
    // append pattern
    fopen(gitignore_path, "ab") $(f) {
        if (f != null) {
            // ensure we start on a new line
            if (!empty(content) && !ends_with(content, "\n")) {
                fprint(f, "\n")
            }
            fprint(f, "{pattern}\n")
        }
    }
    log_debug("added '{pattern}' to {gitignore_path}")
}

def gitignore_remove(root, pattern : string) {
    if (!fexist("{root}/.git")) return
    let gitignore_path = "{root}/.gitignore"
    var content = ""
    fopen(gitignore_path, "rb") $(f) {
        if (f != null) {
            content = fread(f)
        }
    }
    if (empty(content)) return
    // rebuild without the matching line
    var removed = false
    let plen = length(pattern)
    let result = build_string() $(var writer) {
        peek_data(content) $(d) {
            let n = length(d)
            var pos = 0
            while (pos < n) {
                let nl = find(d, "\n", pos)
                let eol = nl >= 0 ? nl : n
                var stop = eol
                if (stop > pos && int(d[stop - 1]) == '\r') {
                    stop--
                }
                if (stop - pos == plen && starts_with(d, pos, pattern)) {
                    removed = true
                } else {
                    // kept lines keep their own \r; the \n only when the source had one
                    writer |> write_string(d, pos, eol)
                    if (nl >= 0) {
                        writer |> write_char('\n')
                    }
                }
                pos = nl >= 0 ? nl + 1 : n
            }
        }
    }
    if (removed) {
        fopen(gitignore_path, "wb") $(f) {
            if (f != null) {
                fprint(f, result)
            }
        }
        log_debug("removed '{pattern}' from {gitignore_path}")
    }
}

// a package is its files, not its history or its build residue: .git costs more than
// the rest of the tree, and a copied build dir (_build or build) carries a CMakeCache
// pointing at the source path, which aborts the installed package's own configure
def private is_package_content(name : string) : bool {
    return name != "." && name != ".." && name != ".git" && name != "_build" && name != "build"
}

// streamed through a fixed window: sizing a buffer by the file costs that file in
// memory, and panics outright on one past max_unreserved_size
def private copy_file_bytes(src, dst : string) : bool {
    var ok = true
    fopen(src, "rb") $(fin) {
        if (fin == null) {
            ok = false
            return
        }
        fopen(dst, "wb") $(fout) {
            if (fout == null) {
                ok = false
                return
            }
            var buf : array<uint8>
            buf |> resize(COPY_WINDOW_BYTES)
            while (true) {
                let n = fread(fin, buf)
                break if (n <= 0)
                if (n < length(buf)) {
                    buf |> resize(n)
                }
                if (fwrite(fout, buf) != length(buf)) {
                    ok = false
                    break
                }
                break if (n < COPY_WINDOW_BYTES)
            }
            // a mid-file read error returns a short count exactly like EOF - feof is the tiebreaker
            if (ok && !feof(fin)) {
                ok = false
            }
            delete buf
        }
    }
    return ok
}

def copy_dir_rec(src, dst : string) : bool {
    mkdir(dst)
    var ok = true
    dir(src) $(name) {
        return if (!is_package_content(name))
        let src_child = "{src}/{name}"
        let dst_child = "{dst}/{name}"
        // never follow a junction/symlink: one into the tree recurses forever, and one
        // out of it copies content that is not the package's
        var link_err : string
        return if (is_reparse_point(src_child, link_err))
        let st = stat(src_child)
        if (st.is_dir) {
            if (!copy_dir_rec(src_child, dst_child)) {
                ok = false
            }
        } elif (st.is_reg) {
            if (!copy_file_bytes(src_child, dst_child)) {
                ok = false
            }
        } else {
            // stat failed, or the entry is neither file nor dir - a copy that silently
            // dropped an entry must not report itself complete
            to_log(LOG_ERROR, "Error: cannot stat {src_child} (or unsupported entry kind)\n")
            ok = false
        }
    }
    return ok
}
