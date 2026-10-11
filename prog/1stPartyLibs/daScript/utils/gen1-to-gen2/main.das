options gen2
options indenting = 4
options rtti = true
options strict_smart_pointers = false

require daslib/clargs
require daslib/ast
require daslib/rtti
require daslib/fio
require convert

//! v1 -> gen2 syntax converter.
//!
//! Reads the file once, parses it with infer off (the tree still mirrors the source), collects
//! every rewrite as an edit in original byte offsets, and applies them in a single forward walk.
//! Nothing is re-parsed and no offset is ever adjusted, so a file is converted in one pass.
//!
//! A comment is the one thing no tool can regenerate, so the output is rejected outright if its
//! comment sequence differs from the input's.

[CommandLineArgs]
struct Config {
    @clarg_short = "p"
    @clarg_doc = "File(s) to convert. Repeatable."
    path : array<string>

    @clarg_short = "i"
    @clarg_doc = "Rewrite the file in place (default: print to stdout)"
    inplace : bool

    @clarg_doc = "Keep the statement-terminating ';'"
    semicolon : bool

    @clarg_name = "gen15"
    @clarg_doc = "Convert to gen1.5 only: make-syntax, no braces or parens"
    gen15 : bool

    @clarg_doc = "Report what a run would change, write nothing"
    verify : bool

    @clarg_doc = ".das_project whose mount points the converted file's `require`s need"
    project : string

    @clarg_name = "verify-compile"
    @clarg_doc = "Compile the converted text as gen2 and fail the file if it does not"
    verify_compile : bool

    @clarg_short = "?"
    @clarg_name = "show-help"
    @clarg_doc = "Show this help and exit"
    help : bool
}


[export]
def main : int {
    var args_r <- parse_args(type<Config>)
    if (args_r |> is_err) {
        to_log(LOG_ERROR, "error: {args_r |> unwrap_err}\n\n")
        print_help(get_command_info(type<Config>), "gen1-to-gen2")
        return 1
    }
    var cfg <- args_r |> move_unwrap
    if (cfg.help || empty(cfg.path)) {
        print_help(get_command_info(type<Config>), "gen1-to-gen2")
        return cfg.help ? 0 : 1
    }
    if (length(cfg.path) > 1 && !cfg.inplace && !cfg.verify) {
        to_log(LOG_ERROR, "error: more than one file needs --inplace or --verify\n")
        return 1
    }

    let opt = ConvertOptions(v2 = !cfg.gen15, semicolon = cfg.semicolon,
        verify_compile = cfg.verify_compile, project = cfg.project)
    var rc = 0
    var n_changed = 0
    for (file in cfg.path) {
        let res <- convert_file(file, opt)
        for (r in res.refused) {
            to_log(LOG_ERROR, "{file}: refused {r}\n")
        }
        if (!empty(res.failed) || !empty(res.refused)) {
            if (!empty(res.failed)) {
                to_log(LOG_ERROR, "{file}: {res.failed}\n")
            }
            rc = 1
            continue
        }
        if (!empty(res.skipped)) {
            to_log(LOG_INFO, "{file}: skipped ({res.skipped})\n")
            if (!cfg.verify && !cfg.inplace) {
                print("{res.output}")
            }
            continue
        }
        if (res.changed) {
            n_changed++
        }
        if (cfg.verify) {
            to_log(LOG_INFO, "{file}: {res.changed ? "would change" : "unchanged"}\n")
        } elif (cfg.inplace) {
            if (res.changed) {
                fwrite(file, res.output)
                to_log(LOG_INFO, "{file}: converted\n")
            }
        } else {
            print("{res.output}")
        }
    }
    if (cfg.verify) {
        to_log(LOG_INFO, "{n_changed} of {length(cfg.path)} file(s) would change\n")
    }
    return rc
}
