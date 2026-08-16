options gen2
options indenting = 4
options remove_unused_symbols = false

// Make it shared, so other shared modules can include it.
module fio shared

require fio_core public
require strings
require math

typedef file = FILE const?


[generic]
def fopen(name : string; mode : string; blk : block<(f : file) : void>) {
    let f = fopen(name, mode)
    invoke(blk, f)
    if (f != null) {
        fclose(f)
    }
}

[generic]
def dir(path : string; blk : block<(filename : string) : void>) {
    builtin_dir(path, blk)
}

[generic]
def stat(path : string) : FStat {
    var fs : FStat
    if (stat(path, fs)) {
        fs.is_valid = true
    }
    return fs
}

[generic]
def fstat(f : file) : FStat {
    var fs : FStat
    if (fstat(f, fs)) {
        fs.is_valid = true
    }
    return fs
}

[generic]
def fread(f : file; blk : block<(data : string const#)>) {
    var stmp = fread(f)
    unsafe {
        invoke(blk, reinterpret<string#>(stmp))
        delete_string(stmp)
    }
}

let {
    df_magic = 0x12345678
}

struct df_header {
    magic : uint
    size : int
}

[generic]
def fload(file : file; size : int64; blk : block<(data : array<uint8>) : void>) : void {
    _builtin_load(file, size, blk)
}

[generic]
def fload(f : file; buf : auto(BufType) -const) {
    var dfh : df_header
    let r1 = _builtin_read(f, dfh, typeinfo sizeof(type<df_header>))
    if (r1 < 0 || dfh.magic != df_magic || dfh.size < 0) return false
    var loaded = false
    _builtin_load(f, int64(dfh.size)) $(data : array<uint8>) {
        if (length(data) != 0) {
            binary_load(buf, data)
            loaded = true
        }
    }
    return loaded
}

[generic]
def fsave(f : file; buf : auto(BufType) const) {
    var r1, r2 : int
    binary_save(buf) $(data) {
        let len64 = long_length(data)
        if (len64 > int64(INT_MAX)) {
            panic("fsave: {len64}-byte payload does not fit the df_header int size field")
        }
        let len = int(len64)
        let dfh = df_header(magic = df_magic, size = len)
        r1 = _builtin_write(f, dfh, typeinfo sizeof(type<df_header>))
        if (len > 0) {   // addr(data[0]) on an empty payload is out of bounds
            unsafe {
                r2 = _builtin_write(f, addr(data[0]), len)
            }
        }
    }
    if (r1 < 0) return r1
    if (r2 < 0) return r2
    return (r1 + r2)
}

[generic]
def fread(f : file; buf : auto(BufType) const implicit) {
    concept_assert(typeinfo is_raw(buf), "'fread' accepts only raw POD types")
    unsafe {
        return _builtin_read(f, addr(buf), typeinfo sizeof(buf))
    }
}

[generic]
def fread(f : file; buf : array<auto(BufType)> const implicit) {
    concept_assert(typeinfo is_raw(type<BufType>), "'fread' accepts only raw POD type arrays")
    return 0 if (empty(buf))   // a 0-byte read; addr(buf[0]) on an empty array is out of bounds
    let nbytes = long_length(buf) * typeinfo sizeof(type<BufType>)
    if (nbytes > int64(INT_MAX)) {
        panic("fread: {nbytes} bytes does not fit the int byte count - use long_fread")
    }
    unsafe {
        return _builtin_read(f, addr(buf[0]), int(nbytes))
    }
}

//! 64-bit fread: fills the whole array regardless of size (fread panics past 2 GiB).
//! Returns bytes read as int64.
[generic]
def long_fread(f : file; buf : array<auto(BufType)> const implicit) : int64 {
    concept_assert(typeinfo is_raw(type<BufType>), "'long_fread' accepts only raw POD type arrays")
    return 0l if (empty(buf))   // a 0-byte read; addr(buf[0]) on an empty array is out of bounds
    unsafe {
        return _builtin_read64(f, addr(buf[0]), long_length(buf) * typeinfo sizeof(type<BufType>))
    }
}

[generic]
def fwrite(f : file; buf : auto(BufType) const implicit) {
    concept_assert(typeinfo is_raw(buf), "'fwrite' accepts only raw POD types")
    unsafe {
        return _builtin_write(f, addr(buf), typeinfo sizeof(buf))
    }
}

[generic]
def fwrite(f : file; buf : array<auto(BufType)> const implicit) {
    concept_assert(typeinfo is_raw(type<BufType>), "'fwrite' accepts only raw POD type arrays")
    return 0 if (empty(buf))   // a 0-byte write; addr(buf[0]) on an empty array is out of bounds
    let nbytes = long_length(buf) * typeinfo sizeof(type<BufType>)
    if (nbytes > int64(INT_MAX)) {
        panic("fwrite: {nbytes} bytes does not fit the int byte count - use long_fwrite")
    }
    unsafe {
        return _builtin_write(f, addr(buf[0]), int(nbytes))
    }
}

//! 64-bit fwrite: writes the whole array regardless of size (fwrite panics past 2 GiB).
//! Returns bytes written as int64.
[generic]
def long_fwrite(f : file; buf : array<auto(BufType)> const implicit) : int64 {
    concept_assert(typeinfo is_raw(type<BufType>), "'long_fwrite' accepts only raw POD type arrays")
    return 0l if (empty(buf))   // a 0-byte write; addr(buf[0]) on an empty array is out of bounds
    unsafe {
        return _builtin_write64(f, addr(buf[0]), long_length(buf) * typeinfo sizeof(type<BufType>))
    }
}

[generic]
def mkdir_rec(path : string) : bool {
    return true if (path == "")
    var st : FStat
    return true if (stat(path, st) && st.is_dir)
    let parent = dir_name(path)
    return true if (parent == path)   // reached a root (e.g. a Windows drive root "E:") — nothing above to create
    mkdir_rec(parent)
    return mkdir(path)
}

[generic]
def mkdir_rec(path : string; var error : string&) : bool {
    return true if (path == "")
    var st : FStat
    return true if (stat(path, st) && st.is_dir)
    let parent = dir_name(path)
    return true if (parent == path)   // reached a root (e.g. a Windows drive root "E:") — nothing above to create
    if (!mkdir_rec(parent, error)) return false
    return mkdir(path, error)
}

// convenience: read entire file by path, returns empty string on failure
[generic]
def fread(path : string) : string {
    var result : string
    fopen(path, "rb") $(f) {
        if (f != null) {
            result = fread(f)
        }
    }
    return result
}

// convenience: write string to file by path, returns true on success
[generic]
def fwrite(path : string; text : string) : bool {
    var ok = false
    fopen(path, "wb") $(f) {
        if (f != null) {
            fwrite(f, text)
            ok = true
        }
    }
    return ok
}

// recursive directory iteration
[generic]
def dir_rec(path : string; blk : block<(filename : string; is_dir : bool) : void>) {
    var error : string
    builtin_dir_rec(path, blk, error)
}

[generic]
def dir_rec(path : string; blk : block<(filename : string; is_dir : bool) : void>; var error : string&) {
    builtin_dir_rec(path, blk, error)
}

// ===== glob =====

// `[...]` character-class arm of the glob matcher: matches one source byte and
// advances pi past the class (and si past the byte) on success.
def private _match_glob_class(p : array<uint8> const#; var pi : int&; s : array<uint8> const#; var si : int&) : bool {
    let plen = length(p)
    let slen = length(s)
    // POSIX fnmatch: `[` without a closing `]` matches the literal `[`.
    var scan = pi + 1
    if (scan < plen && (p[scan] == uint8('!') || p[scan] == uint8('^'))) {
        scan++
    }
    // A `]` immediately after `[` or `[!`/`[^` is a literal class member, not the terminator (POSIX rule for closing brackets in classes).
    if (scan < plen && p[scan] == uint8(']')) {
        scan++
    }
    while (scan < plen && p[scan] != uint8(']')) {
        scan++
    }
    if (scan >= plen) {
        // No closing `]` — treat the `[` as a literal character.
        return false if (si >= slen || int(s[si]) != '[')
        pi++
        si++
        return true
    }
    return false if (si >= slen)
    let sc = int(s[si])
    return false if (sc == '/')
    pi++
    var negate = false
    if (pi < plen && (p[pi] == uint8('!') || p[pi] == uint8('^'))) {
        negate = true
        pi++
    }
    var matched = false
    // POSIX: `]` right after `[` (or `[!`/`[^`) is a literal class member, not the terminator.
    if (pi < plen && p[pi] == uint8(']')) {
        if (sc == ']') {
            matched = true
        }
        pi++
    }
    while (pi < plen && p[pi] != uint8(']')) {
        if (pi + 2 < plen && p[pi + 1] == uint8('-') && p[pi + 2] != uint8(']')) {
            let lo = int(p[pi])
            let hi = int(p[pi + 2])
            if (sc >= lo && sc <= hi) {
                matched = true
            }
            pi += 3
        } else {
            if (sc == int(p[pi])) {
                matched = true
            }
            pi++
        }
    }
    if (pi < plen) {
        pi++
    }
    return false if (matched == negate)
    si++
    return true
}

// Byte-level pathname-glob matcher. `/` is a hard segment boundary that single
// `*` and `?` may not cross; `**` matches across `/`.
def private _match_glob_bytes(p : array<uint8> const#; var pi : int; s : array<uint8> const#; var si : int) : bool {
    let plen = length(p)
    let slen = length(s)
    while (pi < plen) {
        let pc = int(p[pi])
        if (pc == '*') {
            let is_double = (pi + 1 < plen && p[pi + 1] == uint8('*'))
            if (is_double) {
                // `**/` semantics: the `/` is optional only when `**` matches empty. Otherwise `**` must consume one or more segments ending in `/`, so the source must have a `/` boundary.
                if (pi + 2 < plen && p[pi + 2] == uint8('/')) {
                    if (_match_glob_bytes(p, pi + 3, s, si)) return true
                    var k = si
                    while (k < slen) {
                        if (s[k] == uint8('/') && _match_glob_bytes(p, pi + 3, s, k + 1)) return true
                        k++
                    }
                    return false
                }
                pi += 2
                var k = si
                while (k <= slen) {
                    if (_match_glob_bytes(p, pi, s, k)) return true
                    k++
                }
                return false
            } else {
                pi++
                var k = si
                while (k <= slen) {
                    if (_match_glob_bytes(p, pi, s, k)) return true
                    return false if (k < slen && s[k] == uint8('/'))
                    k++
                }
                return false
            }
        } elif (pc == '?') {
            return false if (si >= slen || s[si] == uint8('/'))
            pi++
            si++
        } elif (pc == '[') {
            return false if (!_match_glob_class(p, pi, s, si))
        } else {
            return false if (si >= slen || int(s[si]) != pc)
            pi++
            si++
        }
    }
    return si == slen
}

//! Match a pathname against a shell-style glob.
def match_glob(pattern : string; path : string) : bool {
    let plen = length(pattern)
    let slen = length(path)
    return true if (plen == 0 && slen == 0)
    return false if (plen == 0)
    // Empty path matches a pattern only when every char is `*` (any number of stars or **).
    if (slen == 0) return empty(replace(pattern, "*", ""))
    var result = false
    peek_data(pattern) $(parr) {
        peek_data(path) $(sarr) {
            result = _match_glob_bytes(parr, 0, sarr, 0)
        }
    }
    return result
}

//! Convert a path to use forward slashes regardless of host OS.
[temp_string_result]
def to_generic_path(s : string) : string {
    return replace(s, "\\", "/")
}

//! Walk `root` recursively and invoke `blk` for every entry whose path (relative to `root`) matches `pattern`. Both the matched path and the path passed to `blk` are normalized via `to_generic_path` so callbacks receive stable `/`-separated paths regardless of host.
[generic]
def glob(root : string; pattern : string; blk : block<(filename : string; is_dir : bool) : void>) {
    dir_rec(root) $(filename, is_dir) {
        let normalized = to_generic_path(filename)
        if (match_glob(pattern, normalized)) {
            invoke(blk, normalized, is_dir)
        }
    }
}

//! Walk `root` and invoke `blk` for entries that match at least one of `includes`
//! and none of `excludes`. Excludes win on conflict. Paths passed to `blk` are
//! normalized via `to_generic_path` (forward slashes regardless of host OS).
[generic]
def glob_filtered(root : string; includes : array<string>; excludes : array<string>;
                  blk : block<(filename : string; is_dir : bool) : void>) {
    dir_rec(root) $(filename, is_dir) {
        let normalized = to_generic_path(filename)
        var included = false
        for (pat in includes) {
            if (match_glob(pat, normalized)) {
                included = true
                break
            }
        }
        if (included) {
            for (pat in excludes) {
                if (match_glob(pat, normalized)) {
                    included = false
                    break
                }
            }
        }
        if (included) {
            invoke(blk, normalized, is_dir)
        }
    }
}

//! Returns true if `pattern` contains any glob metacharacter (`*`, `?`, `[`).
//! Used by callers that treat literal patterns differently from glob patterns.
def is_glob_pattern(pattern : string) : bool {
    return find(pattern, '*') >= 0 || find(pattern, '?') >= 0 || find(pattern, '[') >= 0
}

//! Expand a single glob pattern (e.g. `src/**/*.das`) into a sorted list of matching file paths.
def expand_glob(pattern : string; var result : array<string>) {
    let normalized_pattern = to_generic_path(pattern)
    let split = _split_literal_prefix(normalized_pattern)
    let dir_part = split._0
    let file_pattern = split._1
    // Pattern is already `/`-normalized (see normalize above), so no need to also check for `\` here.
    let recursive = (find(file_pattern, "**") >= 0
                     || find(file_pattern, "/") >= 0)
    var matches : array<string>
    if (recursive) {
        glob(dir_part, file_pattern) $(fname, is_dir) {
            if (is_dir) return
            matches |> push(_join_path(dir_part, fname))
        }
    } else {
        // Shallow walk: `dir(...)` yields names without an `is_dir` flag, so stat each match
        // and skip directories — callers expect file paths only.
        dir(dir_part) $(fname) {
            if (fname == "." || fname == "..") return
            if (match_glob(file_pattern, fname)) {
                let full = _join_path(dir_part, fname)
                let st = stat(full)
                if (st.is_valid && st.is_dir) return
                matches |> push(full)
            }
        }
    }
    sort(matches)
    result |> push_from(matches)
}

// Join `dir_part` and `fname` with `/`, but skip the inserted separator when `dir_part`
// is `.` or already ends with `/` or `\` (filesystem root, drive root).
def private _join_path(dir_part : string; fname : string) : string {
    if (dir_part == ".") return fname
    if (dir_part |> ends_with("/") || dir_part |> ends_with("\\")) return "{dir_part}{fname}"
    return "{dir_part}/{fname}"
}

// Split a pattern into (literal_dir, glob_remainder) at the last separator before the first
// glob metacharacter. Pattern with no metacharacters splits at its last separator. Returned
// dir_part is "." when no separator precedes the meta.
def private _split_literal_prefix(pattern : string) : tuple<string; string> {
    var first_meta = find(pattern, '*')
    let qmark = find(pattern, '?')
    if (qmark >= 0 && (first_meta < 0 || qmark < first_meta)) {
        first_meta = qmark
    }
    let lbrack = find(pattern, '[')
    if (lbrack >= 0 && (first_meta < 0 || lbrack < first_meta)) {
        first_meta = lbrack
    }
    let scan_end = first_meta < 0 ? length(pattern) : first_meta
    let prefix = slice(pattern, 0, scan_end)
    let sep_fwd = rfind(prefix, "/")
    let sep_back = rfind(prefix, "\\")
    let sep = max(sep_fwd, sep_back)
    if (sep < 0) return "." => pattern
    let raw_dir = slice(pattern, 0, sep)
    let raw_sep = slice(pattern, sep, sep + 1)
    let remainder = slice(pattern, sep + 1)
    // POSIX absolute root: pattern starts with `/` (or `\`). Empty prefix would tell `glob`/`dir` to walk an empty path; keep the separator.
    if (empty(raw_dir)) return raw_sep => remainder
    // Windows drive root like `C:` (or `C:/foo` after normalization with sep at index 2).
    if (length(raw_dir) == 2 && raw_dir |> ends_with(":")) return "{raw_dir}{raw_sep}" => remainder
    return raw_dir => remainder
}

//! Parse a comma- or newline-separated list of files / dirs / globs into a flat list of paths.
def parse_file_list(file : string; var result : array<string>) {
    if (find(file, ",") < 0 && find(file, "\n") < 0) {
        let part = strip(file)
        if (empty(part)) return
        if (is_glob_pattern(part)) {
            expand_glob(part, result)
        } else {
            result |> push(part)
        }
        return
    }
    builtin_string_split_by_char(file, ",\n") $(arr : array<string>#) : void {
        for (raw in arr) {
            let part = strip(raw)
            if (empty(part)) continue
            if (is_glob_pattern(part)) {
                expand_glob(part, result)
            } else {
                result |> push(part)
            }
        }
    }
}

// convenience: disk_space returning struct
[generic]
def disk_space(path : string) : DiskSpaceInfo {
    var info = DiskSpaceInfo()
    var error : string
    builtin_disk_space(path, info, error)
    return info
}

[generic]
def disk_space(path : string; var error : string&) : DiskSpaceInfo {
    var info = DiskSpaceInfo()
    builtin_disk_space(path, info, error)
    return info
}

// ---- result variants for error reporting ----

variant fs_result_bool {
    value : bool;
    error : string;
}

variant fs_result_int64 {
    value : int64;
    error : string;
}

variant fs_result_string {
    value : string;
    error : string;
}

// ---- _result wrappers ----

[generic]
def file_size_result(path : string) : fs_result_int64 {
    var error : string
    let sz = file_size(path, error)
    if (!empty(error)) return fs_result_int64(error = error)
    return fs_result_int64(value = sz)
}

[generic]
def equivalent_result(a : string; b : string) : fs_result_bool {
    var error : string
    let res = equivalent(a, b, error)
    if (!empty(error)) return fs_result_bool(error = error)
    return fs_result_bool(value = res)
}

[generic]
def is_symlink_result(path : string) : fs_result_bool {
    var error : string
    let res = is_symlink(path, error)
    if (!empty(error)) return fs_result_bool(error = error)
    return fs_result_bool(value = res)
}

[generic]
def copy_file_result(src : string; dst : string; overwrite : bool) : fs_result_bool {
    var error : string
    let res = copy_file(src, dst, overwrite, error)
    if (!empty(error)) return fs_result_bool(error = error)
    return fs_result_bool(value = res)
}

[generic]
def set_mtime_result(path : string; time : clock) : fs_result_bool {  // clock = the builtin Time handled type ("Time" resolves to nothing — this overload was uncallable)
    var error : string
    let res = set_mtime(path, time, error)
    if (!empty(error)) return fs_result_bool(error = error)
    return fs_result_bool(value = res)
}

[generic]
def temp_directory_result() : fs_result_string {
    var error : string
    let res = temp_directory(error)
    if (!empty(error)) return fs_result_string(error = error)
    return fs_result_string(value = res)
}

[generic]
def create_temp_file_result(prefix : string; ext : string) : fs_result_string {
    var error : string
    let res = create_temp_file(prefix, ext, error)
    if (!empty(error)) return fs_result_string(error = error)
    return fs_result_string(value = res)
}

[generic]
def create_temp_directory_result(prefix : string) : fs_result_string {
    var error : string
    let res = create_temp_directory(prefix, error)
    if (!empty(error)) return fs_result_string(error = error)
    return fs_result_string(value = res)
}

[generic]
def relative_result(path : string; base : string) : fs_result_string {
    var error : string
    let res = relative(path, base, error)
    if (!empty(error)) return fs_result_string(error = error)
    return fs_result_string(value = res)
}

[generic]
def remove_result(path : string) : fs_result_bool {
    var error : string
    let res = remove(path, error)
    if (!empty(error)) return fs_result_bool(error = error)
    return fs_result_bool(value = res)
}

[generic]
def rename_result(old_name : string; new_name : string) : fs_result_bool {
    var error : string
    let res = rename(old_name, new_name, error)
    if (!empty(error)) return fs_result_bool(error = error)
    return fs_result_bool(value = res)
}

[generic]
def mkdir_result(path : string) : fs_result_bool {
    var error : string
    let res = mkdir(path, error)
    if (!empty(error)) return fs_result_bool(error = error)
    return fs_result_bool(value = res)
}

[generic]
def rmdir_result(path : string) : fs_result_bool {
    var error : string
    let res = rmdir(path, error)
    if (!empty(error)) return fs_result_bool(error = error)
    return fs_result_bool(value = res)
}

[generic]
def rmdir_rec_result(path : string) : fs_result_bool {
    var error : string
    let res = rmdir_rec(path, error)
    if (!empty(error)) return fs_result_bool(error = error)
    return fs_result_bool(value = res)
}

def run_and_capture(args : array<string>; var output : string&; timeout_sec : float = 0.0) : int {
    //! Run an external command and capture its stdout+stderr (merged into one pipe by the underlying ``popen_argv``). Returns the process exit code;
    //! -1 means the spawn itself failed. No shell is involved, but on Windows the program path (``args[0]``) must use backslashes —
    //! CreateProcess does not resolve ``bin/daslang``-style forward-slash relative paths (probe-verified); ``replace(exe, "/", "\\")`` first.
    var captured : string
    let exit_code = unsafe(popen_argv(args, timeout_sec, $(f) {
        if (f != null) {
            captured = fread(f)
        }
    }))
    output = captured
    return exit_code
}
