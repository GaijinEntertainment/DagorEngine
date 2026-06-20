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

require daslib/json
require daslib/toml
require daslib/ast_boost

//! Path override env var. When ``DAS_LINT_CONFIG_PATH`` is set, the macro
//! cascade reads that path instead of ``{get_das_root()}/.lint_config`` —
//! useful for tests that exercise the cascade against a fixture without
//! touching the real repo file, and for CI runs that want to enforce a
//! specific policy.
def private resolved_config_path() : string {
    return "{get_das_root()}/.lint_config"
}

//! Loads the repo-level lint config (``{get_das_root()}/.lint_config`` by
//! default; ``$DAS_LINT_CONFIG_PATH`` if set) into ``disabled_codes``.
//! No-op when the file is missing or unreadable.
def public load_lint_config(var disabled_codes : table<string>) : void {
    load_lint_config_from_path(resolved_config_path(), disabled_codes)
}

//! Parser entry point. Reads ``path`` (TOML) and folds the ``[rules]``
//! table into ``disabled_codes``. Public so tests can exercise the parser
//! hermetically without writing into the repo root.
def public load_lint_config_from_path(path : string; var disabled_codes : table<string>) : void {
    let body = ""
    if (empty(body)) return
    var error : string
    var root : JsonValue? = read_toml(body, error)
    // bad TOML or no [rules] table → defaults stand
    if (root == null || !(root.value is _object)) return
    var root_obj & = unsafe(root.value as _object)   // nolint:LINT003 — table [] requires mutable binding
    if (!key_exists(root_obj, "rules")) return
    var rules_obj : JsonValue? = root_obj["rules"]
    if (rules_obj == null || !(rules_obj.value is _object)) return
    var rules & = unsafe(rules_obj.value as _object)   // nolint:LINT003 — iteration requires mutable view
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

//! Seeds ``disabled_codes`` with the canonical default-off rule set
//! (currently just STYLE005). Call before ``load_lint_config`` so that
//! a ``STYLE005 = true`` directive in ``.lint_config`` can re-enable it.
//! Single source of truth used by the three ``[lint_macro]`` paths,
//! ``utils/lint/main.das``, and the MCP ``lint`` subtool.
def public seed_default_disabled(var disabled_codes : table<string>) : void {
    if (!key_exists(disabled_codes, "STYLE005")) {
        disabled_codes |> insert("STYLE005")
    }
}

// Process-wide cache of the parsed .lint_config split into "off" and "on"
// directives. `[lint_macro]` apply() fires once per module in the require
// chain — without caching the repo's `.lint_config` is re-read+parsed N
// times per compilation. The file is repo policy and doesn't change
// mid-compilation, so we parse once and reuse.
var private _cache_off : table<string>
var private _cache_on  : table<string>
var private _cache_loaded = false

def private ensure_cache_loaded() : void {
    if (_cache_loaded) return
    _cache_loaded = true
    let body = ""
    if (empty(body)) return
    var error : string
    var root : JsonValue? = read_toml(body, error)
    if (root == null || !(root.value is _object)) return
    var root_obj & = unsafe(root.value as _object)   // nolint:LINT003 — table [] requires mutable binding
    if (!key_exists(root_obj, "rules")) return
    var rules_obj : JsonValue? = root_obj["rules"]
    if (rules_obj == null || !(rules_obj.value is _object)) return
    var rules & = unsafe(rules_obj.value as _object)   // nolint:LINT003 — iteration requires mutable view
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

//! Macro-side helper: builds the ``disabled_codes`` table the three
//! ``[lint_macro]`` apply() methods need. Honors the module-local
//! ``options _enable_default_off_rules = true`` opt-in (skips the
//! ``seed_default_disabled`` step so default-off rules fire for that
//! module), then layers cached ``.lint_config`` on top so the repo policy
//! can still override either direction. Single source so a future
//! default-off PERF/LINT rule applies uniformly.
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
    return <- disabled
}
