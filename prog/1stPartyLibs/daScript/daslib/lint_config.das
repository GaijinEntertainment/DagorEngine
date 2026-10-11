options gen2
options indenting = 4
options no_global_variables = false

module lint_config shared private

//! Repo-level lint enable/disable config.
//!
//! Loads ``{get_das_root()}/.lint_config`` (a TOML file) and folds its
//! ``[rules]`` table into a ``disabled_codes`` set consumed by the three
//! ``[lint_macro]`` pass-macros (``daslib/lint``, ``daslib/perf_lint``,
//! ``daslib/style_lint``), by the standalone runner ``utils/lint/main.das``,
//! and by the MCP ``lint`` subtool.
//!
//! Format (TOML 1.0). One ``[rules]`` table; each entry sets a rule to
//! ``true`` (on) or ``false`` (off):
//!
//!   [rules]
//!   STYLE005 = true     # re-enable a default-off rule
//!   PERF007  = false    # disable a default-on rule
//!
//! Anything that is not a boolean entry under ``[rules]`` is ignored.
//! If the file is missing, unreadable, or fails to parse the call is a
//! no-op — built-in defaults stand.
//!
//! Two environment variables feed the same cascade: ``DAS_LINT_CONFIG_PATH``
//! redirects which file is read, and ``DAS_LINT_DISABLE`` (comma-separated
//! codes) turns rules off for one run, overriding everything above.
//!
//! The ``[format]`` table works differently: it CASCADES per folder. Every
//! directory may hold its own ``.lint_config``; ``format_policy_for`` walks
//! from the file's directory to the repository root and the nearest file
//! that defines a key wins that key (``[rules]`` stays repo-root-only).
//!
//! The ``[docs]`` table is a FOLDER property — no cascade, no inheritance.
//! ``rule_docs_only = true`` marks a folder that carries rule documents only;
//! ``rule_docs_only_at`` reads that one file, and the LINT025 pass in
//! ``utils/lint/main.das`` reports any other ``.md`` sitting beside the code.
//! The ``[arch]`` reverse pass (LINT026: an anchor no code cites) and the LINT027
//! line gate over rule documents need no tag - they run in every folder.

require daslib/fio
require daslib/json
require daslib/toml
require daslib/ast_boost
require daslib/strings_boost

//! Path override env var. When ``DAS_LINT_CONFIG_PATH`` is set, the macro
//! cascade reads that path instead of ``{get_das_root()}/.lint_config`` —
//! useful for tests that exercise the cascade against a fixture without
//! touching the real repo file, and for CI runs that want to enforce a
//! specific policy.
def private resolved_config_path() : string {
    if (has_env_variable("DAS_LINT_CONFIG_PATH")) {
        let v = get_env_variable("DAS_LINT_CONFIG_PATH")
        if (!empty(v)) return v
    }
    return "{get_das_root()}/.lint_config"
}

//! Ad-hoc rule opt-out for one run: ``DAS_LINT_DISABLE=PERF026,LINT017`` (bare codes, comma
//! separated, surrounding whitespace ignored, unknown codes harmless). Needs no source edit —
//! the "let me add a log line while I debug this" hatch. Applied last, so it overrides the config.
def public load_env_disabled(var disabled_codes : table<string>) : void {
    return if (!has_env_variable("DAS_LINT_DISABLE"))
    for (part in split(get_env_variable("DAS_LINT_DISABLE"), ",")) {
        let code = strip(part)
        if (!empty(code) && !key_exists(disabled_codes, code)) {
            disabled_codes |> insert(code)
        }
    }
}

//! Loads the repo-level lint config (``{get_das_root()}/.lint_config`` by default;
//! ``$DAS_LINT_CONFIG_PATH`` if set) into ``disabled_codes``, then layers ``$DAS_LINT_DISABLE``
//! on top. No-op when the file is missing or unreadable.
def public load_lint_config(var disabled_codes : table<string>) : void {
    load_lint_config_from_path(resolved_config_path(), disabled_codes)
    load_env_disabled(disabled_codes)
}

