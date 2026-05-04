options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_global_variables = false

module utils shared public

require daslib/fio
require strings

// ── Logging ──────────────────────────────────────────────────────────

var log_file_path : string
var log_verbose_flag = false

def log_init(root : string) {
    let modules_dir = "{root}/modules"
    mkdir(modules_dir)
    log_file_path = "{modules_dir}/.daspkg.log"
    fopen(log_file_path, "ab") <| $(f) {
        if (f != null) {
            fprint(f, "\n--- daspkg ---\n")
        }
    }
}

def private log_to_file(msg : string) {
    if (empty(log_file_path)) {
        return
    }
    fopen(log_file_path, "ab") <| $(f) {
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
    log_debug("$ {cmd}")
    let tmp_out = "{get_das_root()}/_daspkg_cmd_output.txt"
    let full_cmd = "{cmd} > \"{tmp_out}\" 2>&1"
    var exit_code = -1
    unsafe {
        exit_code = popen(full_cmd) <| $(f) {
            if (f != null) {
                fread(f)
            }
        }
    }
    fopen(tmp_out, "rb") <| $(f) {
        if (f != null) {
            output = fread(f)
        }
    }
    remove(tmp_out)
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
    if (!fexist(path)) {
        return
    }
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
    if (last_sep >= 0) {
        return slice(source, last_sep + 1)
    }
    return source
}

def is_local_path(source : string) : bool {
    if (starts_with(source, "/") || starts_with(source, "./") || starts_with(source, "../")) {
        return true
    }
    if (starts_with(source, ".\\") || starts_with(source, "..\\")) {
        return true
    }
    if (length(source) > 2 && character_at(source, 1) == ':') { // nolint:PERF003
        return true
    }
    return false
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
    if (a.major != b.major) {
        return a.major < b.major ? -1 : 1
    }
    if (a.minor != b.minor) {
        return a.minor < b.minor ? -1 : 1
    }
    if (a.patch != b.patch) {
        return a.patch < b.patch ? -1 : 1
    }
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
    if (!target.valid) {
        return false
    }
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
    if (empty(constraint)) {
        return true
    }
    let ver = parse_version(version_str)
    if (!ver.valid) {
        return false
    }
    // split on comma
    var rest = constraint
    while (!empty(rest)) {
        let comma = find(rest, ",")
        var part : string
        if (comma >= 0) {
            part = strip(slice(rest, 0, comma))
            rest = strip(slice(rest, comma + 1))
        } else {
            part = strip(rest)
            rest = ""
        }
        if (!empty(part) && !check_single_constraint(ver, part)) {
            return false
        }
    }
    return true
}

def copy_dir_rec(src, dst : string) : bool {
    mkdir(dst)
    var ok = true
    dir(src) <| $(name) {
        if (name == "." || name == "..") {
            return
        }
        let src_child = "{src}/{name}"
        let dst_child = "{dst}/{name}"
        var st = stat(src_child)
        if (st.is_dir) {
            if (!copy_dir_rec(src_child, dst_child)) {
                ok = false
            }
        } elif (st.is_reg) {
            fopen(src_child, "rb") <| $(fin) {
                if (fin != null) {
                    let fs = fstat(fin)
                    let sz = int(fs.size)
                    if (sz == 0) {
                        // empty file — just create it
                        fopen(dst_child, "wb") <| $(fout) {
                            if (fout == null) {
                                ok = false
                            }
                        }
                    } else {
                        var buf : array<uint8>
                        buf |> resize(sz)
                        fread(fin, buf)
                        fopen(dst_child, "wb") <| $(fout) {
                            if (fout != null) {
                                fwrite(fout, buf)
                            } else {
                                ok = false
                            }
                        }
                    }
                } else {
                    ok = false
                }
            }
        }
    }
    return ok
}
