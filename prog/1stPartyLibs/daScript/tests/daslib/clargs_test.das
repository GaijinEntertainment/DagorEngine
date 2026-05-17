options gen2

require dastest/testing_boost
require daslib/clargs

enum Color {
    Red
    Green
    Blue
}

[CommandLineArgs]
struct ConfigWithRequired {
    foo : string

    @clarg_required
    bar_required : string
}

[CommandLineArgs]
struct Config {
    name : string
    count : int
    verbose : bool
    color : Color
    timeout : float
    tags : array<string> = ["default"]
}

[CommandLineArgs]
struct ShortFlagsConfig {
    @clarg_short = "n"
    @clarg_doc = "user name"
    name : string

    @clarg_short = "c"
    @clarg_doc = "iteration count"
    count : int

    @clarg_short = "v"
    @clarg_doc = "verbose output"
    verbose : bool

    @clarg_short = "t"
    @clarg_doc = "tag to apply"
    tags : array<string>

    @clarg_short = "p"
    @clarg_doc = "palette"
    color : Color

    @clarg_short = "r"
    @clarg_doc = "request timeout in seconds"
    timeout : float

    @clarg_short = "h"
    @clarg_doc = "show this help and exit"
    help : bool
}

[CommandLineArgs]
struct ShortFlagsRequiredConfig {
    @clarg_short = "f"
    @clarg_required
    file : string
}

[CommandLineArgs]
struct OptionalConfig {
    @clarg_doc = "verbose flag (None when absent)"
    verbose : Option<bool>

    @clarg_doc = "iteration count (None when absent)"
    count : Option<int>

    @clarg_doc = "user name (None when absent)"
    name : Option<string>
}

[CommandLineArgs]
struct PositionalConfig {
    @clarg_positional
    @clarg_doc = "subcommand name"
    command : string

    @clarg_positional
    @clarg_doc = "subcommand argument"
    command_arg : Option<string>

    @clarg_doc = "project root"
    root : string = "."

    @clarg_doc = "force"
    force : bool

    @clarg_short = "v"
    @clarg_doc = "verbose"
    verbose : bool
}

[CommandLineArgs]
struct PositionalArrayTailConfig {
    @clarg_positional
    @clarg_doc = "first arg"
    head : string

    @clarg_positional
    @clarg_doc = "remaining args"
    rest : array<string>
}

[CommandLineArgs]
struct AutoHelpConfig {
    @clarg_doc = "user name"
    name : string
}

[CommandLineArgs]
struct MutexConfig {
    @clarg_mutex_group = "color"
    color : bool

    @clarg_name = "no-color"
    @clarg_mutex_group = "color"
    no_color : bool

    @clarg_mutex_group = "verbosity"
    quiet : bool

    @clarg_mutex_group = "verbosity"
    loud : bool

    flag_outside_groups : bool
}

[CommandLineArgs]
struct CountConfig {
    @clarg_count
    @clarg_short = "v"
    @clarg_doc = "verbosity"
    verbose : int
}