//! Parser entry point. Reads ``path`` (TOML) and folds the ``[rules]``
//! table into ``disabled_codes``. Public so tests can exercise the parser
//! hermetically without writing into the repo root.
def public load_lint_config_from_path(path : string; var disabled_codes : table<string>) : void {
    let body = fread(path)
    if (empty(body)) return
    var error : string
    var root : JsonValue? = read_toml(body, error)
    if (root == null || !(root.value is _object)) return
    var root_obj & = unsafe(root.value as _object)
    if (!key_exists(root_obj, "rules")) return
    var rules_obj : JsonValue? = root_obj["rules"]
    if (rules_obj == null || !(rules_obj.value is _object)) return
    var rules & = unsafe(rules_obj.value as _object)
    for (code, val_jv in keys(rules), values(rules)) {
        if (val_jv == null || !(val_jv.value is _bool)) continue
        let enabled = val_jv.value as _bool
        if (enabled) {
            if (key_exists(disabled_codes, code)) {
                disabled_codes |> erase(code)
            }
        } else {
            if (!key_exists(disabled_codes, code)) {
                disabled_codes |> insert(code)
            }
        }
    }
}

//! Seeds ``disabled_codes`` with the canonical default-off rule set (STYLE005, LINT029 and LINT030). Call before ``load_lint_config`` so that a ``STYLE005 = true`` directive in ``.lint_config`` can re-enable it.
def public seed_default_disabled(var disabled_codes : table<string>) : void {
    for (code in ["STYLE005", "LINT029", "LINT030"]) {
        if (!key_exists(disabled_codes, code)) {
            disabled_codes |> insert(code)
        }
    }
}

var private _cache_off : table<string>
var private _cache_on  : table<string>
var private _cache_path_excludes : array<string>
var private _cache_loaded = false

def private ensure_cache_loaded() : void {
    if (_cache_loaded) return
    _cache_loaded = true
    load_path_excludes_from_path(resolved_config_path(), _cache_path_excludes)
    let body = fread(resolved_config_path())
    if (empty(body)) return
    var error : string
    var root : JsonValue? = read_toml(body, error)
    if (root == null || !(root.value is _object)) return
    var root_obj & = unsafe(root.value as _object)
    if (!key_exists(root_obj, "rules")) return
    var rules_obj : JsonValue? = root_obj["rules"]
    if (rules_obj == null || !(rules_obj.value is _object)) return
    var rules & = unsafe(rules_obj.value as _object)
    for (code, val_jv in keys(rules), values(rules)) {
        if (val_jv == null || !(val_jv.value is _bool)) continue
        let enabled = val_jv.value as _bool
        if (enabled) {
            if (!key_exists(_cache_on, code)) _cache_on |> insert(code)
        } else {
            if (!key_exists(_cache_off, code)) _cache_off |> insert(code)
        }
    }
}

//! Parses the ``[paths] exclude`` glob array from `path` (TOML) into `excludes`.
//! Public so tests can exercise the parser hermetically; non-string entries are ignored.
def public load_path_excludes_from_path(path : string; var excludes : array<string>) : void {
    let body = fread(path)
    if (empty(body)) return
    var error : string
    var root : JsonValue? = read_toml(body, error)
    if (root == null || !(root.value is _object)) return
    var root_obj & = unsafe(root.value as _object)
    if (!key_exists(root_obj, "paths")) return
    var paths_obj : JsonValue? = root_obj["paths"]
    if (paths_obj == null || !(paths_obj.value is _object)) return
    var paths & = unsafe(paths_obj.value as _object)
    if (!key_exists(paths, "exclude")) return
    var excl_jv : JsonValue? = paths["exclude"]
    if (excl_jv == null || !(excl_jv.value is _array)) return
    for (pat_jv in excl_jv.value as _array) {
        if (pat_jv != null && pat_jv.value is _string) {
            let pat = pat_jv.value as _string
            if (!empty(pat)) excludes |> push("{pat}")
        }
    }
}

//! True when `file` matches any glob in `excludes`. Patterns are repo-root-relative
//! (``tests/**``); `file` may be absolute or relative, either separator.
def public matches_path_excludes(file : string; excludes : array<string>) : bool {
    return false if (empty(excludes))
    let gen = to_generic_path(file)
    var rel = gen
    let root = to_generic_path(get_das_root())
    if (!empty(root) && starts_with(gen, root + "/")) {
        rel = slice(gen, length(root) + 1)
    }
    for (pat in excludes) {
        if (match_glob(pat, rel) || match_glob(pat, gen)) return true
    }
    return false
}

