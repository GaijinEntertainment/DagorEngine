options gen2

module clargs shared private

require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost
require strings public

require daslib/option public
require daslib/result public
require daslib/enum_trait public

struct public CommandArgumentInfo {
    field_name : string
    flag_name : string         // long form, e.g. "--verbose"; for positionals this is "<field_name>"
    short_flag_name : string   // e.g. "-v"; empty if no short flag declared
    doc_string : string
    value_type : Type
    is_array : bool
    is_required : bool
    is_positional : bool       // declared with @clarg_positional; consumed in declaration order
    is_optional_wrap : bool    // declared as Option<T> (positional or flag); affects required-ness
    is_count : bool            // declared with @clarg_count; sums every flag occurrence
    mutex_group : string       // declared with @clarg_mutex_group="name"; "" if not in any group
    enum_values : array<string>
}

struct public CommandInfo {
    args : array<CommandArgumentInfo>
    has_user_help : bool       // true if the user already declared a --help/-h flag (suppresses auto-injection)
}

// Returns the slice of `all_args` after a "--" separator (or empty if no "--").
// Use this for script-style invocation (`daslang script.das -- script_args...`).
def public get_cli_arguments(all_args : array<string>) : array<string> {
    var arg_index = find_index(all_args, "--")
    var result : array<string>
    if (arg_index == -1) {
        return <- result
    }
    arg_index++ // Skip the "--" itself
    while (arg_index < length(all_args)) {
        result |> push(all_args[arg_index])
        arg_index++
    }
    return <- result
}

def public get_cli_arguments() : array<string> {
    return <- get_cli_arguments(get_command_line_arguments())
}

// Returns `all_args[1..]` (skips the program name).
// Use this for standalone-tool invocation where there is no "--" separator
// and argv[1..] are the actual flags.
def public get_program_args(all_args : array<string>) : array<string> {
    var result : array<string>
    var i = 1
    while (i < length(all_args)) {
        result |> push(all_args[i])
        i++
    }
    return <- result
}

def public get_program_args() : array<string> {
    return <- get_program_args(get_command_line_arguments())
}

// Mode-aware argv source.  Returns argv[1..] when running as a standalone exe
// (built via `daslang -exe`), and the post-`--` slice when running under the
// interpreter or in-process JIT.  This is what the macro-generated default
// `parse_args()` uses, so utilities work the same whether interpreted or
// shipped as a binary.
def public get_user_args() : array<string> {
    let argv <- get_command_line_arguments()
    if (is_standalone_exe()) {
        return <- get_program_args(argv)
    } else {
        return <- get_cli_arguments(argv)
    }
}

def clargs_flag_name(field_name : string) : string {
    return "--" + replace_multiple(field_name, [(text="_", replacement="-")])
}

// Duplicate-flag policy: last occurrence wins.  Matches GNU getopt, Python
// argparse, Go flag, Rust clap, and most other CLI parsers — lets users
// override an earlier flag (typically from an alias or wrapper script) by
// appending a later one.

def public find_bool_flag_raw_value(args : array<string>; flag_name : string) : Option<string> {
    let prefix = "{flag_name}="
    var result = none(type<string>)
    for (i in range(length(args))) {
        if (args[i] == flag_name) {
            result = some("true")
        } elif (starts_with(args[i], prefix)) {
            result = some(slice(args[i], length(prefix)))
        }
    }
    return result
}

def public find_flag_raw_value(args : array<string>; flag_name : string) : Option<string> {
    let prefix = "{flag_name}="
    var result = none(type<string>)
    for (i in range(length(args))) {
        if (args[i] == flag_name) {
            if (i + 1 < length(args)) {
                result = some(args[i + 1])
            }
        } elif (starts_with(args[i], prefix)) {
            result = some(slice(args[i], length(prefix)))
        }
    }
    return result
}

// Returns true if any form of flag_name (--flag, --flag=value, --flag value)
// is present in args.  Used for required-flag validation.
def public flag_present(args : array<string>; flag_name : string) : bool {
    let prefix = "{flag_name}="
    for (a in args) {
        if (a == flag_name) {
            return true
        }
        if (starts_with(a, prefix)) {
            return true
        }
    }
    return false
}

def public find_array_flag_raw_values(args : array<string>; flag_name : string) : array<string> {
    var result : array<string>
    let prefix = "{flag_name}="
    var i = 0
    while (i < length(args)) {
        let arg = args[i]
        i++
        if (arg == flag_name) {
            if (i < length(args)) {
                result |> push(args[i])
                i++
            }
            continue
        }
        if (starts_with(arg, prefix)) {
            result |> push(slice(arg, length(prefix)))
        }
    }
    return <- result
}

// Short-flag-aware overloads: single pass over args treating long and short
// as aliases.  Last-occurrence wins (same policy as the single-flag overloads
// above), so e.g. `-n Alice --name Bob` resolves to "Bob".