[test]
def test_string_flag(t : T?) {
    var cfg = Config()
    let a <- ["--name", "Alice"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(cfg.name, "Alice")
}

[test]
def test_string_flag_equals(t : T?) {
    var cfg = Config()
    let a <- ["--name=Bob"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(cfg.name, "Bob")
}

[test]
def test_string_array_flag(t : T?) {
    var cfg = Config()
    let a <- ["--tags=aaa", "--tags", "bbb", "--tags=ccc"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(cfg.tags, ["aaa", "bbb", "ccc"])
}

[test]
def test_int_flag(t : T?) {
    var cfg = Config()
    let a <- ["--count", "42"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(cfg.count, 42)
}

[test]
def test_float_flag(t : T?) {
    var cfg = Config()
    let a <- ["--timeout", "45.1"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(cfg.timeout, 45.1)
}

[test]
def test_bool_flag(t : T?) {
    var cfg = Config()
    let a <- ["--verbose"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> success(cfg.verbose, "verbose is true")
}

[test]
def test_bool_flag_explicit_false(t : T?) {
    var cfg = Config()
    let a <- ["--verbose=false"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> success(!cfg.verbose, "verbose is false")
}

[test]
def test_enum_flag(t : T?) {
    var cfg = Config()
    let a <- ["--color", "Green"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(int(cfg.color), int(Color.Green))
}

[test]
def test_invalid_int(t : T?) {
    var cfg = Config()
    let a <- ["--count", "abc"]
    t |> success(parse_args(cfg, a) != "", "parse fails on bad int")
}

[test]
def test_invalid_float(t : T?) {
    var cfg = Config()
    let a <- ["--timeout", "abc"]
    t |> success(parse_args(cfg, a) != "", "parse fails on bad floats")
}

[test]
def test_invalid_enum(t : T?) {
    var cfg = Config()
    let a <- ["--color", "Purple"]
    t |> success(parse_args(cfg, a) != "", "parse fails on unknown enum value")
}

[test]
def test_defaults_unchanged(t : T?) {
    var cfg = Config()
    let a : array<string>
    t |> success(parse_args(cfg, a) == "", "parse succeeds with no args")
    t |> equal(cfg.name, "")
    t |> equal(cfg.count, 0)
    t |> equal(cfg.tags, ["default"])
    t |> success(!cfg.verbose)
}

[test]
def test_required(t : T?) {
    // Required flag present: parse succeeds.
    var cfg = ConfigWithRequired()
    let err = parse_args(cfg, ["--foo=1", "--bar-required=hello"])
    t |> equal(err, "")
    t |> equal(cfg.foo, "1")
    t |> equal(cfg.bar_required, "hello")
}

[test]
def test_required_failure(t : T?) {
    // Required flag missing: parse returns a missing-flag error.
    var cfg = ConfigWithRequired()
    let err = parse_args(cfg, ["--foo=1"])
    t |> equal(err, "--bar-required: missing required flag")
}

// --- Missing-value detection ----------------------------------------------
// A bare "--flag" (last token, no following value, no "=value" form) is
// malformed input for any value-bearing flag.  parse_args must report it
// instead of silently leaving the field at default.

[test]
def test_missing_value_string(t : T?) {
    var cfg = Config()
    let err = parse_args(cfg, ["--name"])
    t |> equal(err, "--name: missing value")
}

[test]
def test_missing_value_int(t : T?) {
    var cfg = Config()
    let err = parse_args(cfg, ["--count"])
    t |> equal(err, "--count: missing value")
}

[test]
def test_missing_value_float(t : T?) {
    var cfg = Config()
    let err = parse_args(cfg, ["--timeout"])
    t |> equal(err, "--timeout: missing value")
}

[test]
def test_missing_value_enum(t : T?) {
    var cfg = Config()
    let err = parse_args(cfg, ["--color"])
    t |> equal(err, "--color: missing value")
}

[test]
def test_missing_value_array(t : T?) {
    var cfg = Config()
    let err = parse_args(cfg, ["--tags"])
    t |> equal(err, "--tags: missing value")
}

[test]
def test_missing_value_short_flag(t : T?) {
    // Same path via the short-flag form.
    var cfg = ShortFlagsConfig()
    let err = parse_args(cfg, ["-n"])
    t |> equal(err, "--name: missing value")
}

[test]
def test_bare_bool_still_sets_true(t : T?) {
    // Bool is the one type where bare "--flag" IS the canonical "set true"
    // form, not a missing value.  Make sure the new detection didn't break it.
    var cfg = Config()
    let err = parse_args(cfg, ["--verbose"])
    t |> equal(err, "")
    t |> success(cfg.verbose, "verbose set true by bare flag")
}

// --- Duplicate-flag policy: last occurrence wins -------------------------
// Matches GNU getopt, argparse, clap, etc. — lets users override an earlier
// flag (typically from an alias / wrapper) by appending a later one.

[test]
def test_duplicate_string_last_wins(t : T?) {
    var cfg = Config()
    parse_args(cfg, ["--name=Alice", "--name=Bob"])
    t |> equal(cfg.name, "Bob")
}

[test]
def test_duplicate_int_last_wins(t : T?) {
    var cfg = Config()
    parse_args(cfg, ["--count", "1", "--count=42"])
    t |> equal(cfg.count, 42)
}

[test]
def test_duplicate_bool_last_wins(t : T?) {
    var cfg = Config()
    parse_args(cfg, ["--verbose", "--verbose=false"])
    t |> success(!cfg.verbose, "explicit false after bare true overrides")
}

[test]
def test_short_long_alias_last_wins(t : T?) {
    var cfg = ShortFlagsConfig()
    parse_args(cfg, ["-n", "Alice", "--name", "Bob"])
    t |> equal(cfg.name, "Bob")
}

[test]
def test_long_short_alias_last_wins(t : T?) {
    var cfg = ShortFlagsConfig()
    parse_args(cfg, ["--name", "Bob", "-n", "Alice"])
    t |> equal(cfg.name, "Alice")
}

[test]
def test_get_command_info(t : T?) {
    let info <- get_command_info(type<Config>)
    t |> equal(length(info.args), 6)
    t |> equal(info.args[0].flag_name, "--name")
    t |> equal(info.args[1].flag_name, "--count")
    t |> equal(info.args[2].flag_name, "--verbose")
    t |> equal(info.args[3].flag_name, "--color")
    t |> equal(info.args[4].flag_name, "--timeout")
    t |> equal(info.args[5].flag_name, "--tags")
    t |> equal(length(info.args[3].enum_values), 3)
}

// --- Short flags ----------------------------------------------------------

[test]
def test_short_flag_string(t : T?) {
    var cfg = ShortFlagsConfig()
    let a <- ["-n", "Alice"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(cfg.name, "Alice")
}

[test]
def test_short_flag_string_equals(t : T?) {
    var cfg = ShortFlagsConfig()
    let a <- ["-n=Bob"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(cfg.name, "Bob")
}

[test]
def test_short_flag_int(t : T?) {
    var cfg = ShortFlagsConfig()
    let a <- ["-c", "42"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(cfg.count, 42)
}

[test]
def test_short_flag_float(t : T?) {
    var cfg = ShortFlagsConfig()
    let a <- ["-r=2.5"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(cfg.timeout, 2.5)
}

[test]
def test_short_flag_bool(t : T?) {
    var cfg = ShortFlagsConfig()
    let a <- ["-v"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> success(cfg.verbose, "verbose is true")
}

[test]
def test_short_flag_enum(t : T?) {
    var cfg = ShortFlagsConfig()
    let a <- ["-p", "Blue"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(int(cfg.color), int(Color.Blue))
}

[test]
def test_short_flag_array(t : T?) {
    var cfg = ShortFlagsConfig()
    let a <- ["-t", "a", "-t=b", "-t", "c"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(cfg.tags, ["a", "b", "c"])
}

[test]
def test_short_and_long_array_preserve_order(t : T?) {
    // Mixing long and short occurrences must preserve command-line order.
    var cfg = ShortFlagsConfig()
    let a <- ["--tags=a", "-t", "b", "--tags=c", "-t=d"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> equal(cfg.tags, ["a", "b", "c", "d"])
}

[test]
def test_short_flag_required_present(t : T?) {
    var cfg = ShortFlagsRequiredConfig()
    let err = parse_args(cfg, ["-f", "in.txt"])
    t |> equal(err, "")
    t |> equal(cfg.file, "in.txt")
}

[test]
def test_short_flag_required_missing(t : T?) {
    var cfg = ShortFlagsRequiredConfig()
    let err = parse_args(cfg, [])
    t |> equal(err, "--file: missing required flag")
}

[test]
def test_short_flag_metadata(t : T?) {
    let info <- get_command_info(type<ShortFlagsConfig>)
    t |> equal(info.args[0].flag_name, "--name")
    t |> equal(info.args[0].short_flag_name, "-n")
    t |> equal(info.args[2].flag_name, "--verbose")
    t |> equal(info.args[2].short_flag_name, "-v")
    t |> equal(info.args[6].flag_name, "--help")
    t |> equal(info.args[6].short_flag_name, "-h")
}

// --- get_cli_arguments / get_program_args overloads -----------------------

[test]
def test_get_cli_arguments_after_double_dash(t : T?) {
    let result <- get_cli_arguments(["host", "script.das", "--", "--name=Alice", "--verbose"])
    t |> equal(length(result), 2)
    t |> equal(result[0], "--name=Alice")
    t |> equal(result[1], "--verbose")
}

[test]
def test_get_cli_arguments_no_separator(t : T?) {
    let result <- get_cli_arguments(["host", "script.das", "--name=Alice"])
    t |> equal(length(result), 0)
}

[test]
def test_get_cli_arguments_empty_after_separator(t : T?) {
    let result <- get_cli_arguments(["host", "script.das", "--"])
    t |> equal(length(result), 0)
}

[test]
def test_get_program_args_skips_argv0(t : T?) {
    let result <- get_program_args(["fmt.exe", "--write", "file.das"])
    t |> equal(length(result), 2)
    t |> equal(result[0], "--write")
    t |> equal(result[1], "file.das")
}

[test]
def test_get_program_args_only_argv0(t : T?) {
    let result <- get_program_args(["fmt.exe"])
    t |> equal(length(result), 0)
}

// --- print_help / format_help --------------------------------------------

[test]
def test_format_help_smoke(t : T?) {
    let info <- get_command_info(type<ShortFlagsConfig>)
    let help = format_help(info, "demo")
    // Test runs under the interpreter, so format_help emits the daslang-script invocation form.
    t |> success(starts_with(help, "Usage: daslang demo -- [flags]\n"), "starts with usage line")
    t |> success(find(help, "-n, --name=STRING") != -1, "renders short+long flag spec")
    t |> success(find(help, "-v, --verbose") != -1, "bool flag has no =placeholder")
    t |> success(find(help, "-p, --color=ENUM") != -1, "enum flag uses ENUM placeholder")
    t |> success(find(help, "(Red|Green|Blue)") != -1, "enum values rendered inline")
    t |> success(find(help, "(repeated)") != -1, "array flags marked (repeated)")
}

[test]
def test_format_help_required_marker(t : T?) {
    let info <- get_command_info(type<ShortFlagsRequiredConfig>)
    let help = format_help(info, "upload")
    t |> success(find(help, "(required)") != -1, "required flags marked (required)")
}

[test]
def test_format_help_no_short_alignment(t : T?) {
    let info <- get_command_info(type<Config>)
    let help = format_help(info, "demo")
    // No fields have short flags — every flag line should start with "      --"
    // (two spaces of margin + four spaces where "-X, " would go).
    t |> success(find(help, "      --name") != -1, "long-only flags align with short slot blank")
}

// --- Option<T> fields ----------------------------------------------------

[test]
def test_optional_bool_absent_is_none(t : T?) {
    var cfg = OptionalConfig()
    let a : array<string>
    t |> success(parse_args(cfg, a) == "", "parse succeeds with no args")
    t |> success(cfg.verbose |> is_none, "verbose stays None when flag absent")
    t |> success(cfg.count |> is_none, "count stays None when flag absent")
    t |> success(cfg.name |> is_none, "name stays None when flag absent")
}

[test]
def test_optional_bool_set_true(t : T?) {
    var cfg = OptionalConfig()
    let a <- ["--verbose"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> success(cfg.verbose |> is_some, "verbose becomes Some")
    t |> equal(cfg.verbose |> unwrap, true)
}

[test]
def test_optional_bool_set_false_explicit(t : T?) {
    var cfg = OptionalConfig()
    let a <- ["--verbose=false"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> success(cfg.verbose |> is_some, "explicit --verbose=false produces Some(false)")
    t |> equal(cfg.verbose |> unwrap, false)
}

[test]
def test_optional_int_set(t : T?) {
    var cfg = OptionalConfig()
    let a <- ["--count", "7"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> success(cfg.count |> is_some, "count Some")
    t |> equal(cfg.count |> unwrap, 7)
}

[test]
def test_optional_string_set(t : T?) {
    var cfg = OptionalConfig()
    let a <- ["--name=Alice"]
    t |> success(parse_args(cfg, a) == "", "parse succeeds")
    t |> success(cfg.name |> is_some, "name Some")
    t |> equal(cfg.name |> unwrap, "Alice")
}

[test]
def test_optional_int_invalid_value(t : T?) {
    var cfg = OptionalConfig()
    let a <- ["--count=notanumber"]
    let err = parse_args(cfg, a)
    t |> success(err != "", "parse fails on bad int")
    t |> success(find(err, "--count") != -1, "error mentions flag name")
}

// --- Positional arguments -------------------------------------------------
// `@clarg_positional` lets a struct field be filled from non-flag tokens,
// in declaration order.  Required positionals (plain `string`) error if
// missing; optional positionals (`Option<string>`) silently default; an
// `array<string>` positional collects all remaining tokens.

[test]
def test_positional_basic(t : T?) {
    var cfg = PositionalConfig()
    let a <- ["install", "dasImgui"]
    t |> equal(parse_args(cfg, a), "")
    t |> equal(cfg.command, "install")
    t |> success(cfg.command_arg |> is_some, "command_arg present")
    t |> equal(cfg.command_arg |> unwrap, "dasImgui")
}

[test]
def test_positional_only_required(t : T?) {
    var cfg = PositionalConfig()
    let a <- ["list"]
    t |> equal(parse_args(cfg, a), "")
    t |> equal(cfg.command, "list")
    t |> success(cfg.command_arg |> is_none, "command_arg absent")
}

[test]
def test_positional_missing_required(t : T?) {
    var cfg = PositionalConfig()
    let a : array<string>
    let err = parse_args(cfg, a)
    t |> success(err != "", "parse fails when required positional missing")
    t |> success(find(err, "command") != -1, "error mentions positional name")
}

[test]
def test_positional_mixed_with_flags(t : T?) {
    var cfg = PositionalConfig()
    let a <- ["--root", "/tmp", "install", "dasImgui", "--force"]
    t |> equal(parse_args(cfg, a), "")
    t |> equal(cfg.command, "install")
    t |> equal(cfg.command_arg |> unwrap, "dasImgui")
    t |> equal(cfg.root, "/tmp")
    t |> success(cfg.force, "force flag set")
}

[test]
def test_positional_equals_form_flag_no_consumption(t : T?) {
    // `--root=/tmp` MUST NOT consume `install` as the flag value.
    var cfg = PositionalConfig()
    let a <- ["--root=/tmp", "install"]
    t |> equal(parse_args(cfg, a), "")
    t |> equal(cfg.root, "/tmp")
    t |> equal(cfg.command, "install")
    t |> success(cfg.command_arg |> is_none, "no second positional")
}

[test]
def test_positional_short_flag_with_value(t : T?) {
    // Space-separated short flag form takes the next token as value.
    var cfg = PositionalConfig()
    let a <- ["-v", "install", "foo"]
    t |> equal(parse_args(cfg, a), "")
    t |> success(cfg.verbose, "verbose set")
    t |> equal(cfg.command, "install")
    t |> equal(cfg.command_arg |> unwrap, "foo")
}

[test]
def test_positional_unknown_flag_skipped(t : T?) {
    // Unknown `--unknown` is silently dropped, NOT treated as positional.
    var cfg = PositionalConfig()
    let a <- ["--unknown", "install"]
    t |> equal(parse_args(cfg, a), "")
    t |> equal(cfg.command, "install")
}

[test]
def test_positional_array_tail(t : T?) {
    var cfg = PositionalArrayTailConfig()
    let a <- ["a", "b", "c", "d"]
    t |> equal(parse_args(cfg, a), "")
    t |> equal(cfg.head, "a")
    t |> equal(cfg.rest, ["b", "c", "d"])
}

[test]
def test_positional_array_tail_empty(t : T?) {
    var cfg = PositionalArrayTailConfig()
    let a <- ["only"]
    t |> equal(parse_args(cfg, a), "")
    t |> equal(cfg.head, "only")
    t |> equal(length(cfg.rest), 0)
}

[test]
def test_positional_help_renders_usage(t : T?) {
    let info = get_command_info(type<PositionalConfig>)
    let help = format_help(info, "test")
    t |> success(find(help, "<command>") != -1, "required positional in usage")
    t |> success(find(help, "[<command_arg>]") != -1, "optional positional bracketed in usage")
    t |> success(find(help, "Positional arguments:") != -1, "positionals section header")
}

// --- Auto --help / -h -----------------------------------------------------
// `parse_args_with_help` is auto-generated when the user did not declare
// their own `--help` / `-h` flag.  It returns -1 on clean parse, 0 on help,
// 1 on parse error.

[test]
def test_auto_help_long(t : T?) {
    // format_help_with_auto_help injects --help / -h into the help table when
    // the user did not declare their own (AutoHelpConfig has none).
    let info = get_command_info(type<AutoHelpConfig>)
    let help = format_help_with_auto_help(info, "test")
    t |> success(find(help, "--help") != -1, "auto-injected --help row")
    t |> success(find(help, "-h, --help") != -1, "auto-injected -h short alias")
}

[test]
def test_auto_help_user_help_no_inject(t : T?) {
    // ShortFlagsConfig (defined above) declares its OWN help field; auto-help
    // must NOT add a duplicate row.
    let info = get_command_info(type<ShortFlagsConfig>)
    let help = format_help_with_auto_help(info, "test")
    // count occurrences of "--help" — should be exactly 1 (the user's one).
    var n = 0
    var search_from = 0
    while (true) {
        let idx = find(help, "--help", search_from)
        if (idx == -1) {
            break
        }
        n++
        search_from = idx + 1
    }
    t |> equal(n, 1, "exactly one --help row")
}

// --- Mutex groups ---------------------------------------------------------

[test]
def test_mutex_group_one_member(t : T?) {
    var cfg = MutexConfig()
    let a <- ["--color"]
    t |> equal(parse_args(cfg, a), "")
    t |> success(cfg.color, "color set")
    t |> success(!cfg.no_color, "no_color clear")
}

[test]
def test_mutex_group_violation(t : T?) {
    var cfg = MutexConfig()
    let a <- ["--color", "--no-color"]
    let err = parse_args(cfg, a)
    t |> success(err != "", "mutex violation reported")
    t |> success(find(err, "--color") != -1, "error mentions --color")
    t |> success(find(err, "--no-color") != -1, "error mentions --no-color")
    t |> success(find(err, "color") != -1, "error mentions group name")
}

[test]
def test_mutex_group_none_set(t : T?) {
    var cfg = MutexConfig()
    let a : array<string>
    t |> equal(parse_args(cfg, a), "")
}

[test]
def test_mutex_different_groups_coexist(t : T?) {
    var cfg = MutexConfig()
    let a <- ["--color", "--quiet"]
    t |> equal(parse_args(cfg, a), "")
    t |> success(cfg.color, "color set")
    t |> success(cfg.quiet, "quiet set")
}

[test]
def test_mutex_help_decoration(t : T?) {
    let info = get_command_info(type<MutexConfig>)
    let help = format_help(info, "test")
    t |> success(find(help, "(mutex: color)") != -1, "color group annotated")
    t |> success(find(help, "(mutex: verbosity)") != -1, "verbosity group annotated")
}

// --- Count action ---------------------------------------------------------

[test]
def test_count_default(t : T?) {
    var cfg = CountConfig()
    let a : array<string>
    t |> equal(parse_args(cfg, a), "")
    t |> equal(cfg.verbose, 0)
}

[test]
def test_count_long_once(t : T?) {
    var cfg = CountConfig()
    let a <- ["--verbose"]
    t |> equal(parse_args(cfg, a), "")
    t |> equal(cfg.verbose, 1)
}

[test]
def test_count_short_thrice(t : T?) {
    var cfg = CountConfig()
    let a <- ["-v", "-v", "-v"]
    t |> equal(parse_args(cfg, a), "")
    t |> equal(cfg.verbose, 3)
}

[test]
def test_count_mixed_long_short(t : T?) {
    var cfg = CountConfig()
    let a <- ["-v", "--verbose"]
    t |> equal(parse_args(cfg, a), "")
    t |> equal(cfg.verbose, 2)
}

[test]
def test_count_rejects_value_form(t : T?) {
    var cfg = CountConfig()
    let a <- ["--verbose=2"]
    let err = parse_args(cfg, a)
    t |> success(err != "", "value form rejected")
    t |> success(find(err, "--verbose") != -1, "error mentions flag")
}

[test]
def test_count_help_decoration(t : T?) {
    let info = get_command_info(type<CountConfig>)
    let help = format_help(info, "test")
    t |> success(find(help, "(repeats)") != -1, "count flag annotated as repeats")
}
