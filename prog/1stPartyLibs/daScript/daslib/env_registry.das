options gen2

//! Shared scaffolding for a module's environment-knob registry. A registry module declares its
//! knobs as `[EnvConfig]` structs (daslib/clargs) and loads them once into `g_env_*` globals;
//! this module carries everything around that declaration that is the same for every adopter:
//! the ENVIRONMENT.md renderers, the enforcement-test scanners, and the sanctioned dynamic-name
//! readers. Worked examples: `dasllama/dasllama_env.das`, `llvm/daslib/llvm_env.das`.

module env_registry shared public

require daslib/clargs
require daslib/rtti
require daslib/fio
require strings
require daslib/strings_boost

// ===== dynamic-name reads (the ONE sanctioned escape hatch) =====
// For names known only at runtime, and for ambient variables a registry lists but does not own
// (HOME, PROCESSOR_IDENTIFIER, ...). Same set-but-empty-is-unset rule as the loaders. Not for
// hot paths, and a LITERAL knob name through these must still be declared in the registry.

def public env_is_set(name : string) : bool {
    return has_env_variable(name) && !empty(get_env_variable(name))
}

def public env_value_of(name : string) : string {
    return get_env_variable(name)
}

//! A hand-written doc row for knobs no struct field can tell the truth about (compile-time
//! constants, foreign variables) — appended to a section after its `write_env_info_rows`.
struct public EnvDocRow {
    name : string
    kind : string
    dflt : string
    doc : string
}

def public write_env_row(var w : StringBuilderWriter; name, kind, dflt, doc : string) {
    w |> write("| `{name}` | {kind} | {empty(dflt) ? "unset" : dflt} | {doc} |\n")
}

def public write_env_row(var w : StringBuilderWriter; row : EnvDocRow) {
    write_env_row(w, row.name, row.kind, row.dflt, row.doc)
}

def public write_env_info_rows(var w : StringBuilderWriter; info : CommandInfo) {
    for (a in info.args) {
        var kind = "number"
        if (a.value_type == Type.tBool) {
            kind = "flag"
        } elif (a.value_type == Type.tString) {
            kind = a.is_path ? "path" : "text"
        }
        var dflt = a.default_doc
        if (a.value_type == Type.tBool) {
            dflt = dflt == "true" ? "on" : (dflt == "false" ? "off" : dflt)
        }
        write_env_row(w, a.env_name, kind, dflt, a.doc_string)
    }
}

def public write_env_section(var w : StringBuilderWriter; title, note : string) {
    w |> write("\n## {title}\n\n")
    if (!empty(note)) {
        w |> write("{note}\n\n")
    }
    w |> write("| Variable | Type | Default | Effect |\n|---|---|---|---|\n")
}

//! The loader-semantics paragraph — clargs behaviour, so the wording is shared verbatim. The
//! registry's own header (title + where it generates from + the regen command) stays with the
//! registry, since those name its files.
def public write_env_doc_semantics(var w : StringBuilderWriter) {
    w |> write("Types: **flag** is unset-means-default, `0`/`false`/`off`/`no` (any case) is false and\n")
    w |> write("anything else true; **number** falls back to the default when unset or unparseable, with\n")
    w |> write("a logged warning on garbage; **text** and **path** are taken verbatim. A SET-BUT-EMPTY\n")
    w |> write("variable counts as unset everywhere. Every knob loads ONCE, at context init, into the\n")
    w |> write("`g_env_*` globals - hot code reads struct fields, and `set_env_variable` after startup\n")
    w |> write("is invisible (arm a child process's environment instead).\n")
}

//! Every env name an `[EnvConfig]` struct declares, appended in declaration order — feeds the
//! registry's `registered_env_names()`.
def public push_info_names(var out : array<string>; info : CommandInfo) {
    out |> reserve(length(out) + length(info.args))
    for (a in info.args) {
        out |> push(a.env_name)
    }
}

//! The generator-driver body: write the rendered doc, log the outcome. Callers pass an
//! absolute path (usually `{get_das_root()}/...`).
def public save_generated_doc(path, body : string) : bool {
    var f = fopen(path, "wb")
    if (f == null) {
        to_log(LOG_ERROR, "env_registry: cannot write {path}\n")
        return false
    }
    fwrite(f, body)
    fclose(f)
    to_log(LOG_INFO, "env_registry: wrote {path} ({long_length(body)} bytes)\n")
    return true
}

let public ENV_READ_MARKERS = [
    "get_env_variable(\"",
    "has_env_variable(\"",
    "env_config_flag(\"",
    "env_config_int(\"",
    "env_config_int64(\"",
    "env_config_float(\"",
    "env_config_string(\"",
    "env_config_text(\""
]

let public ENV_SANCTIONED_MARKERS = [
    "env_is_set(\"",
    "env_value_of(\""
]

