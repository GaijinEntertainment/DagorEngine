options gen2
options indenting = 4

require daslib/fio
require strings
require math

require fs
require log

require daslib/jobque_boost
require daslib/das_source_formatter
require daslib/strings_boost
require daslib/lint_config
require daslib/ast
require daslib/rtti


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
    if (normalized_before == normalized_after) return (differ = false, log = "")

    // Split into lines for line-by-line comparison
    let lines_before = split(normalized_before, "\n")
    let lines_after = split(normalized_after, "\n")
    let len_before = length(lines_before)
    let len_after = length(lines_after)
    let max_lines = max(len_before, len_after)

    let diff_output = build_string() $(w) {
        for (i in range(max_lines)) {
            if (i >= len_before) {
                // Added lines
                w |> write(green_str("{i+1}a1\n> {lines_after[i]}\n"))
            } elif (i >= len_after) {
                // Removed lines
                w |> write(red_str("{i+1}d1\n< {lines_before[i]}\n"))
            } elif (lines_before[i] != lines_after[i]) {
                // Changed lines
                w |> write(blue_str("{i+1}c1\n"))
                w |> write(red_str("< {lines_before[i]}\n"))
                w |> write("---\n")
                w |> write(green_str("> {lines_after[i]}\n"))
            }
        }
    }

    return (differ = true, log = diff_output)
}

def private collect_input_paths(args : array<string>; key : string; var paths : array<string>) {
    for (i in range(length(args) - 1)) {
        if (args[i] == key) {
            paths |> push <| args[i + 1]
        }
    }
}


def private collect_files(input_paths : array<string>; var cache : table<string; void?>; var files : array<string>) : bool {
    for (path in input_paths) {
        if (path |> ends_with(".das")) {
            if (!fexist(path)) {
                log::error("Unable to open '{path}'")
                return false
            }
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
    return true
}


def private collect_files_from_list(list_file : string; var cache : table<string; void?>; var files : array<string>) : bool {
    // Read a newline-separated list of file paths to format/verify.
    var opened = false
    fopen(list_file, "rb") $(fr) {
        if (fr != null) {
            opened = true   // file opened; an empty list is zero files, not an error
            fmap(fr) $(data) {
                for (raw_line in split(string(data), "\n")) {
                    let line = replace(raw_line, "\r", "")
                    if (!empty(line) && line |> ends_with(".das") && !cache |> key_exists(line)) {
                        cache.insert(line, null)
                        files |> push <| line
                    }
                }
            }
        }
    }
    if (!opened) log::error("Unable to open file list '{list_file}'")
    return opened
}


def private get_arg(args : array<string>; name : string; def_val : int = 0) : int {
    let idx = find_index(args, name)
    return idx >= 0 && idx + 1 < length(args) ? int(args[idx + 1]) : def_val
}


def private drop_excluded_files(args : array<string>; var files : array<string>) {
    var excludeMasks : array<string>
    collect_input_paths(args, "--exclude-mask", excludeMasks)
    var idx = length(files) - 1
    while (idx >= 0) {
        let path = files[idx] |> replace("\\", "/")
        for (rawMask in excludeMasks) {
            var excludePath = rawMask |> replace("\\", "/")
            if (excludePath |> starts_with("./")) {
                excludePath = slice(excludePath, 2)
            }
            continue if (empty(excludePath))
            if (path |> find(excludePath) >= 0) {
                log::info("Excluding file '{path}' mask '{excludePath}'")
                files |> erase(idx)
                break
            }
        }
        idx--
    }
}


def private stripped_source_compiles(file, text : string) : bool {
    // coordinating-thread only: the compiler is not reentrant across job threads
    var compiles = false
    var inscope access <- make_file_access("")
    set_file_source(access, file, text)
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            compile_file(file, access, unsafe(addr(mg)), cop) $(ok; _program; _issues) {
                compiles = ok
            }
        }
    }
    return compiles
}


def private format_one_strip_file(file : string; verify, verbose : bool; var verified : bool&; var strip_failures, stripped_count : int&) {
    var before : string
    var after : string
    var open = false
    fopen(file, "rb") $(fr) {
        if (fr != null) {
            fmap(fr) $(data) {
                open = true
                before = string(data)
                after = format_source(data, true)
            }
        }
    }
    let diff_result = collect_diff(before, after)
    let changed = diff_result.differ
    if (!open) {
        if (!fexist(file)) {
            log::warn("'{file}' no longer exists, skipped")
            return
        }
        log::info("-- {file}")
        log::error("Unable to open '{file}'")
        strip_failures++
        return
    }
    if (verify) {
        if (changed) {
            log::error("Unformatted file '{file}'")
            log::info(diff_result.log)
            verified = false
        }
        return
    }
    if (!changed) {
        log::info("-- {file}")
        return
    }
    if (!stripped_source_compiles(file, after)) {
        log::info("-- {file}")
        log::error("Comment strip broke compilation of '{file}'; file left unchanged")
        strip_failures++
        return
    }
    var write = false
    fopen(file, "wb") $(fw) {
        if (fw != null) {
            fwrite(fw, after)
            write = true
        }
    }
    if (write) {
        let stripped = after != format_source_string(before, false)
        log::info(stripped ? "++ {file} (comments stripped)" : "++ {file}")
        if (stripped) {
            stripped_count++
        }
        if (verbose) {
            log::info(diff_result.log)
        }
    } else {
        log::info("-- {file}")
        log::error("Unable to open '{file}' for writing")
        strip_failures++
    }
}


