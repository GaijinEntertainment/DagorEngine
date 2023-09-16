options indenting = 4

module fs shared

require fio
require strings
require uriparser


def private ends_with_separator(str: string): bool
    for c in "\\/"
        if ends_with(str, to_char(c))
            return true
    return false


def private starts_with_separator(str: string): bool
    for c in "\\/"
        if starts_with(str, to_char(c))
            return true
    return false


def private trim_path(path: string): string
    if path == "."
        return ""
    if path |> starts_with("./") || path |> starts_with(".\\")
        return path |> slice(2)
    return path


def join_path(a, path_b: string): string
    let b = trim_path(path_b)
    if length(a) == 0
        return fix_path(b)
    if length(b) == 0
        return fix_path(a)
    var res = build_string() <| $(builder)
        builder |> write(a)
        let ends = ends_with_separator(a)
        let starts = starts_with_separator(b)
        if ends && starts
            builder |> write(slice(b, 1))
        elif !ends && !starts
            builder |> write("/")
            builder |> write(b)
        else
            builder |> write(b)
    return fix_path(res)


def fix_path(path: string): string
    return path |> trim_path() |> file_name_to_uri() |> normalize_uri() |> uri_to_file_name()


def scan_dir(path: string; var cache: table<string; void?>; var res: array<string>; suffix = ".das"): bool
    let dirStat = stat(path)
    if !dirStat.is_dir
        return false
    fio::dir(path) <| $(n)
        if n == "." || n == ".."
            return
        let f = path |> join_path(n)
        let fStat = stat(f)
        if !fStat.is_valid
            return
        if fStat.is_dir
            f |> scan_dir(cache, res)
        elif fStat.is_reg && f |> ends_with(suffix) && !cache |> key_exists(f)
            cache[f] = null
            res |> push(f)
    return true
