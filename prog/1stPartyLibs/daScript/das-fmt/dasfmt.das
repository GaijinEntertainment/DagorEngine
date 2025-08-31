options gen2
options indenting = 4

require fio
require strings
require math

require fs
require log

require daslib/jobque_boost
require daslib/das_source_formatter
require daslib/strings_boost


def private normalize_line_endings(text : string) : string {
    // Convert all line endings to LF for consistent comparison
    var result = text
    result = replace(result, "\r\n", "\n")  // Windows CRLF -> LF
    result = replace(result, "\r", "\n")    // Old Mac CR -> LF
    return result
}


def private collect_diff(before, after : string) : tuple<differ : bool; log : string> {
    // Normalize line endings for comparison
    let normalized_before = normalize_line_endings(before)
    let normalized_after = normalize_line_endings(after)

    // If identical after normalization, no differences
    if (normalized_before == normalized_after) {
        return (false, "")
    }

    // Split into lines for line-by-line comparison
    let lines_before = split(normalized_before, "\n")
    let lines_after = split(normalized_after, "\n")
    var diff_output = ""

    let len_before = length(lines_before)
    let len_after = length(lines_after)
    let max_lines = max(len_before, len_after)

    for (i in range(max_lines)) {
        if (i >= len_before) {
            // Added lines
            diff_output += green_str("{i+1}a1\n> {lines_after[i]}\n")
        } elif (i >= len_after) {
            // Removed lines
            diff_output += red_str("{i+1}d1\n< {lines_before[i]}\n")
        } elif (lines_before[i] != lines_after[i]) {
            // Changed lines
            diff_output += (blue_str("{i+1}c1\n")
                        + red_str("< {lines_before[i]}\n")
                        + "---\n"
                        + green_str("> {lines_after[i]}\n"))
        }
    }

    return (true, diff_output)
}

def private collect_input_paths(args : array<string>; key : string; var paths : array<string>) {
    for (i in range(length(args) - 1)) {
        if (args[i] == key) {
            paths |> push <| args[i + 1]
        }
    }
}


def private collect_files(input_paths : array<string>; var files : array<string>) : bool {
    var cache : table<string; void?>
    for (path in input_paths) {
        if (path |> ends_with(".das")) {
            if (!cache |> key_exists(path)) {
                cache.insert(path, null)
                files |> push <| path
            }
        } else {
            if (!fs::scan_dir(path, cache, files, ".das")) {
                log::error("Unable to scan given path '{path}'")
                return false
            }
        }
    }
    delete cache
    return true
}


def private get_arg(args : array<string>; name : string; def_val : int = 0) : int {
    let idx = find_index(args, name)
    return idx >= 0 && idx + 1 < length(args) ? int(args[idx + 1]) : def_val
}


[export]
def main() {
    var args <- get_command_line_arguments()
    log::init_log(args)
    let threadsNum = max(args |> get_arg("--t") <| get_total_hw_threads(), 1)
    let verify = args |> find_index("--verify") >= 0
    let verbose = args |> find_index("--verbose") >= 0

    let startTime = ref_time_ticks()

    var inputPaths : array<string>
    collect_input_paths(args, "--path", inputPaths)
    var files : array<string>
    if (!collect_files(inputPaths, files)) {
        log::error("Unable to collect files list\n")
        unsafe {
            fio::exit(1)
        }
        return
    }

    var excludeMasks : array<string>
    collect_input_paths(args, "--exclude-mask", excludeMasks)
    var idx = length(files) - 1
    while (idx >= 0) {
        let path = files[idx]
        for (excludePath in excludeMasks) {
            if (path |> find(excludePath) >= 0) {
                log::info("Excluding file '{path}' mask '{excludePath}'")
                files |> erase(idx)
                break
            }
        }
        idx--
    }

    let filesNum = length(files)
    var verified = true

    with_job_que <| $() {
        while (length(files) > 0) {
            let batchNum = min(length(files), threadsNum)
            with_job_status(batchNum) <| $(status) {
                for (i in range(batchNum)) {
                    let file = files[length(files) - 1]
                    files |> pop()
                    unsafe {
                        new_job <| @ capture(& verified, := args) {
                            log::init_log(args)
                            var before : string
                            var after : string
                            var open = false
                            fopen(file, "rb") <| $(fr) {
                                if (fr != null) {
                                    fmap(fr) <| $(data) {
                                        open = true
                                        before = string(data)
                                        after = format_source(data)
                                    }
                                }
                            }
                            let diff_result = collect_diff(before, after)
                            let changed = diff_result.differ
                            if (!open) {
                                log::info("-- {file}")
                                log::error("Unable to open '{file}'")
                            } elif (verify) {
                                if (changed) {
                                    log::error("Unformatted file '{file}'")
                                    log::info(diff_result.log)
                                    verified = false
                                }
                            } elif (!changed) {
                                log::info("-- {file}")
                            } else {
                                var write = false
                                fopen(file, "wb") <| $(fw) {
                                    if (fw != null) {
                                        fwrite(fw, after)
                                        write = true
                                    }
                                }
                                if (write) {
                                    log::info("++ {file}")
                                    if (verbose) {
                                        log::info(diff_result.log)
                                    }
                                } else {
                                    log::info("-- {file}")
                                    log::error("Unable to open '{file}' for writing")
                                }
                            }
                            status |> notify_and_release
                        }
                    }
                }
                status |> join
            }
        }
    }

    if (verify) {
        if (verified) {
            log::info("Verified! {filesNum} files in {time_dt_hr(get_time_usec(startTime))}")
            return
        } else {
            log::info("Verification failed! {filesNum} files in {time_dt_hr(get_time_usec(startTime))}")
            unsafe {
                fio::exit(1)
            }
        }
    }

    log::info("Complete! {filesNum} files in {time_dt_hr(get_time_usec(startTime))}")
}

// options debugger
// require daslib/debug
