options gen2
options indenting = 4

module log shared

require rtti
require uriparser
require strings
require fio


let private verbose = false
let private uriPaths = false

var useTtyColors = false

def info_raw(msg : string | #) {
    print("{msg}")
}

def info(msg : string | #) {
    if (!verbose) {
        print("{msg}\n")
    } else {
        print("{msg} @{file_info_hr(get_line_info(1), uriPaths)}\n")
    }
}


def warn(msg : string | #) {
    if (!verbose) {
        print(yellow_str("[W] {msg}\n"))
    } else {
        print(yellow_str("[W] {msg} @{file_info_hr(get_line_info(1), uriPaths)}\n"))
    }
}


def error(msg : string | #) {
    if (!verbose) {
        print(red_str("[E] {msg}\n"))
    } else {
        print(red_str("[E] {msg} @{file_info_hr(get_line_info(1), uriPaths)}\n"))
    }
}


def green(msg : string | #) {
    if (!verbose) {
        print(green_str("{msg}\n"))
    } else {
        print(green_str("{msg} @{file_info_hr(get_line_info(1), uriPaths)}\n"))
    }
}


def red(msg : string | #) {
    if (!verbose) {
        print(red_str("{msg}\n"))
    } else {
        print(red_str("{msg} @{file_info_hr(get_line_info(1), uriPaths)}\n"))
    }
}


def blue(msg : string | #) {
    if (!verbose) {
        print(blue_str("{msg}\n"))
    } else {
        print(blue_str("{msg} @{file_info_hr(get_line_info(1), uriPaths)}\n"))
    }
}


def file_info_hr(at : LineInfo; uri_path : bool) {
    return (uri_path
        ? "{file_name_to_uri(at.fileInfo != null ? string(at.fileInfo.name) : "")}#{int(at.line)}"
        : "{at.fileInfo != null ? string(at.fileInfo.name) : ""}:{int(at.line)}"
    )
}


def red_str(str : string) {
    return useTtyColors ? "\x1B[31m{str}\x1B[0m" : str
}
def green_str(str : string) {
    return useTtyColors ? "\x1B[32m{str}\x1B[0m" : str
}
def yellow_str(str : string) {
    return useTtyColors ? "\x1B[33m{str}\x1B[0m" : str
}
def blue_str(str : string) {
    return useTtyColors ? "\x1B[34m{str}\x1B[0m" : str
}

def time_dt_hr(dt : int) : string {
    return build_string <| $(str) {
        str |> write("(")
        str |> fmt(":.6f", double(dt) / 1000000.0lf)
        str |> write("s)")
    }
}

def init_log() {
    init_log(get_command_line_arguments())
}


def init_log(args : array<string>) {
    let noColor = has_env_variable("NO_COLOR") ? get_env_variable("NO_COLOR") : "0"
    let forceNoColor = noColor != "0" && noColor != "false"
    if (forceNoColor || has_value(args, "--no-color")) {
        useTtyColors = false
    } else {
        let term = get_env_variable("TERM")
        let probabblyColor = length(term) > 3 && (find(term, "xterm") >= 0 || find(term, "color") >= 0 || find(term, "ansi") >= 0 || find(term, "vt100") >= 0)
        useTtyColors = has_value(args, "--color") || probabblyColor
    }
}
