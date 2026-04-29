require daslib/fio
require strings
require daslib/strings_boost

var colored_output = true

def cmd_exec(cmd : string) : string {
    var out = ""
    unsafe {
        popen(cmd) $(f) {
            out = build_string() $(var w) {
                while (!feof(f)) {
                    w |> write(fgets(f))
                }
            }
        }
    }
    return trim(out)
}

def file_exists(filename : string) : bool {
    return stat(filename).is_valid
}

def read_file(filename : string) : string {
    var result = ""
    fopen(filename, "r") $(f) {
        if (f != null) {
            result = fread(f)
        }
    }
    return result
}

def bare_str(str : string) : string {
    return replace_multiple(str, [
        ("\x1B[0m", ""),

        ("\x1B[31m", ""),
        ("\x1B[32m", ""),
        ("\x1B[33m", ""),
        ("\x1B[34m", ""),
    ])
}

def red_str(str : string) : string {
    return colored_output ? "\x1B[31m{str}\x1B[0m" : str
}

def green_str(str : string) : string {
    return colored_output ? "\x1B[32m{str}\x1B[0m" : str
}

def yellow_str(str : string) : string {
    return colored_output ? "\x1B[33m{str}\x1B[0m" : str
}

def blue_str(str : string) : string {
    return colored_output ? "\x1B[34m{str}\x1B[0m" : str
}