def public find_flag_raw_value(args : array<string>; arg_info : CommandArgumentInfo) : Option<string> {
    if (arg_info.is_positional) {
        return none(type<string>)
    }
    let has_short = !empty(arg_info.short_flag_name)
    let long_prefix = "{arg_info.flag_name}="
    let short_prefix = has_short ? "{arg_info.short_flag_name}=" : ""
    var result = none(type<string>)
    for (i in range(length(args))) {
        let arg = args[i]
        if (arg == arg_info.flag_name || (has_short && arg == arg_info.short_flag_name)) {
            if (i + 1 < length(args)) {
                result = some(args[i + 1])
            }
        } elif (starts_with(arg, long_prefix)) {
            result = some(slice(arg, length(long_prefix)))
        } elif (has_short && starts_with(arg, short_prefix)) {
            result = some(slice(arg, length(short_prefix)))
        }
    }
    return result
}

def public find_bool_flag_raw_value(args : array<string>; arg_info : CommandArgumentInfo) : Option<string> {
    if (arg_info.is_positional) {
        return none(type<string>)
    }
    let has_short = !empty(arg_info.short_flag_name)
    let long_prefix = "{arg_info.flag_name}="
    let short_prefix = has_short ? "{arg_info.short_flag_name}=" : ""
    var result = none(type<string>)
    for (arg in args) {
        if (arg == arg_info.flag_name || (has_short && arg == arg_info.short_flag_name)) {
            result = some("true")
        } elif (starts_with(arg, long_prefix)) {
            result = some(slice(arg, length(long_prefix)))
        } elif (has_short && starts_with(arg, short_prefix)) {
            result = some(slice(arg, length(short_prefix)))
        }
    }
    return result
}

// Single pass over args so that mixed long/short occurrences are returned
// in command-line order as they appear in args
// (e.g. --tags=a -t b --tags=c -> ["a", "b", "c"]).
def public find_array_flag_raw_values(args : array<string>; arg_info : CommandArgumentInfo) : array<string> {
    var result : array<string>
    if (arg_info.is_positional) {
        return <- result
    }
    let long_prefix = "{arg_info.flag_name}="
    let has_short = !empty(arg_info.short_flag_name)
    let short_prefix = has_short ? "{arg_info.short_flag_name}=" : ""
    var i = 0
    while (i < length(args)) {
        let arg = args[i]
        i++
        if (arg == arg_info.flag_name || (has_short && arg == arg_info.short_flag_name)) {
            if (i < length(args)) {
                result |> push(args[i])
                i++
            }
            continue
        }
        if (starts_with(arg, long_prefix)) {
            result |> push(slice(arg, length(long_prefix)))
            continue
        }
        if (has_short && starts_with(arg, short_prefix)) {
            result |> push(slice(arg, length(short_prefix)))
        }
    }
    return <- result
}

def public flag_present(args : array<string>; arg_info : CommandArgumentInfo) : bool {
    if (flag_present(args, arg_info.flag_name)) {
        return true
    }
    if (!empty(arg_info.short_flag_name) && flag_present(args, arg_info.short_flag_name)) {
        return true
    }
    return false
}

// For value-bearing flags (everything except bool), a bare "--flag" with no
// following value token and no "=value" form is malformed input.  Each
// parse_argument overload below returns Result<Option<T>, string> distinguishing
// three states:
//   - ok(some(v))   — found and parsed a value
//   - ok(none)      — flag absent; caller leaves the field at its default
//   - err("...")    — parse error (e.g. "missing value", "invalid int value '...')

[unused_argument(t)]
def public parse_argument(args : array<string>; arg_info : CommandArgumentInfo; t : type<string>) : Result<Option<string>; string> {
    let opt = find_flag_raw_value(args, arg_info)
    if (is_none(opt) && flag_present(args, arg_info)) {
        return err("missing value", type<Option<string>>)
    }
    return ok(opt, type<string>)
}

[unused_argument(t)]
def public parse_argument(args : array<string>; arg_info : CommandArgumentInfo; t : type<int>) : Result<Option<int>; string> {
    let opt = find_flag_raw_value(args, arg_info)
    if (is_none(opt)) {
        if (flag_present(args, arg_info)) {
            return err("missing value", type<Option<int>>)
        }
        return ok(none(type<int>), type<string>)
    }
    let raw_value = unwrap(opt)
    var result : ConversionResult
    var offset = 0
    let parsed = int(raw_value, result, offset)
    // Reject both outright parse failures and trailing garbage ("42abc").
    if (result != ConversionResult.ok || offset != length(raw_value)) {
        return err("invalid int value '{raw_value}'", type<Option<int>>)
    }
    return ok(some(parsed), type<string>)
}