//! True when `file` falls under a ``[paths] exclude`` glob in the repo ``.lint_config``.
//! Excluded files still compile through lint — only report-time findings drop
//! (the filter lives in ``utils/lint/main.das``; escape hatch ``--lint-excluded-paths``).
def public is_lint_path_excluded(file : string) : bool {
    ensure_cache_loaded()
    return matches_path_excludes(file, _cache_path_excludes)
}

//! True when the repo ``.lint_config`` carries an explicit ``CODE = true`` directive. Rules with
//! a path-scoped default (STYLE039) read this as the "everywhere" arm of their tri-state:
//! ``= false`` disables via the disabled_codes rail, absent means the rule's own path default.
def public lint_config_forces_on(code : string) : bool {
    ensure_cache_loaded()
    return key_exists(_cache_on, code)
}

//! Macro-side helper: builds the ``disabled_codes`` table the three ``[lint_macro]`` apply() methods need. Honors the module-local ``options _enable_default_off_rules = true`` opt-in (skips BOTH the ``seed_default_disabled`` step and the repo config's ``off`` directives, so default-off rules fire for that module and repo policy cannot silence the rule a fixture exercises), then layers cached ``on`` directives and the environment on top. Single source so a future default-off PERF/LINT rule applies uniformly.
def public build_lint_macro_disabled(prog : ProgramPtr) : table<string> {
    let force_default_off = prog._options |> find_arg("_enable_default_off_rules") ?as tBool ?? false
    var disabled : table<string>
    if (!force_default_off) {
        seed_default_disabled(disabled)
    }
    ensure_cache_loaded()
    if (!force_default_off) {
        for (code in keys(_cache_off)) {
            if (!key_exists(disabled, code)) disabled |> insert(code)
        }
    }
    for (code in keys(_cache_on)) {
        if (key_exists(disabled, code)) disabled |> erase(code)
    }
    load_env_disabled(disabled)
    add_module_rule_overrides(prog, disabled)
    return <- disabled
}

//! The module-local rule overrides, layered LAST so the file has the final say: ``options _nolint``
//! silences the listed codes for this file, ``options _lint`` arms them (a default-off rule, or one
//! the repo config turned off). An enable beats a disable; an unknown code changes nothing.
def public add_module_rule_overrides(prog : ProgramPtr; var disabled_codes : table<string>) : void {
    apply_module_codes(prog, "_nolint", disabled_codes, true)
    apply_module_codes(prog, "_lint", disabled_codes, false)
}

def private apply_module_codes(prog : ProgramPtr; option : string;
                               var disabled_codes : table<string>; disable : bool) : void {
    let configured = prog._options |> find_arg(option)
    return if (!(configured is tString))
    for (part in split(configured as tString, ",")) {
        let code = part |> strip |> to_upper
        continue if (empty(code))
        if (disable) {
            if (!key_exists(disabled_codes, code)) {
                disabled_codes |> insert(code)
            }
        } elif (key_exists(disabled_codes, code)) {
            disabled_codes |> erase(code)
        }
    }
}

def private module_source_path(prog : ProgramPtr) : string {
    let mod = prog.getThisModule
    if (mod == null) return ""
    var res = ""
    peek(mod.fileName) $(fn) {
        res = fn |> replace("\\", "/")
    }
    return res
}

def private path_has_segment(file : string; seg : string) : bool {
    return starts_with(file, seg + "/") || find(file, "/" + seg + "/") >= 0
}

//! True when the compiling module's source lives under ``daslib/``.
//! Default gate for rules that hold the standard library to a stricter bar (STYLE014/STYLE015).
def public is_daslib_source(prog : ProgramPtr) : bool {
    return path_has_segment(module_source_path(prog), "daslib")
}

//! True when the compiling module's source lives under ``daslib/`` or ``modules/`` -
//! shipped library code whose emitted strings flow into every consumer's logs (STYLE039).
def public lint029_source_exempt(prog : ProgramPtr) : bool {
    return path_has_segment(module_source_path(prog), "daslib")
}

def public is_shipped_library_source(prog : ProgramPtr) : bool {
    let file = module_source_path(prog)
    return path_has_segment(file, "daslib") || path_has_segment(file, "modules")
}