def private format_one_file(file : string; verify, verbose, strip_comments : bool; var verified : bool&) {
    // Runs on a job thread: formats `file` in place, or (under --verify) reports
    // it as unformatted and clears `verified`.
    var before : string
    var after : string
    var open = false
    fopen(file, "rb") $(fr) {
        if (fr != null) {
            fmap(fr) $(data) {
                open = true
                before = string(data)
                after = format_source(data, strip_comments)
            }
        }
    }
    let diff_result = collect_diff(before, after)
    let changed = diff_result.differ
    if (!open) {
        if (!fexist(file)) {
            log::warn("'{file}' no longer exists, skipped")
            return
        }
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
        fopen(file, "wb") $(fw) {
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
}


def private drop_format_disabled_files(var files : array<string>) : array<FormatJob> {
    var skipped : array<string>
    var jobs <- partition_by_format_policy(files, skipped)
    for (file in skipped) {
        log::info("-- {file} (format_enabled = false)")
    }
    return <- jobs
}

def private run_format_jobs(var jobs : array<FormatJob>; threadsNum : int; verify, verbose : bool; args : array<string>; var verified : bool&; var strip_failures, stripped_count : int&) {
    var plain_files : array<string>
    var strip_files : array<string>
    for (job in jobs) {
        if (job.strip_comments) {
            strip_files |> push(job.file)
        } else {
            plain_files |> push(job.file)
        }
    }
    delete jobs

    with_job_que() $ {
        while (!empty(plain_files)) {
            let batchNum = min(length(plain_files), threadsNum)
            with_job_status(batchNum) $(status) {
                for (_ in range(batchNum)) {
                    let file = plain_files[length(plain_files) - 1]
                    plain_files |> pop()
                    unsafe {
                        new_job <| @ capture(& verified, := args) {
                            log::init_log(args)
                            format_one_file(file, verify, verbose, false, verified)
                            status |> notify_and_release
                        }
                    }
                }
                status |> join
            }
        }
    }

    for (file in strip_files) {
        format_one_strip_file(file, verify, verbose, verified, strip_failures, stripped_count)
    }
}


[export]
def main() {
    let args <- get_command_line_arguments()
    log::init_log(args)
    let threadsNum = max(args |> get_arg("--t") <| get_total_hw_threads(), 1)
    let verify = args |> find_index("--verify") >= 0
    let verbose = args |> find_index("--verbose") >= 0

    let startTime = ref_time_ticks()

    var seen : table<string; void?>   // shared across --path and --files-from to skip duplicate files
    var inputPaths : array<string>
    collect_input_paths(args, "--path", inputPaths)
    var files : array<string>
    if (!collect_files(inputPaths, seen, files)) {
        log::error("Unable to collect files list\n")
        unsafe(fio::exit(1))
        return
    }

    var listPaths : array<string>
    collect_input_paths(args, "--files-from", listPaths)
    for (lf in listPaths) {
        if (!collect_files_from_list(lf, seen, files)) {
            log::error("Unable to collect files list\n")
            unsafe(fio::exit(1))
            return
        }
    }
    delete seen

    drop_excluded_files(args, files)

    var jobs <- drop_format_disabled_files(files)

    let filesNum = length(jobs)
    var verified = true
    var strip_failures = 0
    var stripped_count = 0
    run_format_jobs(jobs, threadsNum, verify, verbose, args, verified, strip_failures, stripped_count)

    if (stripped_count > 0) {
        log::verdict("comments were deleted from {stripped_count} file(s) — before committing, run the harvester agent over their strip diff (the '(comments stripped)' files above)")
    }

    if (strip_failures > 0) {
        log::verdict("Failed! {strip_failures} file(s) not stripped in {time_dt_hr(get_time_usec(startTime))}")
        unsafe(fio::exit(1))
        return
    }

    if (verify) {
        if (verified) {
            log::verdict("Verified! {filesNum} files in {time_dt_hr(get_time_usec(startTime))}")
            return
        } else {
            log::verdict("Verification failed! {filesNum} files in {time_dt_hr(get_time_usec(startTime))}")
            unsafe(fio::exit(1))
        }
    }

    log::verdict("Complete! {filesNum} files in {time_dt_hr(get_time_usec(startTime))}")
}

// options debugger
// require daslib/debug