[unused_argument(t)]
def public parse_argument(args : array<string>; arg_info : CommandArgumentInfo; t : type<float>) : Result<Option<float>; string> {
    let opt = find_flag_raw_value(args, arg_info)
    if (is_none(opt)) {
        if (flag_present(args, arg_info)) {
            return err("missing value", type<Option<float>>)
        }
        return ok(none(type<float>), type<string>)
    }
    let raw_value = unwrap(opt)
    var result : ConversionResult
    var offset = 0
    let parsed = float(raw_value, result, offset)
    if (result != ConversionResult.ok || offset != length(raw_value)) {
        return err("invalid float value '{raw_value}'", type<Option<float>>)
    }
    return ok(some(parsed), type<string>)
}

[unused_argument(t)]
def public parse_argument(args : array<string>; arg_info : CommandArgumentInfo; t : type<array<string>>) : Result<Option<array<string>>; string> {
    var raw_values = find_array_flag_raw_values(args, arg_info)
    if (!empty(raw_values)) {
        var sm <- move_some(raw_values)
        return move_ok(sm, type<string>)
    }
    if (flag_present(args, arg_info)) {
        return err("missing value", type<Option<array<string>>>)
    }
    return ok(none(type<array<string>>), type<string>)
}

[unused_argument(t)]
def public parse_argument(args : array<string>; arg_info : CommandArgumentInfo; t : type<bool>) : Result<Option<bool>; string> {
    let opt = find_bool_flag_raw_value(args, arg_info)
    if (is_none(opt)) {
        return ok(none(type<bool>), type<string>)
    }
    let raw_value = unwrap(opt)
    if (raw_value == "true") {
        return ok(some(true), type<string>)
    } elif (raw_value == "false") {
        return ok(some(false), type<string>)
    } else {
        return err("invalid bool value: '{raw_value}'", type<Option<bool>>)
    }
}

// Positional-argument collection.
//
// Walks `args` and returns the tokens that should be treated as positionals —
// i.e. tokens that don't match any declared flag (long or short, with or
// without `=value`).  When a known value-bearing flag appears in its
// space-separated form (e.g. "--root /tmp"), the next token is consumed as
// the value (NOT a positional).  Tokens that look flag-shaped but don't match
// any declared flag (`--unknown`, `-x`) are skipped one-by-one so they don't
// land in the positional list either.

def public collect_positional_args(args : array<string>; info : CommandInfo) : array<string> {
    var result : array<string>
    var i = 0
    while (i < length(args)) {
        let arg = args[i]
        var matched_flag = false
        var consumes_value = false
        for (a in info.args) {
            if (a.is_positional) {
                continue
            }
            let has_short = !empty(a.short_flag_name)
            let exact_long = (arg == a.flag_name)
            let exact_short = has_short && (arg == a.short_flag_name)
            let prefix_long = starts_with(arg, "{a.flag_name}=")
            let prefix_short = has_short && starts_with(arg, "{a.short_flag_name}=")
            if (exact_long || exact_short || prefix_long || prefix_short) {
                matched_flag = true
                let is_bool_simple = (a.value_type == Type.tBool && !a.is_array)
                // Bool flags and count flags never consume the next token as a value.
                if (!is_bool_simple && !a.is_count && (exact_long || exact_short)) {
                    consumes_value = true
                }
                break
            }
        }
        if (matched_flag) {
            i++
            if (consumes_value && i < length(args)) {
                i++
            }
            continue
        }
        // Unknown flag-shaped tokens — skip without consuming next.  Avoids
        // them landing in the positional list and matches today's silent-
        // ignore behavior for unknown flags.
        if (length(arg) >= 2 && first_character(arg) == int(0x2D) /* '-' */) {
            i++
            continue
        }
        result |> push(arg)
        i++
    }
    return <- result
}

// Counts every occurrence of `arg_info.flag_name` and `arg_info.short_flag_name`
// in `args`.  Backs `@clarg_count` fields.  v1 does not implement short-flag
// clustering ("-vvv"); only `-v -v -v` and `--verbose --verbose` style sums.
def public count_flag(args : array<string>; arg_info : CommandArgumentInfo) : int {
    if (arg_info.is_positional) {
        return 0
    }
    let has_short = !empty(arg_info.short_flag_name)
    var n = 0
    for (a in args) {
        if (a == arg_info.flag_name) {
            n++
        } elif (has_short && a == arg_info.short_flag_name) {
            n++
        }
    }
    return n
}

// Walks `args` to detect `--flag=value` against a count flag, which is
// rejected (count flags don't accept values).  Returns "" if clean.
def public count_flag_value_violation(args : array<string>; arg_info : CommandArgumentInfo) : string {
    if (arg_info.is_positional || !arg_info.is_count) {
        return ""
    }
    let has_short = !empty(arg_info.short_flag_name)
    let long_prefix = "{arg_info.flag_name}="
    let short_prefix = has_short ? "{arg_info.short_flag_name}=" : ""
    for (a in args) {
        if (starts_with(a, long_prefix)) {
            return "{arg_info.flag_name}: count flag does not accept a value"
        }
        if (has_short && starts_with(a, short_prefix)) {
            return "{arg_info.flag_name}: count flag does not accept a value"
        }
    }
    return ""
}