//! True under ``daslib/`` or ``utils/`` - the trees kept free of duplicated regions and unused
//! private declarations (STYLE040/LINT022). Elsewhere both are a backlog, not a gate.
def public is_core_library_source(prog : ProgramPtr) : bool {
    let file = module_source_path(prog)
    return ((path_has_segment(file, "daslib") || path_has_segment(file, "utils"))
        && !path_has_segment(file, "modules"))
}

//! Per-file format policy from the ``[format]`` table of cascading ``.lint_config`` files:
//! ``format_enabled = false`` exempts a tree (load-bearing fixture layout),
//! ``force_clean_comments = true`` strips comments outside the kept set (swept trees).
struct public FormatPolicy {
    format_enabled : bool = true
    force_clean_comments : bool = false
}

// Parses `path`, runs `blk` over its [table_name] table when present, then frees the
// parsed tree - read_toml mints it, and no view may outlive this scope.
def private with_config_table(path, table_name : string; blk : block<(var tbl : JsonValue?) : void>) {
    let body = fread(path)
    return if (empty(body))
    var error : string
    var root : JsonValue? = read_toml(body, error)
    return if (root == null)
    if (root.value is _object) {
        var root_obj & = unsafe(root.value as _object)
        if (key_exists(root_obj, table_name)) {
            var tbl_jv : JsonValue? = root_obj[table_name]
            if (tbl_jv != null && tbl_jv.value is _object) {
                blk |> invoke(tbl_jv)
            }
        }
    }
    unsafe {
        delete root
    }
}

// One bool key of the [docs] table of exactly `config_path` - a folder property, so no cascade
// and no inheritance. A missing file, missing table, missing key or non-bool value reads as false.
def private docs_flag_at(config_path, key : string) : bool {
    var result = false
    with_config_table(config_path, "docs") $(var docs_jv) {
        var docs & = unsafe(docs_jv.value as _object)
        return if (!key_exists(docs, key))
        var val_jv : JsonValue? = docs[key]
        return if (val_jv == null || !(val_jv.value is _bool))
        result = val_jv.value as _bool
    }
    return result
}

//! True when the ``.lint_config`` at `config_path` sets ``[docs] rule_docs_only = true`` — the folder
//! carries rule documents only (``REVIEW*.md`` / ``ARCHITECTURE*.md``). A folder property,
//! NOT the ``[format]`` cascade: only that one file is read, and anything missing reads as false.
def public rule_docs_only_at(config_path : string) : bool {
    return docs_flag_at(config_path, "rule_docs_only")
}

def private config_dir_of(file : string) : string {
    let dir = to_generic_path(dir_name(to_generic_path(file)))
    return empty(dir) ? "." : dir
}

def private take_format_key(var fmt_jv : JsonValue?; key : string; var dst : bool&; var decided : bool&) {
    return if (decided)
    var fmt & = unsafe(fmt_jv.value as _object)
    if (!key_exists(fmt, key)) return
    var val_jv : JsonValue? = fmt[key]
    if (val_jv == null || !(val_jv.value is _bool)) return
    dst = val_jv.value as _bool
    decided = true
}

//! One file the formatter will process, carrying its folder's comment policy.
struct public FormatJob {
    file : string
    strip_comments : bool
}

//! Splits ``files`` by policy: drops ``format_enabled = false`` entries into ``skipped``;
//! survivors come back as jobs carrying their folder's ``force_clean_comments``.
//! Single-threaded, like ``format_policy_for``.
def public partition_by_format_policy(var files : array<string>; var skipped : array<string>) : array<FormatJob> {
    var jobs : array<FormatJob>
    jobs |> reserve(length(files))
    var policy_by_dir : table<string; FormatPolicy>
    for (file in files) {
        let dir = config_dir_of(file)
        if (!key_exists(policy_by_dir, dir)) {
            policy_by_dir[dir] = format_policy_for(file)
        }
        let policy = policy_by_dir?[dir] ?? FormatPolicy()
        if (!policy.format_enabled) {
            skipped |> push(file)
            continue
        }
        jobs |> push(FormatJob(file = file, strip_comments = policy.force_clean_comments))
    }
    delete files
    return <- jobs
}

