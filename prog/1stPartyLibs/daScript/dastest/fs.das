options gen2
options indenting = 4

module fs shared

require daslib/fio
require strings
require uriparser


def private ends_with_separator(str : string) : bool {
    for (c in "\\/") {
        if (ends_with(str, to_char(c))) {
            return true
        }
    }
    return false
}


def private starts_with_separator(str : string) : bool {
    for (c in "\\/") {
        if (starts_with(str, to_char(c))) {
            return true
        }
    }
    return false
}


def private trim_path(path : string) : string {
    if (path == ".") {
        return ""
    }
    if (path |> starts_with("./") || path |> starts_with(".\\")) {
        return path |> slice(2)
    }
    return path
}


def join_path(a, path_b : string) : string {
    let b = trim_path(path_b)
    if (empty(a)) {
        return fix_path(b)
    }
    if (empty(b)) {
        return fix_path(a)
    }
    let res = build_string() $(builder) {
        builder |> write(a)
        let ends = ends_with_separator(a)
        let starts = starts_with_separator(b)
        if (ends && starts) {
            builder |> write(slice(b, 1))
        } elif (!ends && !starts) {
            builder |> write("/")
            builder |> write(b)
        } else {
            builder |> write(b)
        }
    }
    return fix_path(res)
}


def fix_path(path : string) : string {
    let uri = path |> trim_path() |> file_name_to_uri()
    // file_name_to_uri does NOT percent-encode the path, so a path containing a
    // space (or '#', '?', ...) produces an invalid URI that normalize_uri's
    // parser rejects, returning "" — which silently drops the path and makes
    // every file under such a directory fail to collect. uri_to_file_name itself
    // tolerates those characters, so fall back to the un-normalized round-trip
    // when normalization fails. Normal paths still normalize as before.
    let normalized = uri |> normalize_uri()
    return (empty(normalized) ? uri : normalized) |> uri_to_file_name()
}


def scan_dir(path : string; var cache : table<string; void?>; var res : array<string>; suffix = ".das") : bool {
    return scan_dir(path, cache, res, suffix) $(_n) {
        return true
    }
}

def scan_dir(path : string; var cache : table<string; void?>; var res : array<string>; suffix = ".das";
             dir_filter : block<(name : string) : bool>) : bool {
    let dirStat = stat(path)
    if (!dirStat.is_dir) {
        return false
    }
    fio::dir(path) $(n) {
        if (n == "." || n == "..") {
            return
        }
        let f = path |> join_path(n)
        let fStat = stat(f)
        if (!fStat.is_valid) {
            return
        }
        if (fStat.is_dir) {
            if (n |> starts_with("_") || !invoke(dir_filter, n)) {
                return
            }
            f |> scan_dir(cache, res, suffix, dir_filter)
        } elif (fStat.is_reg && f |> ends_with(suffix) && !(n |> starts_with("_")) && !cache |> key_exists(f)) {
            cache |> insert(f, null)
            res |> push(f)
        }
    }
    return true
}