// Returns "" if every mutex group has at most one member present, or an
// error string identifying the conflicting flags otherwise.
def public find_mutex_violation(args : array<string>; info : CommandInfo) : string {
    var groups : table<string; array<string>>
    for (a in info.args) {
        if (empty(a.mutex_group)) {
            continue
        }
        if (!flag_present(args, a)) {
            continue
        }
        if (!key_exists(groups, a.mutex_group)) {
            groups[a.mutex_group] <- [a.flag_name]
        } else {
            groups[a.mutex_group] |> push(a.flag_name)
        }
    }
    for (gname, members in keys(groups), values(groups)) {
        if (length(members) > 1) {
            return build_string() $(var w) {
                for (i, m in range(length(members)), members) {
                    if (i > 0) {
                        write(w, ", ")
                    }
                    write(w, m)
                }
                write(w, ": mutually exclusive (group '")
                write(w, gname)
                write(w, "')")
            }
        }
    }
    return ""
}

// Synthetic --help/-h argument used by `parse_args_with_help` and friends.
// Not pushed into the user's CommandInfo unless the user explicitly opts in.
def public make_auto_help_arg() : CommandArgumentInfo {
    return CommandArgumentInfo(
        field_name = "_auto_help",
        flag_name = "--help",
        short_flag_name = "-h",
        doc_string = "show this help and exit",
        value_type = Type.tBool
    )
}

// Convenience: render help with the auto-injected --help row appended unless
// the user already declared their own.  Used by `parse_args_with_help`.
def public format_help_with_auto_help(info : CommandInfo; prog_name : string) : string {
    if (info.has_user_help) {
        return format_help(info, prog_name)
    }
    var augmented : CommandInfo
    augmented.has_user_help = info.has_user_help
    augmented.args := info.args
    augmented.args |> push_clone(make_auto_help_arg())
    return format_help(augmented, prog_name)
}

// Help rendering.
//
// `format_help` builds the help text as a string (testable, redirectable).
// `print_help` is a thin wrapper that prints it to stdout.
//
// Layout (standalone exe — `is_standalone_exe() == true`):
//   Usage: <prog_name> [flags]
//
// Layout (interpreted script):
//   Usage: daslang <prog_name> -- [flags]
//
//   Flags:
//     -X, --long=PLACEHOLDER    doc_string (required) (repeated)
//         --long=PLACEHOLDER    doc_string
//
// Bool flags omit the =PLACEHOLDER. Enum flag docs include `(V1|V2|...)`.

def value_placeholder(arg : CommandArgumentInfo) : string {
    if (arg.is_positional || arg.is_count) {
        return ""
    }
    if (arg.value_type == Type.tBool && !arg.is_array) {
        return ""
    }
    if (arg.value_type == Type.tInt) {
        return "=INT"
    }
    if (arg.value_type == Type.tFloat) {
        return "=FLOAT"
    }
    if (arg.value_type == Type.tEnumeration) {
        return "=ENUM"
    }
    return "=STRING"
}

def render_flag_spec(arg : CommandArgumentInfo) : string {
    if (arg.is_positional) {
        // arg.flag_name is already wrapped as "<name>" by collect_command_info.
        return arg.flag_name
    }
    var spec = ""
    if (!empty(arg.short_flag_name)) {
        spec = "{arg.short_flag_name}, "
    } else {
        spec = "    "
    }
    spec += arg.flag_name
    spec += value_placeholder(arg)
    return spec
}

def render_doc_column(arg : CommandArgumentInfo) : string {
    return build_string() $(var w) {
        var first = true
        if (!empty(arg.doc_string)) {
            write(w, arg.doc_string)
            first = false
        }
        if (!empty(arg.enum_values)) {
            if (!first) {
                write(w, " ")
            }
            first = false
            write(w, "(")
            for (i, v in range(length(arg.enum_values)), arg.enum_values) {
                if (i > 0) {
                    write(w, "|")
                }
                write(w, v)
            }
            write(w, ")")
        }
        if (arg.is_required) {
            if (!first) {
                write(w, " ")
            }
            first = false
            write(w, "(required)")
        }
        if (arg.is_array) {
            if (!first) {
                write(w, " ")
            }
            first = false
            write(w, "(repeated)")
        }
        if (arg.is_count) {
            if (!first) {
                write(w, " ")
            }
            first = false
            write(w, "(repeats)")
        }
        if (!empty(arg.mutex_group)) {
            if (!first) {
                write(w, " ")
            }
            write(w, "(mutex: {arg.mutex_group})")
        }
    }
}

// Renders a positional arg's slot in the usage line: `<name>` for required,
// `[<name>]` for optional, `[<name>...]` for the array tail.
def render_positional_usage_token(arg : CommandArgumentInfo) : string {
    if (arg.is_array) {
        return "[{arg.flag_name}...]"
    }
    if (arg.is_required) {
        return "{arg.flag_name}"
    }
    return "[{arg.flag_name}]"
}