let public ENV_WRITE_MARKERS = [
    "set_env_variable(\""
]

let public ENV_BANNED_HELPERS = [
    "def private env_or(",
    "def env_or(",
    "def private pf_env_int(",
    "def private win_env_int(",
    "def env_int64(",
    "def private env_int64(",
    "def env_int(",
    "def private env_int(",
    "def env_str(",
    "def private env_str(",
    "def env_flag(",
    "def private env_flag(",
    "def env_int64_min(",
    "def private env_int64_min("
]

def private skip_path(path : string; skip : array<string>) : bool {
    let p = replace(path, "\\", "/")
    if (find(p, "_aot_generated") >= 0) {
        return true
    }
    for (s in skip) {
        if (find(p, s) >= 0) {
            return true
        }
    }
    return false
}

def private walk_das(dirpath : string; skip : array<string>; blk : block<(path : string; text : string) : void>) {
    dir(dirpath) $(name : string) {
        return if (name == "." || name == "..")
        let full = "{dirpath}/{name}"
        if (stat(full).is_dir) {
            return if (name == "_aot_generated" || name == "_llvm_aot_generated")
            walk_das(full, skip, blk)
        } elif (ends_with(name, ".das") && !skip_path(full, skip)) {
            let text = fread(full)
            if (!empty(text)) {
                blk |> invoke(full, text)
            }
        }
    }
}

def private collect_names(text : string; markers : array<string>; var out : table<string>) {
    peek_data(text) $(d) {
        for (marker in markers) {
            var from = 0
            while (true) {
                let hit = find(d, marker, from)
                break if (hit < 0)
                let start = hit + length(marker)
                let close = find(d, "\"", start)
                break if (close < 0)
                let name = slice(d, start, close)
                if (!empty(name) && !key_exists(out, name)) {
                    out |> insert(name)
                }
                from = close + 1
            }
        }
    }
}

//! Files under `roots` (das-root-relative) that read the environment raw — for the strict
//! "knobs are `g_env_*` fields" check. `skip` = path substrings to exempt (the registry file,
//! the test itself).
def public scan_env_raw_reads(roots, skip : array<string>) : array<string> {
    var offenders : array<string>
    for (root in roots) {
        walk_das("{get_das_root()}/{root}", skip) $(path, text) {
            for (marker in ENV_READ_MARKERS) {
                if (find(text, marker) >= 0) {
                    offenders |> push("{path}: {marker}...\")")
                    break
                }
            }
        }
    }
    return <- offenders
}

//! Every literal env name read, written, or dynamically probed under the roots, plus the file
//! count (a coverage floor: a broken walk must not pass vacuously).
def public scan_env_used_names(roots, skip : array<string>; var used : table<string>) : int {
    var files = 0
    for (root in roots) {
        walk_das("{get_das_root()}/{root}", skip) $(_path, text) {
            files++
            collect_names(text, ENV_READ_MARKERS, used)
            collect_names(text, ENV_SANCTIONED_MARKERS, used)
            collect_names(text, ENV_WRITE_MARKERS, used)
        }
    }
    return files
}

//! Files under the roots re-declaring a private env-reader helper.
def public scan_env_banned_helpers(roots, skip : array<string>) : array<string> {
    var offenders : array<string>
    for (root in roots) {
        walk_das("{get_das_root()}/{root}", skip) $(path, text) {
            for (sig in ENV_BANNED_HELPERS) {
                if (find(text, sig) >= 0) {
                    offenders |> push("{base_name(path)}: {sig}...)")
                }
            }
        }
    }
    return <- offenders
}

//! Names in `used` with no declaration. Ambient variables the registry lists but does not
//! own (HOME, PROCESSOR_IDENTIFIER, ...) belong in `declared` too — fold them into the
//! registry's `registered_env_names()`.
def public env_undeclared_names(used : table<string>; declared : array<string>) : array<string> {
    var known : table<string>
    for (n in declared) {
        known |> insert(n)
    }
    var missing <- [for (n in keys(used)); n; where !key_exists(known, n)]
    delete known
    return <- missing
}

//! Duplicate declarations across the registry's area structs.
def public env_duplicate_names(names : array<string>) : array<string> {
    var seen : table<string>
    var dupes : array<string>
    for (n in names) {
        if (key_exists(seen, n)) {
            dupes |> push(n)
        } else {
            seen |> insert(n)
        }
    }
    delete seen
    return <- dupes
}

//! Rendered doc rows whose Effect column is empty.
def public env_undocumented_rows(md : string) : array<string> {
    var undocumented : array<string>
    for (line in split(md, "\n")) {
        continue if (!starts_with(line, "| `"))
        var cols <- split(line, "|")
        if (length(cols) >= 5 && empty(strip(cols[4]))) {
            undocumented |> push(strip(cols[1]))
        }
        delete cols
    }
    return <- undocumented
}
