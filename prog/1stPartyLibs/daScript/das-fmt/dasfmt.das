options indenting = 4

require fio
require strings
require math

require fs
require log

require daslib/jobque_boost
require daslib/das_source_formatter


def private collect_input_paths(args : array<string>; key : string; var paths : array<string>)
    for i in range(length(args) - 1)
        if args[i] == key
            paths |> push <| args[i + 1]


def private collect_files(input_paths : array<string>; var files : array<string>) : bool
    var cache : table<string; void?>
    for path in input_paths
        if path |> ends_with(".das")
            if !cache |> key_exists(path)
                cache.insert(path, null)
                files |> push <| path
        else
            if !fs::scan_dir(path, cache, files, ".das")
                log::error("Unable to scan given path '{path}'")
                return false
    delete cache
    return true


def private get_arg(args : array<string>; name : string; def_val : int = 0) : int
    let idx = find_index(args, name)
    return idx >= 0 && idx + 1 < length(args) ? int(args[idx + 1]) : def_val


[export]
def main()
    var args <- get_command_line_arguments()
    log::init_log(args)
    let threadsNum = max(args |> get_arg("--t") <| get_total_hw_threads(), 1)
    let verify = args |> find_index("--verify") >= 0

    let startTime = ref_time_ticks()

    var inputPaths : array<string>
    collect_input_paths(args, "--path", inputPaths)
    var files : array<string>
    if !collect_files(inputPaths, files)
        log::error("Unable to collect files list\n")
        unsafe
            fio::exit(1)
        return

    var excludeMasks : array<string>
    collect_input_paths(args, "--exclude-mask", excludeMasks)
    var idx = length(files) - 1
    while idx >= 0
        let path = files[idx]
        for excludePath in excludeMasks
            if path |> find(excludePath) >= 0
                log::info("Excluding file '{path}' mask '{excludePath}'")
                files |> erase(idx)
                break
        idx--

    let filesNum = length(files)
    var verified = true

    with_job_que <|
        while length(files) > 0
            let batchNum = min(length(files), threadsNum)
            with_job_status(batchNum) <| $(status)
                for i in range(batchNum)
                    let file = files[length(files) - 1]
                    files |> pop()
                    unsafe
                        new_job <| @ [[& verified]]
                            var before : string
                            var after : string
                            var open = false
                            fopen(file, "rb") <| $(fr)
                                if fr != null
                                    fmap(fr) <| $(data)
                                        open = true
                                        before = string(data)
                                        after = format_source(data)
                            let changed = before != after
                            if !open
                                log::info("-- {file}")
                                log::error("Unable to open '{file}'")
                            elif verify
                                if changed
                                    log::error("Unformatted file '{file}'")
                                    verified = false
                            elif !changed
                                log::info("-- {file}")
                            else
                                var write = false
                                fopen(file, "wb") <| $(fw)
                                    if fw != null
                                        fwrite(fw, after)
                                        write = true
                                if write
                                    log::info("++ {file}")
                                else
                                    log::info("-- {file}")
                                    log::error("Unable to open '{file}' for writing")
                            status |> notify_and_release()
                status |> join

    if verify
        if verified
            log::info("Verified! {filesNum} files in {time_dt_hr(get_time_usec(startTime))}")
            return
        else
            log::info("Verification failed! {filesNum} files in {time_dt_hr(get_time_usec(startTime))}")
            unsafe
                fio::exit(1)

    log::info("Complete! {filesNum} files in {time_dt_hr(get_time_usec(startTime))}")

// options debugger
// require daslib/debug