def public format_help(info : CommandInfo; prog_name : string) : string {
    var positionals : array<CommandArgumentInfo>
    var flags : array<CommandArgumentInfo>
    for (arg in info.args) {
        if (arg.is_positional) {
            positionals |> push_clone(arg)
        } else {
            flags |> push_clone(arg)
        }
    }
    var specs : array<string>
    specs |> reserve(length(info.args))
    var max_spec_len = 0
    for (arg in positionals) {
        let s = render_flag_spec(arg)
        if (length(s) > max_spec_len) {
            max_spec_len = length(s)
        }
        specs |> push(s)
    }
    for (arg in flags) {
        let s = render_flag_spec(arg)
        if (length(s) > max_spec_len) {
            max_spec_len = length(s)
        }
        specs |> push(s)
    }
    let gutter = 4
    return build_string() $(var w) {
        if (is_standalone_exe()) {
            write(w, "Usage: {prog_name}")
        } else {
            write(w, "Usage: daslang {prog_name} --")
        }
        for (arg in positionals) {
            write(w, " ")
            write(w, render_positional_usage_token(arg))
        }
        if (!empty(flags)) {
            write(w, " [flags]")
        }
        write(w, "\n")

        if (!empty(positionals)) {
            write(w, "\nPositional arguments:\n")
            for (i, arg in range(length(positionals)), positionals) {
                let s = specs[i]
                let pad = max_spec_len - length(s) + gutter
                write(w, "  ")
                write(w, s)
                write(w, repeat(" ", pad))
                write(w, render_doc_column(arg))
                write(w, "\n")
            }
        }
        if (!empty(flags)) {
            write(w, "\nFlags:\n")
            let pos_count = length(positionals)
            for (i, arg in range(length(flags)), flags) {
                let s = specs[pos_count + i]
                let pad = max_spec_len - length(s) + gutter
                write(w, "  ")
                write(w, s)
                write(w, repeat(" ", pad))
                write(w, render_doc_column(arg))
                write(w, "\n")
            }
        }
    }
}

def public print_help(info : CommandInfo; prog_name : string) : void {
    print(format_help(info, prog_name))
}

def option_inner_type(t : TypeDeclPtr) : TypeDeclPtr {
    if (t.baseType == Type.typeMacro) {
        if (length(t.dimExpr) < 2 || t.dimExpr[0] == null || !(t.dimExpr[0] is ExprConstString)) {
            return null
        }
        if ((t.dimExpr[0] as ExprConstString).value != "Option") {
            return null
        }
        if (t.dimExpr[1] == null || !(t.dimExpr[1] is ExprTypeDecl)) {
            return null
        }
        return clone_type((t.dimExpr[1] as ExprTypeDecl).typeexpr)
    }
    if (t.baseType != Type.tStructure || t.structType == null) {
        return null
    }
    var has_flag = false
    var value_t : TypeDeclPtr = null
    for (f in t.structType.fields) {
        if ("{f.name}" == "_has_value" && f._type.baseType == Type.tBool) {
            has_flag = true
        } elif ("{f.name}" == "_value") {
            value_t = clone_type(f._type)
        }
    }
    if (!has_flag || value_t == null) {
        return null
    }
    return value_t
}


[structure_macro(name=CommandLineArgs)]
class ArgsMacro : AstStructureAnnotation {
    def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        let command_info = collect_command_info(st, errors)
        if (!empty(errors)) {
            return false
        }

        var command_info_fn = make_command_info_fn(st, command_info)
        compiling_module() |> add_function(command_info_fn)

        var parse_args_fn = make_parse_fn(st, command_info)
        compiling_module() |> add_function(parse_args_fn)

        var parse_args_default_fn = make_parse_default_fn(st)
        compiling_module() |> add_function(parse_args_default_fn)

        // Auto --help/-h is only injected when the user did not declare their
        // own help flag.  If they did, they're responsible for handling it.
        if (!command_info.has_user_help) {
            var parse_args_with_help_fn = make_parse_args_with_help_fn(st)
            compiling_module() |> add_function(parse_args_with_help_fn)
        }

