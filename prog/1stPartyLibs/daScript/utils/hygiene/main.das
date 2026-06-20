options gen2

require strings
require daslib/clargs
require daslib/strings_boost

require process


[CommandLineArgs]
struct Config {
    @clarg_doc = "report would-be changes; exit 1 if any; don't write"
    check : bool

    @clarg_short = "?"
    @clarg_name = "show-help"
    @clarg_doc = "Show this help and exit"
    help : bool
}


def collect_files(args : array<string>) : array<string> {
    // Files are positional: anything that doesn't start with `-`. The known
    // flags (--check, --help, -?) all start with `-`, and none of them take
    // a separate-token value, so this filter is unambiguous.
    var files : array<string>
    for (a in args) {
        if (starts_with(a, "-")) continue
        files |> push(a)
    }
    return <- files
}


[export]
def main() : int {
    var args_r <- parse_args(type<Config>)
    if (args_r |> is_err) {
        to_log(LOG_ERROR, "error: {args_r |> unwrap_err}\n\n")
        print_help(get_command_info(type<Config>), "hygiene")
        return 1
    }
    let cfg <- args_r |> move_unwrap
    if (cfg.help) {
        print_help(get_command_info(type<Config>), "hygiene")
        return 0
    }
    let files <- collect_files(get_user_args())
    if (empty(files)) {
        to_log(LOG_ERROR, "error: no files specified\n\n")
        print_help(get_command_info(type<Config>), "hygiene")
        return 1
    }
    var any_changed = false
    var any_missing = false
    for (f in files) {
        let r = process_path(f, cfg.check)
        if (r.kind == ProcessResultKind.missing_file) {
            to_log(LOG_ERROR, "error: file not found: {f}\n")
            any_missing = true
            continue
        }
        if (r.kind == ProcessResultKind.not_a_file) {
            to_log(LOG_ERROR, "error: not a regular file: {f}\n")
            any_missing = true
            continue
        }
        if (r.kind == ProcessResultKind.write_failed) {
            to_log(LOG_ERROR, "error: failed to write {f}\n")
            return 2
        }
        if (r.kind == ProcessResultKind.no_change) continue
        any_changed = true
        if (r.kind == ProcessResultKind.would_change) {
            to_log(LOG_INFO, "would trim: {f}\n")
        } elif (r.kind == ProcessResultKind.changed) {
            to_log(LOG_INFO, "trimmed {f} ({r.lines_removed} lines removed)\n")
        }
    }
    if (any_missing) return 2
    if (cfg.check && any_changed) return 1
    return 0
}