//! Walks ``.lint_config`` files from ``file``'s directory up to the repository root (the
//! directory holding ``.git``, read inclusively); the nearest file defining a key wins that
//! key. Single-threaded — resolve on the coordinating thread, pass results to jobs.
def public format_policy_for(file : string) : FormatPolicy {
    var pol = FormatPolicy()
    var enabled_decided = false
    var clean_decided = false
    var dir = config_dir_of(file)
    while (true) {
        let cfg = dir == "." ? ".lint_config" : "{dir}/.lint_config"
        with_config_table(cfg, "format") $(var fmt_jv) {
            take_format_key(fmt_jv, "format_enabled", pol.format_enabled, enabled_decided)
            take_format_key(fmt_jv, "force_clean_comments", pol.force_clean_comments, clean_decided)
        }
        break if (enabled_decided && clean_decided)
        var st : FStat
        break if (stat(path_join(dir, ".git"), st))
        let up = dir_name(dir)
        if (empty(up) || up == dir) {
            break if (dir == ".")
            dir = "."
        } else {
            dir = up
        }
    }
    return pol
}

//! True when `fn` is user-authored from lint's point of view: not generated, or a generated
//! CARRIER whose body is verbatim user AST - a lambda, a `@@` local function, or a generator
//! (which also carries the lambda flag - generateLambdaFunction is the one maker of both).
def public is_user_authored_body(fn : Function?) : bool {
    return true if (fn == null || !fn.flags.generated)
    return fn.flags._lambda || fn.moreFlags2.localFunction
}

//! True for the lint-fixture naming convention: an underscore-led basename containing
//! ``fixture``. Such files violate rules BY DESIGN, for tests that compile them or spawn
//! the lint CLI over them, so the runner skips them (re-enabled via ``--lint-fixtures``).
def public is_lint_fixture_name(base : string) : bool {
    return starts_with(base, "_") && find(base, "fixture") >= 0
}

let public LINT_SKIP_HEADER_LINES = 16

//! Content-based skip probe shared by the CLI runner and the MCP lint tool. An ``expect <code>`` file
//! (intentional compile errors) skips while ``expect_skips`` holds - a caller linting rule fixtures passes false;
//! a header ``// lint-skip-file: <reason>`` opts a file out whatever the caller. Returns the skip reason, or "" to lint.
def public lint_file_skip_reason(file : string; expect_skips : bool = true) : string {
    let text = fread(file)
    return "" if (empty(text))
    for (line, ln in split(text, "\n"), count()) {
        let trimmed = line |> strip
        return "(intentional compile errors via `expect`)" if (expect_skips && trimmed |> starts_with("expect "))
        if (ln < LINT_SKIP_HEADER_LINES && trimmed |> starts_with("// lint-skip-file")) {
            let colon = find(trimmed, ":")
            let reason = colon >= 0 ? strip(slice(trimmed, colon + 1)) : ""
            return empty(reason) ? "(lint-skip-file directive)" : "(lint-skip-file: {reason})"
        }
    }
    return ""
}

//! One structured lint finding — the machine-readable twin of the ``"<message> at <file>:<line>:<column>"`` strings the ``*_collect`` overloads emit. Produced by the ``*_collect_issues`` overloads in ``daslib/lint`` / ``daslib/perf_lint`` / ``daslib/style_lint`` for consumers that need positions (e.g. the LSP validate subtool).
struct public LintIssue {
    code : string           //!< rule id ("LINT002", "PERF018", "STYLE005"); "" when the message carries none
    message : string        //!< message text without the position suffix; may be multi-line (instanced-from chains)
    file : string           //!< source file as spelled by the compiler; "" when the location has no file info
    line : int              //!< 1-based start line
    column : int            //!< 0-based start column (byte offset — the LineInfo convention)
    last_line : int         //!< 1-based end line; 0 when unknown
    last_column : int       //!< 0-based end column (byte offset, end-exclusive)
}

//! Builds a ``LintIssue`` from a lint message and its source location.
//! Shared funnel helper for the three lint visitors.
def public make_lint_issue(message : string; at : LineInfo) : LintIssue {
    return LintIssue(
        code = extract_lint_code(message),
        message = message,
        file = at.fileInfo != null ? "{at.fileInfo.name}" : "",
        line = int(at.line),
        column = int(at.column),
        last_line = int(at.last_line),
        last_column = int(at.last_column))
}
