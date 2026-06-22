options gen2

require daslib/clargs

[CommandLineArgs]
struct BenchctlArgs {
    @clarg_positional
    @clarg_doc = "Subcommand: reset / insert / query / compare / help"
    command : Option<string>

    @clarg_positional
    @clarg_doc = "Files to insert (insert subcommand only)"
    files : array<string>

    @clarg_doc = "SQLite database path"
    db : string = "benchdata.db"

    @clarg_name = "no-color"
    @clarg_doc = "Disable colored output"
    no_color : bool

    @clarg_doc = "Git commit hash for insert (default: git rev-parse HEAD); also filters query"
    commit : string

    @clarg_doc = "Tag (insert: repeatable; query: filter)"
    tag : array<string>

    @clarg_name = "old-commit"
    @clarg_doc = "Baseline commit hash filter (compare)"
    old_commit : string

    @clarg_name = "old-tag"
    @clarg_doc = "Baseline tag filter (compare)"
    old_tag : string

    @clarg_name = "new-commit"
    @clarg_doc = "New commit hash filter (compare)"
    new_commit : string

    @clarg_name = "new-tag"
    @clarg_doc = "New tag filter (compare)"
    new_tag : string

    @clarg_doc = "Regex rename old=>new before pairing benchmark names (compare)"
    s : string

    @clarg_short = "?"
    @clarg_doc = "Show this help and exit"
    help : bool
}

struct public ParsedArgs {
    ok : bool
    err : string
    command : string
    files : array<string>
    db : string
    no_color : bool
    commit : string
    tags : array<string>
    old_commit : string
    old_tag : string
    new_commit : string
    new_tag : string
    s : string
    help : bool
}

def public parse_benchctl_args(argv : array<string>) : ParsedArgs {
    var parse_r <- parse_args(type<BenchctlArgs>, argv)
    if (parse_r |> is_err) return ParsedArgs(ok = false, err = parse_r |> unwrap_err)
    var parsed <- parse_r |> move_unwrap
    return <- ParsedArgs(
        ok = true,
        command = parsed.command ?? "",
        files <- parsed.files,
        db = parsed.db,
        no_color = parsed.no_color,
        commit = parsed.commit,
        tags <- parsed.tag,
        old_commit = parsed.old_commit,
        old_tag = parsed.old_tag,
        new_commit = parsed.new_commit,
        new_tag = parsed.new_tag,
        s = parsed.s,
        help = parsed.help
    )
}

def public print_benchctl_help() {
    print(format_help_with_auto_help(get_command_info(type<BenchctlArgs>), "benchctl"))
}
