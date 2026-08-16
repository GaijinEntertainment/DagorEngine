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
    // bad TOML or no [rules] table → defaults stand
    if (root == null || !(root.value is _object)) return
    var root_obj & = unsafe(root.value as _object)   // table [] requires mutable binding
    if (!key_exists(root_obj, "rules")) return
    var rules_obj : JsonValue? = root_obj["rules"]
    if (rules_obj == null || !(rules_obj.value is _object)) return
    var rules & = unsafe(rules_obj.value as _object)   // iteration requires mutable view
    for (code, val_jv in keys(rules), values(rules)) {
        if (val_jv == null || !(val_jv.value is _bool)) continue
        let enabled = val_jv.value as _bool
        if (enabled) {
            // CODE = true → on → remove from default-disabled
            if (key_exists(disabled_codes, code)) {
                disabled_codes |> erase(code)
            }
        } else {
            // CODE = false → off → add to disabled
            if (!key_exists(disabled_codes, code)) {
                disabled_codes |> insert(code)
            }
        }
    }
}

//! Seeds ``disabled_codes`` with the canonical default-off rule set (currently just STYLE005). Call before ``load_lint_config`` so that a ``STYLE005 = true`` directive in ``.lint_config`` can re-enable it.
def public seed_default_disabled(var disabled_codes : table<string>) : void {
    if (!key_exists(disabled_codes, "STYLE005")) {
        disabled_codes |> insert("STYLE005")
    }
}

// Process-wide cache of the parsed .lint_config split into "off" and "on" directives. `[lint_macro]` apply() fires once per module in the require chain — without caching the repo's `.lint_config` is re-read+parsed N times per compilation. The file is repo policy and doesn't change mid-compilation, so we parse once and reuse.
var private _cache_off : table<string>
var private _cache_on  : table<string>
var private _cache_loaded = false

def private ensure_cache_loaded() : void {
    if (_cache_loaded) return
    _cache_loaded = true
    let body = fread(resolved_config_path())
    if (empty(body)) return
    var error : string
    var root : JsonValue? = read_toml(body, error)
    if (root == null || !(root.value is _object)) return
    var root_obj & = unsafe(root.value as _object)   // table [] requires mutable binding
    if (!key_exists(root_obj, "rules")) return
    var rules_obj : JsonValue? = root_obj["rules"]
    if (rules_obj == null || !(rules_obj.value is _object)) return
    var rules & = unsafe(rules_obj.value as _object)   // iteration requires mutable view
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

//! True when the repo ``.lint_config`` carries an explicit ``CODE = true`` directive. Rules with
//! a path-scoped default (STYLE039) read this as the "everywhere" arm of their tri-state:
//! ``= false`` disables via the disabled_codes rail, absent means the rule's own path default.
def public lint_config_forces_on(code : string) : bool {
    ensure_cache_loaded()
    return key_exists(_cache_on, code)
}

//! Macro-side helper: builds the ``disabled_codes`` table the three ``[lint_macro]`` apply() methods need. Honors the module-local ``options _enable_default_off_rules = true`` opt-in (skips the ``seed_default_disabled`` step so default-off rules fire for that module), then layers cached ``.lint_config`` on top so the repo policy can still override either direction. Single source so a future default-off PERF/LINT rule applies uniformly.
def public build_lint_macro_disabled(prog : ProgramPtr) : table<string> {
    let force_default_off = prog._options |> find_arg("_enable_default_off_rules") ?as tBool ?? false
    var disabled : table<string>
    if (!force_default_off) {
        seed_default_disabled(disabled)
    }
    ensure_cache_loaded()
    for (code in keys(_cache_off)) {
        if (!key_exists(disabled, code)) disabled |> insert(code)
    }
    for (code in keys(_cache_on)) {
        if (key_exists(disabled, code)) disabled |> erase(code)
    }
    // last, so a one-run DAS_LINT_DISABLE beats a `CODE = true` in the repo config
    load_env_disabled(disabled)
    return <- disabled
}

// ---------------------------------------------------------------------------
// Path-based rule defaults - the ONE place lint rules key off source location
// ---------------------------------------------------------------------------

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
def public is_shipped_library_source(prog : ProgramPtr) : bool {
    let file = module_source_path(prog)
    return path_has_segment(file, "daslib") || path_has_segment(file, "modules")
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
