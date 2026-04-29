options gen2
require daslib/aot_cpp
require daslib/aot_standalone

require daslib/fio
require daslib/strings_boost

require daslib/rtti


def updateCOP(var cop : CodeOfPolicies; gen2 : bool; gen2_make : bool; aot_lib : bool) {
    cop.threadlock_context = false
    cop.aot = false;
    cop.aot_module = true;
    cop.aot_lib = aot_lib;
    cop.fail_on_lack_of_aot_export = true;
    cop.ignore_shared_modules = false;
    cop.version_2_syntax = gen2;
    cop.gen2_make_syntax = gen2_make;
}


def find_argument_or(args : array<string>; key : string, def_val : string) {
    for (i in range(length(args) - 1)) {
        if (args[i] == key) {
            return args[i + 1]
        }
    }
    return def_val
}

def find_argument(args : array<string>; key : string) {
    for (i in range(length(args) - 1)) {
        if (args[i] == key) {
            return args[i + 1]
        }
    }
    panic("expected {key} argument!")
    return ""
}

def get_list_of_files(args : array<string>, key : string) {
    let len = length(args)
    var result = array<tuple<string; string>>()
    for (i in range(len)) {
        if (args[i] == key) {
            if (i + 2 >= len) {
                panic("Arguments expected in format `-aot <in> <out>`.")
            }
            result |> push((args[i + 1], args[i + 2]))
        }
    }
    return <- result
}


def private normalize_line_endings(text : string) : string {
    // Convert all line endings to LF for consistent comparison
    var result = text
    result = replace(result, "\r\n", "\n")  // Windows CRLF -> LF
    result = replace(result, "\r", "\n")    // Old Mac CR -> LF
    return result
}

def private is_content_same(content : string; filename : string) : bool {
    var is_same = false
    fopen(filename, "rb") <| $(fr) {
        if (fr == null) {
            is_same = false
            return
        }
        fread(fr) <| $(data) {
            is_same = content == data
        }
    }
    return is_same
}


def parse_input_file(filename : string) {
    var aot_files = array<tuple<string; string>>()
    var aot_lib_files = array<tuple<string; string>>()
    var ctx_files = array<tuple<string; string>>()
    fopen(filename, "rb") <| $(fr) {
        if (fr == null) {
            print("Couldn't open input file {filename}\n")
        }
        fread(fr) <| $(data) {
            for (line in normalize_line_endings(string(data)) |> split("\n")) {
                if (line.empty()) {
                    continue
                }
                // aot/ctx in.das out.cpp
                let parts = line |> split(" ")
                let mode = parts[0]
                if (mode == "aot") {
                    aot_files |> push((parts[1], parts[2]))
                } elif (mode == "aotlib") {
                    aot_lib_files |> push((parts[1], parts[2]))
                } elif (mode == "ctx") {
                    ctx_files |> push((parts[1], parts[2]))
                } else {
                    to_log(LOG_DEBUG, "Unexpected mode {mode}. Choose `aot` or `ctx`.\n")
                }
            }
        }
    }
    return <- (aot_files, aot_lib_files, ctx_files)
}

def find_bool(args : array<string>; key : string) {
    let idx = find_index(args, key)
    return idx >= 0 && idx + 1 < length(args)
}

def write_result(content : string; output_filename : string;
                 mode : string; force_overwrite : bool) {
    var result : bool = true
    if (content |> length() > 0 && (force_overwrite || !is_content_same(content, output_filename))) {
        fopen(output_filename, "wb") <| $(fw) {
            if (fw != null) {
                to_log(LOG_INFO, "{mode} to {output_filename}\n")
                fwrite(fw, content)
            } else {
                to_log(LOG_ERROR, "Couldn't create output file {output_filename}\n")
                result = false
            }
        }
    } else {
        to_log(LOG_INFO, "Content is same for {output_filename}\n")
    }
    return result
}

[export]
def main() {
    let args <- get_command_line_arguments()

    let cross_platform = find_bool(args, "-cross_platform");
    let gen1 = find_bool(args, "-gen1");
    let gen2_make = find_bool(args, "-gen2-make");
    let force_overwrite = find_bool(args, "-force-overwrite");
    let quiet = find_bool(args, "-quiet");

    var aot_files = array<tuple<string; string>>()
    var aotlib_files = array<tuple<string; string>>()
    var ctx_files = array<tuple<string; string>>()

    let input_file = find_argument_or(args, "-in_file", "")
    if (!input_file.empty()) {
        var (a1, a2, a3) <- parse_input_file(input_file)
        aot_files <- a1
        aotlib_files <- a2
        ctx_files <- a3
    } else {
        aot_files = get_list_of_files(args, "-aot")
        aotlib_files = get_list_of_files(args, "-aotlib")
        ctx_files = get_list_of_files(args, "-ctx")
    }
    var result = true
    using <| $(var cop : CodeOfPolicies) {
        updateCOP(cop, !gen1, gen2_make, false)
        for ((aot_in, aot_out) in aot_files) {
            let res = aot(aot_in, false, cross_platform, cop)
            if (res |> empty()) {
                to_log(LOG_INFO, "Failed to compile `{aot_in}` in standalone.\n")
                result = false
            } else {
                let written = write_result(res, aot_out, "Aot", force_overwrite)
                result = written && result
            }
        }
        updateCOP(cop, !gen1, gen2_make, true)
        for ((aot_in, aot_out) in aotlib_files) {
            let res = aot(aot_in, false, cross_platform, cop)
            if (res |> empty()) {
                to_log(LOG_INFO, "Failed to compile `{aot_in}` in aotlib.\n")
                result = false
            } else {
                let written = write_result(res, aot_out, "Aot library", force_overwrite)
                result = written && result
            }
        }
        updateCOP(cop, !gen1, gen2_make, false)
        for ((ctx_in, ctx_out) in ctx_files) {
            let is_ok = standalone_aot(ctx_in, ctx_out, cross_platform, false, cop)
            if (!is_ok && !quiet) {
                to_log(LOG_INFO, "Failed to compile `{ctx_in}` in standalone.\n")
            }
            result = is_ok && result
        }
    }
    if (!result) {
        unsafe {
            // Doing exit() may cause SIFO on complex files
            // with init, so return as usual if everything is ok.
            fio::exit(-1)
        }
    }
}
