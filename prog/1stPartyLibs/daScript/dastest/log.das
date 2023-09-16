options indenting = 4

module log shared

require rtti
require uriparser
require strings


let private verbose = false
let private uriPaths = false

var useTtyColors = false

def info_raw(msg: string)
    print("{msg}")

def info(msg: string)
    if !verbose
        print("{msg}\n")
    else
        print("{msg} @{file_info_hr(get_line_info(1), uriPaths)}\n")


def warn(msg: string)
    if !verbose
        print(yellow_str("[W] {msg}\n"))
    else
        print(yellow_str("[W] {msg} @{file_info_hr(get_line_info(1), uriPaths)}\n"))


def error(msg: string)
    if !verbose
        print(red_str("[E] {msg}\n"))
    else
        print(red_str("[E] {msg} @{file_info_hr(get_line_info(1), uriPaths)}\n"))


def green(msg: string)
    if !verbose
        print(green_str("{msg}\n"))
    else
        print(green_str("{msg} @{file_info_hr(get_line_info(1), uriPaths)}\n"))


def red(msg: string)
    if !verbose
        print(red_str("{msg}\n"))
    else
        print(red_str("{msg} @{file_info_hr(get_line_info(1), uriPaths)}\n"))


def blue(msg: string)
    if !verbose
        print(blue_str("{msg}\n"))
    else
        print(blue_str("{msg} @{file_info_hr(get_line_info(1), uriPaths)}\n"))


def file_info_hr(at: LineInfo; uri_path: bool)
    return (uri_path
        ? "{file_name_to_uri(at.fileInfo != null ? string(at.fileInfo.name) : "")}#{int(at.line)}"
        : "{at.fileInfo != null ? string(at.fileInfo.name) : ""}:{int(at.line)}"
    )


def private red_str(str: string)
    return useTtyColors ? "\x1B[31m{str}\x1B[0m" : str
def private green_str(str: string)
    return useTtyColors ? "\x1B[32m{str}\x1B[0m" : str
def private yellow_str(str: string)
    return useTtyColors ? "\x1B[33m{str}\x1B[0m" : str
def private blue_str(str: string)
    return useTtyColors ? "\x1B[34m{str}\x1B[0m" : str

def time_dt_hr(dt: int): string
    return build_string <| $(str)
        str |> write("(")
        str |> format("%.6f", double(dt)/1000000.0lf)
        str |> write("s)")

def init_log()
    init_log(get_command_line_arguments())


def init_log(args: array<string>)
    log::useTtyColors = args |> has_value("--color")