        return true
    }

    def find_bool_annotation(args : AnnotationArgumentList; argn : string, default_value : bool) : bool {
        let v = find_arg(args, argn)
        if (v is tBool) {
            return v as tBool
        }
        return default_value
    }

    def find_string_annotation(args : AnnotationArgumentList; argn : string, default_value : string) : string {
        let v = find_arg(args, argn)
        if (v is tString) {
            return v as tString
        }
        return default_value
    }

    // collect_command_info turns the structure info into command info.
    // It serves two functions: it makes the code generation easier
    // by pre-processing the raw type data, plus it makes it
    // possible to have an easy-to-use runtime value with all command info.
    // This info can be used to print command's help, for example.
    //
    // On invalid input (bad @clarg_short value, short-flag collision) populates
    // `errors` and returns the partial result; the caller checks `errors` and
    // aborts macro application.
    def collect_command_info(st : StructurePtr; var errors : das_string&) : CommandInfo {
        var result : CommandInfo
        result.args |> reserve(length(st.fields))
        var seen_short : table<string; string>
        var has_array_tail = false       // an array<string> positional terminates the positional list
        var saw_optional_positional = false
        for (field in st.fields) {
            if (find_bool_annotation(field.annotation, "clarg_skip", false)) {
                continue
            }
            let is_positional = find_bool_annotation(field.annotation, "clarg_positional", false)
            let is_count = find_bool_annotation(field.annotation, "clarg_count", false)
            let raw_name = find_string_annotation(field.annotation, "clarg_name", "{field.name}")
            var arg = CommandArgumentInfo(
                field_name = "{field.name}",
                flag_name = is_positional ? "<{raw_name}>" : clargs_flag_name(raw_name),
                is_positional = is_positional,
                is_count = is_count,
                mutex_group = find_string_annotation(field.annotation, "clarg_mutex_group", "")
            )

            let short_char = find_string_annotation(field.annotation, "clarg_short", "")
            if (!empty(short_char)) {
                if (is_positional) {
                    errors := "field '{arg.field_name}': @clarg_short cannot be combined with @clarg_positional"
                    return <- result
                }
                if (length(short_char) != 1) {
                    errors := "field '{arg.field_name}': @clarg_short must be a single character (got '{short_char}')"
                    return <- result
                }
                // Reject characters that produce ambiguous or unparseable
                // short flags: '-' yields '--' (collides with the script-args
                // separator), '=' yields '-=' (collides with --flag=value
                // form), whitespace can't be typed on the command line.
                if (short_char == "-" || short_char == "=" || short_char == " " || short_char == "\t" || short_char == "\n" || short_char == "\r") {
                    errors := "field '{arg.field_name}': @clarg_short cannot be '-', '=', or whitespace (got '{short_char}')"
                    return <- result
                }
                if (key_exists(seen_short, short_char)) {
                    errors := "field '{arg.field_name}': @clarg_short '-{short_char}' already used by field '{seen_short[short_char]}'"
                    return <- result
                }
                seen_short[short_char] = arg.field_name
                arg.short_flag_name = "-{short_char}"
            }
            arg.is_required = find_bool_annotation(field.annotation, "clarg_required", false)
            arg.value_type = field._type.baseType
            if (field._type.baseType == Type.tArray) {
                arg.value_type = field._type.firstType.baseType
                arg.is_array = true
            } else {
                let inner = option_inner_type(field._type)
                if (inner != null) {
                    arg.value_type = inner.baseType
                    arg.is_optional_wrap = true
                }
            }
            if (arg.value_type == Type.tEnumeration) {
                if (arg.is_array) {
                    for (entry in field._type.firstType.enumType.list) {
                        arg.enum_values |> push("{entry.name}")
                    }
                } else {
                    for (entry in field._type.enumType.list) {
                        arg.enum_values |> push("{entry.name}")
                    }
                }
            }

            if (is_positional) {
                // Allowed types: string, Option<string>, array<string>.
                if (arg.value_type != Type.tString) {
                    errors := "field '{arg.field_name}': @clarg_positional only supports string, Option<string>, and array<string>"
                    return <- result
                }
                if (has_array_tail) {
                    errors := "field '{arg.field_name}': another @clarg_positional cannot follow an array<string> positional"
                    return <- result
                }
                let is_required_positional = !arg.is_array && !arg.is_optional_wrap
                if (is_required_positional && saw_optional_positional) {
                    errors := "field '{arg.field_name}': required @clarg_positional cannot follow an optional one"
                    return <- result
                }
                if (arg.is_array) {
                    has_array_tail = true
                } elif (arg.is_optional_wrap) {
                    saw_optional_positional = true
                }
                arg.is_required = is_required_positional
                if (find_bool_annotation(field.annotation, "clarg_required", false) && arg.is_optional_wrap) {
                    errors := "field '{arg.field_name}': @clarg_required cannot be combined with Option<...> @clarg_positional (declare the field as plain string instead)"
                    return <- result
                }
            }

            if (is_count) {
                if (is_positional) {
                    errors := "field '{arg.field_name}': @clarg_count cannot be combined with @clarg_positional"
                    return <- result
                }
                if (arg.is_array || arg.is_optional_wrap) {
                    errors := "field '{arg.field_name}': @clarg_count requires a plain int field"
                    return <- result
                }
                if (arg.value_type != Type.tInt) {
                    errors := "field '{arg.field_name}': @clarg_count requires int field type"
                    return <- result
                }
            }

            arg.doc_string = find_string_annotation(field.annotation, "clarg_doc", "")
            // Detect a user-declared --help/-h flag — suppresses auto-injection by parse_args_with_help.
            if (!is_positional && (arg.flag_name == "--help" || arg.short_flag_name == "-h")) {
                result.has_user_help = true
            }
            result.args |> emplace(arg)
        }
        return <- result
    }

    def make_command_info_fn(var st : StructurePtr; info : CommandInfo) : FunctionPtr {
        var fn = qmacro_function("get_command_info") $(typ : $t(st)) : CommandInfo {
            var x := $v(info)
            return <- x
        }
        fn.flags |= FunctionFlags.generated
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_parse_default_fn(var st : StructurePtr) : FunctionPtr {
        var fn = qmacro_function("parse_args") $(var dst : $t(st)) : string {
            return parse_args(dst, get_user_args())
        }
        fn.flags |= FunctionFlags.generated
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_parse_args_with_help_fn(var st : StructurePtr) : FunctionPtr {
        // Auto-handles --help/-h by printing format_help_with_auto_help and
        // returning 0.  Returns -1 on clean parse, 1 on parse error.
        var fn = qmacro_function("parse_args_with_help") $(var dst : $t(st); prog_name : string) : int {
            let auto_args <- get_user_args()
            if (find_index(auto_args, "--help") != -1 || find_index(auto_args, "-h") != -1) {
                let auto_info <- get_command_info(type<$t(st)>)
                print(format_help_with_auto_help(auto_info, prog_name))
                return 0
            }
            let auto_err = parse_args(dst, auto_args)
            if (!empty(auto_err)) {
                print("error: {auto_err}\n")
                return 1
            }
            return -1
        }
        fn.flags |= FunctionFlags.generated
        fn.body |> force_at(st.at)
        return <- fn
    }

    def make_simple_init_expr(var st : StructurePtr; i : int; arg : CommandArgumentInfo; field_index : int) : ExpressionPtr {
        return qmacro_block() {
            {
                var r <- parse_argument(args, command_info.args[$v(i)], type<$t(st.fields[field_index]._type)>)
                if (r |> is_err) {
                    return "{$v(arg.flag_name)}: {r |> unwrap_err}"
                }
                var opt <- r |> move_unwrap   // nolint:LINT003 — non-copyable static_if branch below consumes `opt` via `move_unwrap`, so it must stay `var`
                if (opt |> is_some) {
                    static_if (typeinfo can_copy(type<$t(st.fields[field_index]._type)>)) {
                        dst.$f(arg.field_name) = opt |> unwrap
                    } else {
                        dst.$f(arg.field_name) <- opt |> move_unwrap
                    }
                }
            }
        }
    }

    def make_optional_init_expr(var st : StructurePtr; i : int; arg : CommandArgumentInfo; field_index : int; inner_t : TypeDeclPtr) : ExpressionPtr {
        return qmacro_block() {
            {
                var r <- parse_argument(args, command_info.args[$v(i)], type<$t(inner_t)>)
                if (r |> is_err) {
                    return "{$v(arg.flag_name)}: {r |> unwrap_err}"
                }
                dst.$f(arg.field_name) <- r |> move_unwrap
            }
        }
    }

    def make_enum_init_expr(var st : StructurePtr; i : int; arg : CommandArgumentInfo; field_index : int) : ExpressionPtr {
        return qmacro_block() {
            {
                let opt = find_flag_raw_value(args, command_info.args[$v(i)])
                if (is_some(opt)) {
                    let raw_value = unwrap(opt)
                    if (find_index(command_info.args[$v(i)].enum_values, raw_value) == -1) {
                        return "{$v(arg.flag_name)}: invalid enum value '{raw_value}'"
                    }
                    dst.$f(arg.field_name) = to_enum(type<$t(st.fields[field_index]._type)>, raw_value)
                } elif (flag_present(args, command_info.args[$v(i)])) {
                    return "{$v(arg.flag_name)}: missing value"
                }
            }
        }
    }

    def make_required_positional_init_expr(arg : CommandArgumentInfo; positional_index : int) : ExpressionPtr {
        return qmacro_block() {
            {
                if ($v(positional_index) < length(positionals)) {
                    dst.$f(arg.field_name) = positionals[$v(positional_index)]
                } else {
                    return "{$v(arg.flag_name)}: missing required positional argument"
                }
            }
        }
    }

    def make_optional_positional_init_expr(arg : CommandArgumentInfo; positional_index : int) : ExpressionPtr {
        return qmacro_block() {
            {
                if ($v(positional_index) < length(positionals)) {
                    dst.$f(arg.field_name) <- some(positionals[$v(positional_index)])
                }
            }
        }
    }

    def make_array_positional_init_expr(arg : CommandArgumentInfo; positional_index : int) : ExpressionPtr {
        return qmacro_block() {
            {
                var rest_pos : array<string>
                let start = $v(positional_index)
                if (start < length(positionals)) {
                    rest_pos |> reserve(length(positionals) - start)
                }
                for (j in range(start, length(positionals))) {
                    rest_pos |> push(positionals[j])
                }
                dst.$f(arg.field_name) <- rest_pos
            }
        }
    }

    def make_count_init_expr(arg : CommandArgumentInfo; i : int) : ExpressionPtr {
        return qmacro_block() {
            {
                let count_viol = count_flag_value_violation(args, command_info.args[$v(i)])
                if (!empty(count_viol)) {
                    return count_viol
                }
                dst.$f(arg.field_name) = count_flag(args, command_info.args[$v(i)])
            }
        }
    }

    def make_parse_fn(var st : StructurePtr; info : CommandInfo) : FunctionPtr {
        var name_to_index <- { for (i in range(length(st.fields))); string(st.fields[i].name) => i }
        var field_index_by_name <- { for (arg in info.args); arg.field_name => name_to_index[arg.field_name]; where key_exists(name_to_index, arg.field_name) }

        var has_positional = false
        var has_mutex = false
        for (arg in info.args) {
            if (arg.is_positional) {
                has_positional = true
            }
            if (!empty(arg.mutex_group)) {
                has_mutex = true
            }
        }

        var init_exprs : array<ExpressionPtr>
        var positional_idx = 0
        for (i, arg in range(length(info.args)), info.args) {
            let field_index = field_index_by_name[arg.field_name]
            if (arg.is_positional) {
                let pi = positional_idx
                positional_idx ++
                if (arg.is_array) {
                    init_exprs |> push(make_array_positional_init_expr(arg, pi))
                } elif (arg.is_optional_wrap) {
                    init_exprs |> push(make_optional_positional_init_expr(arg, pi))
                } else {
                    init_exprs |> push(make_required_positional_init_expr(arg, pi))
                }
                continue
            }
            if (arg.is_count) {
                init_exprs |> push(make_count_init_expr(arg, i))
                continue
            }
            let inner_t = option_inner_type(st.fields[field_index]._type)
            if (inner_t != null) {
                init_exprs |> push(make_optional_init_expr(st, i, arg, field_index, inner_t))
                continue
            }
            if (arg.value_type != Type.tEnumeration) {
                init_exprs |> push(make_simple_init_expr(st, i, arg, field_index))
                continue
            }
            init_exprs |> push(make_enum_init_expr(st, i, arg, field_index))
        }

        if (has_positional && has_mutex) {
            var fn = qmacro_function("parse_args") $(var dst : $t(st); args : array<string>) : string {
                let command_info <- get_command_info(type<$t(st)>)
                let positionals <- collect_positional_args(args, command_info)
                $b(init_exprs)
                let mutex_err = find_mutex_violation(args, command_info)
                if (!empty(mutex_err)) {
                    return mutex_err
                }
                for (arg in command_info.args) {
                    if (arg.is_required && !arg.is_positional && !arg.is_count && !flag_present(args, arg)) {
                        return "{arg.flag_name}: missing required flag"
                    }
                }
                return ""
            }
            fn.flags |= FunctionFlags.generated
            fn.body |> force_at(st.at)
            return <- fn
        }
        if (has_positional) {
            var fn = qmacro_function("parse_args") $(var dst : $t(st); args : array<string>) : string {
                let command_info <- get_command_info(type<$t(st)>)
                let positionals <- collect_positional_args(args, command_info)
                $b(init_exprs)
                for (arg in command_info.args) {
                    if (arg.is_required && !arg.is_positional && !arg.is_count && !flag_present(args, arg)) {
                        return "{arg.flag_name}: missing required flag"
                    }
                }
                return ""
            }
            fn.flags |= FunctionFlags.generated
            fn.body |> force_at(st.at)
            return <- fn
        }
        if (has_mutex) {
            var fn = qmacro_function("parse_args") $(var dst : $t(st); args : array<string>) : string {
                let command_info <- get_command_info(type<$t(st)>)
                $b(init_exprs)
                let mutex_err = find_mutex_violation(args, command_info)
                if (!empty(mutex_err)) {
                    return mutex_err
                }
                for (arg in command_info.args) {
                    if (arg.is_required && !arg.is_count && !flag_present(args, arg)) {
                        return "{arg.flag_name}: missing required flag"
                    }
                }
                return ""
            }
            fn.flags |= FunctionFlags.generated
            fn.body |> force_at(st.at)
            return <- fn
        }
        var fn = qmacro_function("parse_args") $(var dst : $t(st); args : array<string>) : string {
            let command_info <- get_command_info(type<$t(st)>)
            $b(init_exprs)
            for (arg in command_info.args) {
                if (arg.is_required && !arg.is_count && !flag_present(args, arg)) {
                    return "{arg.flag_name}: missing required flag"
                }
            }
            return ""
        }
        fn.flags |= FunctionFlags.generated
        fn.body |> force_at(st.at)
        return <- fn
    }
}
