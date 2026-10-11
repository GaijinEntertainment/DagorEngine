options gen2
options indenting = 4

module log shared

require daslib/rtti
require uriparser
require strings
require daslib/ansi_colors public


let private verbose = false
let private uriPaths = false

var useTtyColors = false
var failuresOnly = false

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

def started(msg : string | #) {
    to_log(LOG_INFO, "{iso8601_now()} {msg}\n")
}


//! prints a colored line with the newline outside the color wrap, so the closing reset
//! escape can never land at the head of the next stdout line (it corrupts machine-read
//! output such as `--bench-format json`)
def print_line(colored : string) {
    print(colored + "\n")
}


def warn(msg : string | #) {
    if (!verbose) {
        print_line(yellow_str("[W] {msg}"))
    } else {
        print_line(yellow_str("[W] {msg} @{file_info_hr(get_line_info(1), uriPaths)}"))
    }
}


def error(msg : string | #) {
    if (!verbose) {
        print_line(red_str("[E] {msg}"))
    } else {
        print_line(red_str("[E] {msg} @{file_info_hr(get_line_info(1), uriPaths)}"))
    }
}


def green(msg : string | #) {
    if (failuresOnly) return
    if (!verbose) {
        print_line(green_str("{msg}"))
    } else {
        print_line(green_str("{msg} @{file_info_hr(get_line_info(1), uriPaths)}"))
    }
}


def red(msg : string | #) {
    if (!verbose) {
        print_line(red_str("{msg}"))
    } else {
        print_line(red_str("{msg} @{file_info_hr(get_line_info(1), uriPaths)}"))
    }
}


def blue(msg : string | #) {
    if (failuresOnly) return
    if (!verbose) {
        print_line(blue_str("{msg}"))
    } else {
        print_line(blue_str("{msg} @{file_info_hr(get_line_info(1), uriPaths)}"))
    }
}


def file_info_hr(at : LineInfo; uri_path : bool) {
    return (uri_path
        ? "{file_name_to_uri(at.fileInfo != null ? string(at.fileInfo.name) : "")}#{int(at.line)}"
        : "{at.fileInfo != null ? string(at.fileInfo.name) : ""}:{int(at.line)}"
    )
}


def time_dt_hr(dt : int) : string {
    return time_dt_hr(int64(dt))
}


def time_dt_hr(dt : int64) : string {
    return build_string() $(str) {
        str |> write("(")
        str |> fmt(":.6f", double(dt) / 1000000.0lf)
        str |> write("s)")
    }
}

def init_log() {
    init_log(get_command_line_arguments())
}


def init_log(args : array<string>) {
    init_ansi_colors(args)
    useTtyColors = use_tty_colors
}
