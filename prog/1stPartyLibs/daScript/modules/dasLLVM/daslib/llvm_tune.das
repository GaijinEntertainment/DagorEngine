options gen2
options indenting = 4
options no_aot = true
options no_global_variables = false
options no_aliasing = false

module llvm_tune shared public

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require daslib/templates_boost
require daslib/fio
require daslib/strings_boost
require daslib/command_line
require daslib/json_boost
require daslib/tty
require daslib/clargs
require strings
require math
require llvm/daslib/llvm_code public
require llvm/daslib/llvm_jit_cli
require llvm/daslib/llvm_env
require llvm/daslib/llvm_cpu_class
require llvm/daslib/llvm_code_shell   // the [llvm_code] spelling for the fat words below - never public, a program has daslib/tune's

//! Generalized tune framework (gemm_generator_plan.md part 2). `[tune_perm(...), ...,
//! tune(gen="key", fallback="suffix")]` — perm rows first, `tune` last — turns one reference
//! function into a tuned `[llvm_code]` kernel family. `[tune_companion(fn=, gen=)]` rows
//! (between the perm rows and `tune`) stamp sibling functions with the SAME perm from the same
//! manifest entry — the two-function stamp (a kernel and its repack-layout query can never
//! desync). `DAS_TUNE_MODE` selects the compile-time mode:
//!  - normal (default): stamp exactly one winner onto the function itself — the manifest
//!    entry when one exists, else this box's class entry in a shipped defaults profile
//!    (`[tune_scope(defaults=)]`), else the `fallback=` perm, else the reference body (a manifest
//!    with no entry for a function falls through to its fallback — new kernel families must
//!    not be dropped to the reference tier by a stale manifest; an explicit "reference"
//!    entry forces the original body). No variant stubs are emitted; the `<name>_variants()`
//!    registry still exists (reference row only) so harness code compiles in every mode.
//!  - tune / test: stamp the full grid as `<name>__<suffix>` clones plus a `<name>_variants()`
//!    registry; a harness benches them (tune, then `tune_manifest_set` the winner) or
//!    bit-exact-gates every variant against the reference row (test).
//!
//! Winners live in the PER-APP sidecar `<app>.tune.json` beside the app — the root script
//! this process runs (the first `.das` on the command line), or the binary itself when
//! there is none. One sectioned file per app: `"kernels"` (this module's stamps and the
//! loop-hint perms), `"runtime"` (library runtime knobs), `"provenance"` (who wrote it),
//! `"race"` (per-family race results behind each multi-variant crown).
//! A sidecar OLDER than the running binary is stale and reads as absent everywhere — a
//! rebuilt binary invalidates every measured winner. `DAS_TUNE_MANIFEST` env overrides the
//! location outright. A library declares `[tune_scope(name=, tuner=)]` to wire the harness
//! that regenerates its kernels; an app declares `[tune_policy(missing = fallback|warn|
//! error|auto|restart)]`; `--tune` after `--` on the app's command line forces a fresh
//! tuner run. AOT builds are fully tune-free (`tune_aot_gate`); `-exe` builds stamp
//! normally, run no policy, but always emit the status [init] so the artifact can
//! self-report its baked stamps (`tune_exe_gate`) — an exe built WITH a sidecar is a
//! local-use artifact shipping those winners, one built without ships the generic per-ISA
//! `fallback=` stamps.

// ===== manifest + mode =====

//! Compile-time tune mode from the `DAS_TUNE_MODE` env var: "normal" (default), "tune", "test",
//! "fat" (one clone per shipped CPU class, `-exe` only).
def tune_mode() : string {
    let m = g_env_tune.tune_mode
    return empty(m) ? "normal" : m
}

//! The jit codegen tier this process resolved: `--jit-opt-level` CLI over `policies.jit_opt_level`.
//! Compile-time — lets policy defaults key off the tier (winners are raced under O3 codegen,
//! so an O0 run cannot represent them and must not block on the tuner to mint them).
def jit_cli_opt_level() : int {
    let r <- parse_args(type<JitCliOptions>)
    let cli = r |> unwrap_or_default
    return cli.opt_level |> unwrap_or(compiling_program().policies.jit_opt_level)
}

def tune_aot_gate() : bool {
    return (compiling_program().policies.tune_frozen
        || compiling_program().policies.aot
        || !empty(get_target_triple()))
}

//! True when this compile must retain reference bodies. Documentation roots
//! inspect source APIs and, like frozen/AOT consumers, must never consult or
//! stamp per-box tune state.
def tune_stamp_free_gate() : bool {
    return tune_aot_gate() || is_building_documentation()
}

    //! True when this compile produces a standalone exe (`llvm-jit -exe`). Stamping still runs:
    //! with a sidecar the exe ships those winners, without one the generic `fallback=` stamps.
    //! Only the policy rail (`[tune_policy]`, `--tune`) is dead.
def tune_exe_gate() : bool {
    return compiling_program().policies.jit_exe_mode
}

// ===== the per-app sidecar =====

    //! The app this process runs: the first `.das` argument on the command line before `--`.
    //! Macro time and runtime read the same argv, so stamps and the write API agree on the file.
    //! "" in a standalone exe or embedded host — the binary itself is the app then.
def tune_app_script() : string {
    // aliases a process-global Array header (locked) — do not delete
    var argv <- get_command_line_arguments()
    var first = true
    for (a in argv) {
        if (!first) {
            break if (a == "--")
            if (a |> ends_with(".das")) {
                return "{a}"
            }
        }
        first = false
    }
    return ""
}

    //! The sidecar-name convention, `<stem>.tune.json`: foo/app.das, foo/app.exe and foo/app
    //! all map to foo/app.tune.json. The ONE implementation of the naming rule — callers that
    //! need "the sidecar beside this artifact" (oracles, release tooling) resolve through it.
def sidecar_for(app : string) : string {
    var stem = app
    if (stem |> ends_with(".das") || stem |> ends_with(".exe")) {
        stem = slice(stem, 0, length(stem) - 4)
    }
    return "{stem}.tune.json"
}

    // the resolved executable, NOT get_das_exe(): that derives the interpreter-to-SPAWN, which does
    // not exist inside a released bundle; argv[0] as typed is a bare name when PATH found the exe.
def private running_binary() : string {
    let exe = get_executable_path()
    if (!empty(exe)) return replace(exe, "\\", "/")
    // aliases a process-global Array header (locked) — do not delete
    let argv <- get_command_line_arguments()
    return empty(argv) ? "" : replace("{argv[0]}", "\\", "/")
}

// manual override for harnesses that manage their own sidecar location
var private g_runtime_manifest = ""

//! Point the get/set APIs (and nothing else — compile-time stamps read argv/env only) at an
//! explicit sidecar; harnesses that manage their own location call it before get/set.
def set_tune_manifest_runtime_path(p : string) {
    g_runtime_manifest = p
}

//! Whether the sidecar location was chosen (`set_tune_manifest_runtime_path` or `DAS_TUNE_MANIFEST`)
//! rather than defaulted beside the app.
def tune_manifest_explicit() : bool {
    return !empty(g_runtime_manifest) || !empty(g_env_tune.tune_manifest)
}

    //! The tune sidecar location: the in-process override (`set_tune_manifest_runtime_path` names
    //! a file of the process's own choosing, so it outranks the ambient env), else `DAS_TUNE_MANIFEST`,
    //! else `<app>.tune.json` beside the app (root script, or the binary). Never "".
def tune_manifest_path() : string {
    if (!empty(g_runtime_manifest)) return g_runtime_manifest
    let env = g_env_tune.tune_manifest
    if (!empty(env)) return env
    let app = tune_app_script()
    return sidecar_for(empty(app) ? running_binary() : app)
}

var private g_box_identity = ""

def private all_digits(s : string) : bool {
    if (empty(s)) {
        return false
    }
    var ok = true
    peek_data(s) $(arr) {
        for (b in arr) {
            ok = ok && is_number(int(b))
        }
    }
    return ok
}

// the substring from the first digit to the end — locale-proof extraction of the version
// number out of a `ver` banner whose leading word ("Version", "Versión", "Версия", …) varies
def private digits_tail(s : string) : string {
    var at = -1
    peek_data(s) $(arr) {
        for (i in range(length(arr))) {
            if (is_number(int(arr[i]))) {
                at = i
                break
            }
        }
    }
    return at >= 0 ? slice(s, at) : ""
}

// The CPU brand from /proc/cpuinfo — x86 has "model name", arm64 does not (it carries "Model" /
// "Hardware" / "CPU part"); try the keys in order so the identity's cpu field is never empty
def private linux_cpu_brand() : string {
    var brand = ""
    for (key in fixed_array("model name", "Model", "Hardware", "CPU part")) {
        fopen("/proc/cpuinfo", "rb") $(f) {
            if (f != null) {
                while (!feof(f)) {
                    let ln = fgets(f)
                    if (ln |> starts_with(key)) {
                        let ix = find(ln, ":")
                        if (ix >= 0) {
                            brand = strip(slice(ln, ix + 1))
                        }
                        break
                    }
                }
            }
        }
        break if (!empty(brand))
    }
    return brand
}

    //! This box's tune identity: platform|arch|model_id|os_build|cpu — a sidecar carrying a
    //! different identity reads as stale (the cross-box hazard). Always five fields, non-empty
    //! cpu (the schema requires it); os_build keeps its full value, `box_match_key` folds it.
def tune_box_identity() : string {
    if (!empty(g_box_identity)) {
        return g_box_identity
    }
    var model_id = ""   // nolint:LINT003 - only the macOS branch writes it, and platform folding drops that branch elsewhere
    var os_build = ""
    var cpu : string
    let plat = get_platform_name()
    if (plat == "windows") {
        var build = ""
        unsafe(popen("ver") $(f) {
            if (f != null) {
                while (!feof(f)) {
                    let ln = strip(fgets(f))
                    if (!empty(ln)) {
                        build = ln
                    }
                }
            }
        })
        let lb = find(build, "[")
        let rb = find(build, "]")
        // keep only the trailing [0-9.] version run, so a localized "Version" word (and its
        // console-codepage bytes) never enters the identity
        os_build = (lb >= 0 && rb > lb) ? digits_tail(strip(slice(build, lb + 1, rb))) : ""
        if (empty(os_build)) {
            os_build = env_value_of("OS")
        }
        cpu = env_value_of("PROCESSOR_IDENTIFIER")
    } elif (plat == "osx" || plat == "darwin") {   // get_platform_name says "darwin" on macOS
        var lines : array<string>
        unsafe(popen("sysctl -n hw.model kern.osversion machdep.cpu.brand_string 2>/dev/null") $(f) {
            while (!feof(f)) {
                let ln = strip(fgets(f))
                if (!empty(ln)) {
                    lines |> push(ln)
                }
            }
        })
        // positional: a missing sysctl key blank-fills its field rather than collapsing the count
        model_id = !empty(lines) ? lines[0] : ""
        os_build = length(lines) > 1 ? lines[1] : ""
        cpu = length(lines) > 2 ? lines[2] : ""
        delete lines
    } else {
        unsafe(popen("uname -r 2>/dev/null") $(f) {
            os_build = strip(fgets(f))
        })
        cpu = linux_cpu_brand()
    }
    if (empty(cpu)) {
        cpu = "unknown-cpu"   // the schema rejects an empty cpu field; never emit one
    }
    g_box_identity = "{plat}|{get_architecture_name()}|{model_id}|{os_build}|{cpu}"
    return g_box_identity
}

    //! Comparison key for a box identity — stored and shown verbatim, but staleness and same-box
    //! tests fold the volatile Windows `os_build` UBR (4th dotted component) so a monthly patch
    //! neither invalidates a sidecar nor blocks its resubmit. Lowercased; other builds unchanged.
def box_match_key(box : string) : string {
    let parts <- split(box, "|")
    if (length(parts) != 5) {
        return to_lower(box)
    }
    var osb = parts[3]
    let dots <- split(osb, ".")
    if (length(dots) == 4 && all_digits(dots[0]) && all_digits(dots[1])
        && all_digits(dots[2]) && all_digits(dots[3])) {
        osb = "{dots[0]}.{dots[1]}.{dots[2]}"   // drop the UBR
    }
    return to_lower("{parts[0]}|{parts[1]}|{parts[2]}|{osb}|{parts[4]}")
}

// provenance.box of the sidecar at `path`, "" when absent/unreadable — cached per path+mtime
// (the staleness gate runs per [tuned] compile read; one parse per file per process is enough)
var private g_box_cache : table<string; string>

def private sidecar_box(path : string; mkey : string) : string {
    let key = "{path}@{mkey}"
    if (g_box_cache |> key_exists(key)) {
        return g_box_cache[key]
    }
    var box = ""
    let text = fread(path)
    if (!empty(text)) {
        var err = ""
        var doc = read_json(text, err)
        if (doc != null) {
            // applied_box wins: a resolver-adopted cross-box sidecar records the ADOPTING box there; a hand-copy (no applied_box) still reads stale
            box = doc?["provenance"]?["applied_box"] ?? ""
            if (empty(box)) {
                box = doc?["provenance"]?["box"] ?? ""
            }
            delete_json(doc)
        }
    }
    g_box_cache[key] = box
    return box
}

// a hand-edited sidecar may carry a number where tune_provenance_note writes strings — read both
def private sidecar_prov_value(path : string; key : string) : string {
    var r = ""
    let text = fread(path)
    if (!empty(text)) {
        var err = ""
        var doc = read_json(text, err)
        if (doc != null) {
            let v = doc?["provenance"]?[key]
            if (v != null && v is _string) {
                r = v as _string
            } elif (v != null && v is _number) {
                r = "{int(v as _number)}"
            } elif (v != null && v is _longint) {
                r = "{v as _longint}"   // the parser stores json integers in the _longint arm
            }
            delete_json(doc)
        }
    }
    return r
}

    //! True when the sidecar predates the running binary OR carries a different box identity —
    //! the stale-file and cross-box-copy traps; stale reads as ABSENT everywhere. A
    //! `provenance.applied_box` naming this box overrides the identity check (resolver adoption).
def tune_sidecar_stale(path : string) : bool {
    var sfs : FStat
    return false if (!stat(path, sfs))   // absent is "missing", not "stale"
    var bfs : FStat
    if (stat(running_binary(), bfs) && sfs.mtime < bfs.mtime) {
        return true
    }
    return box_match_key(sidecar_box(path, "{sfs.mtime}")) != box_match_key(tune_box_identity())
}

    //! The compile-time half of a sidecar or profile read: the module cache records the file, existing
    //! or not, so a re-mint invalidates the stamps minted against it. Kept out of every reader the
    //! runtime shares - a standalone exe binds each extern at startup (the cited section says why).
[arch(at="../ARCHITECTURE.md#tune-sidecar-cache-pin")]
def private pin_module_cache_dependency(path : string) {
    add_module_cache_dependency(path)
}

def private pin_profile_chain(defaultsDir : string) {
    // a fat build reads every class of the ladder, not only the chain below this box's class
    var chain <- tune_mode() == "fat" ? fat_ladder() : tune_class_chain()
    for (c in chain) {
        pin_module_cache_dependency(path_join(defaultsDir, "{c}.tune-defaults.json"))
    }
    delete chain
}

// the whole ladder of the baseline's arch, most capable class first
def private fat_ladder() : array<string> {
    let arch = jit_cpu_class_row(jit_baseline_class()).arch
    return <- [for (c in split(jit_cpu_class_ladder(arch), ";")); c; where !empty(c)]
}

// the sidecar's "kernels" section as a flat { function name : perm suffix } map;
// found = the file existed, parsed to an object, and is NOT stale vs the running binary
// (a "kernels"-less but fresh sidecar counts as found-empty: runtime-knob-only files)
def private read_manifest(path : string; var found : bool&) : table<string; string> {
    found = false
    var tab : table<string; string>
    if (empty(path) || tune_sidecar_stale(path)) {
        return <- tab
    }
    let text = fread(path)
    if (empty(text)) {
        return <- tab
    }
    var err = ""
    var doc = read_json(text, err)
    if (doc == null || !(doc is _object)) {
        delete_json(doc)
        return <- tab
    }
    found = true
    let kv = doc?["kernels"]
    if (kv != null && kv is _object) {
        for (k, v in keys(kv as _object), values(kv as _object)) {
            if (v != null && v is _string) {
                tab[k] = v as _string   // das strings survive the node delete below (string heap)
            }
        }
    }
    delete_json(doc)
    return <- tab
}

//! The winning perm suffix recorded for `fname`, or "" when the manifest is absent or has no
//! entry — the read-your-own-tune-value API.
def tune_manifest_get(fname : string) : string {
    var found = false
    var tab <- read_manifest(tune_manifest_path(), found)
    let r = tab?[fname] ?? ""
    delete tab
    return r
}

// `onlyClass` names one class to read instead of walking the chain - a fat build's per-class pick
def private locate_profile_doc(defaultsDir, verKey, verVal : string; compileTime : bool; var klass, ppath : string&; onlyClass : string = "") : JsonValue? {
    var chain <- empty(onlyClass) ? tune_class_chain() : [onlyClass]
    var doc : JsonValue?
    for (c in chain) {
        let p = path_join(defaultsDir, "{c}.tune-defaults.json")
        let text = fread(p)
        continue if (empty(text))
        var err = ""
        var d = read_json(text, err)
        if (d == null || !(d.value is _object)) {
            if (!compileTime) {
                print("llvm_tune: defaults profile {p} is not valid JSON ({err}) - skipped\n")
            }
            delete_json(d)
            continue
        }
        if (!empty(verKey) && "{d?["provenance"]?[verKey] ?? ""}" != verVal) {
            if (!compileTime) {
                print("llvm_tune: defaults profile '{c}' predates this build ({verKey} != {verVal}) - skipped\n")
            }
            delete_json(d)
            continue
        }
        klass = c
        ppath = p
        doc = d
        break
    }
    delete chain
    return doc
}

def private no_kernels() : table<string; string> {
    var kernels : table<string; string>
    return <- kernels
}

//! A scope whose version pin cannot resolve is skipped, not an error: its pinned module may not be on the AST yet at this kernel's compile.
def private profile_kernels(var klass, ppath : string&; onlyClass : string = "") : table<string; string> {
    klass = ""
    ppath = ""
    var kernels : table<string; string>
    let prog = get_ptr(compiling_program())
    if (prog == null) {
        return <- kernels
    }
    prog |> for_each_module() $(mod) {
        mod |> for_each_structure() $(st) {
            for (ann in st.annotations) {
                continue if (ann.annotation.name != "tune_scope")
                let dv = find_arg(ann.arguments, "defaults")
                continue if (!(dv is tString) || empty(dv as tString))
                let dir = resolve_decl_path("{st.at.fileInfo.name}", dv as tString)
                var verKey = ""
                var verVal = ""
                let vv = find_arg(ann.arguments, "version_of")
                if (vv is tString) {
                    var keyOverride = ""
                    let kov = find_arg(ann.arguments, "version_key")
                    if (kov is tString) {
                        keyOverride = kov as tString
                    }
                    var verr = ""
                    continue if (!resolve_version_of(prog, vv as tString, keyOverride, verKey, verVal, verr))
                }
                var k = ""
                var p = ""
                pin_profile_chain(dir)
                var doc = locate_profile_doc(dir, verKey, verVal, true, k, p, onlyClass)
                continue if (doc == null)
                let ks = doc?["kernels"]
                if (ks != null && ks.value is _object) {
                    for (kk, v in keys(ks as _object), values(ks as _object)) {
                        if (v != null && v is _string && !(kernels |> key_exists(kk))) {
                            kernels[kk] = v as _string
                        }
                    }
                }
                delete_json(doc)
                klass = empty(klass) ? k : "{klass};{k}"
                ppath = empty(ppath) ? p : "{ppath};{p}"
            }
        }
    }
    return <- kernels
}

//! Compile time only — a `[tune]`-family macro's read: the perm `fname` stamps on this box, the
//! sidecar entry first, else this box's class entry in the shipped defaults profile its scope
//! declares. `source` is "" (no pick), the sidecar path, or the profile path.
def tune_kernel_pick(fname : string; var source : string&) : string {
    var found = false
    let mpath = tune_manifest_path()
    pin_module_cache_dependency(mpath)
    var tab <- read_manifest(mpath, found)
    var r = tab?[fname] ?? ""
    delete tab
    source = empty(r) ? "" : mpath
    if (empty(r)) {
        var klass = ""
        var ppath = ""
        var prof <- profile_kernels(klass, ppath)
        r = prof?[fname] ?? ""
        delete prof
        source = empty(r) ? "" : ppath
    }
    return r
}

// the sidecar document for read-modify-write: a new empty object when absent, unparsable,
// STALE, or EXCHANGE-ADOPTED — a local tuner writing over an adopted file must not blend its
// measured keys with the donor's (that yields a complete-looking, wrongly re-submittable mix).
def private load_sidecar_doc(path : string) : JsonValue? {
    var doc : JsonValue?
    if (!tune_sidecar_stale(path)) {
        let text = fread(path)
        if (!empty(text)) {
            var err = ""
            doc = read_json(text, err)
            if (doc != null && doc.value is _object
                && (doc?["provenance"]?["origin"] ?? "") == "exchange") {
                delete_json(doc)
                doc = null
            }
        }
    }
    if (doc == null || !(doc.value is _object)) {
        var fresh : table<string; JsonValue?>
        doc = JV(fresh)
    }
    return doc
}

// refresh "provenance" (base identity + tune_provenance_note entries), PRESERVING other
// processes' note entries — a save rebuilt from its own notes alone drops every other scope's
// version stamp, and two pinned scopes then invalidate each other (the auto policy loops)
def private save_sidecar_doc(path : string; var doc : JsonValue?) : bool {
    var prov = JV((binary = tilde_home(running_binary()), platform = get_platform_name(),
        arch = get_architecture_name(), box = tune_box_identity(), written = iso8601_now(),
        features = tune_feature_fingerprint()))
    let old = doc?["provenance"]
    if (old != null && old is _object) {
        for (k, v in keys(old as _object), values(old as _object)) {
            continue if (k == "binary" || k == "platform" || k == "arch" || k == "box" || k == "written"
                || k == "features" || g_prov_notes |> key_exists(k))
            if (v != null && v is _string) {
                update(prov, k, JV(v as _string))
            } elif (v != null && v is _number) {
                update(prov, k, JV(v as _number))
            } elif (v != null && v is _longint) {
                update(prov, k, JV(v as _longint))
            }
        }
    }
    for (k, v in keys(g_prov_notes), values(g_prov_notes)) {
        if (k != "binary" && k != "platform" && k != "arch" && k != "box") {
            update(prov, k, JV(v))
        }
    }
    update(doc, "provenance", prov)
    let ok = fwrite(path, write_json(doc))
    if (ok) {
        // mtime is 1s-granular: a same-second re-read must not reuse a PRE-write box observation
        var sfs : FStat
        if (stat(path, sfs)) {
            g_box_cache["{path}@{sfs.mtime}"] = tune_box_identity()
        }
    }
    return ok
}

var private g_prov_notes : table<string; string>

    //! Attach `key = value` to the `"provenance"` section of every subsequent sidecar save in
    //! this process — how a tuner records its measurement conditions (noise verdict, mode)
    //! next to the winners they produced. Same key overwrites; base identity keys always win.
def tune_provenance_note(key : string; value : string) {
    g_prov_notes[key] = value
}

// upsert one "kernels" entry into an already-loaded document
def private doc_kernels_set(var doc : JsonValue?; fname : string; suffix : string) {
    var kernels = doc?["kernels"]
    if (kernels == null || !(kernels.value is _object)) {
        update(doc, "kernels", JV({ fname => JV(suffix) }))   // frees a non-object leftover
        return
    }
    // kern aliases the pointee behind the kernels JsonValue? — stable across doc rehashes
    var kern & = unsafe(kernels.value as _object)
    if (kern |> key_exists(fname)) {
        delete_json(kern[fname])
    }
    kern[fname] = JV(suffix)
}

    //! Upsert `fname -> suffix` into the sidecar's `"kernels"` section and save — what a harness
    //! calls after picking a winner. Other sections and other scopes' keys survive, which is what
    //! lets several libraries share one file. A stale sidecar resets first; false = write failed.
def tune_manifest_set(fname : string; suffix : string) : bool {
    let path = tune_manifest_path()
    if (empty(path)) {
        return false
    }
    var doc = load_sidecar_doc(path)
    doc_kernels_set(doc, fname, suffix)
    let ok = save_sidecar_doc(path, doc)
    delete_json(doc)
    return ok
}

    //! The bulk form: upsert every `kernels` entry, and when `runtime` is non-null REPLACE the
    //! sidecar's `"runtime"` section with it (the document ADOPTS that tree — do not reuse it).
    //! Same preservation and stale-reset semantics as `tune_manifest_set`.
def tune_sidecar_merge(kernels : table<string; string>; var runtime : JsonValue?) : bool {
    let path = tune_manifest_path()
    if (empty(path)) {
        return false
    }
    var doc = load_sidecar_doc(path)
    for (fname, suffix in keys(kernels), values(kernels)) {
        doc_kernels_set(doc, fname, suffix)
    }
    if (runtime != null) {
        update(doc, "runtime", runtime)   // frees the previous section; doc adopts the new one
    }
    let ok = save_sidecar_doc(path, doc)
    delete_json(doc)
    return ok
}

    //! Upsert `entries` into sidecar section `section` (created when absent; same key
    //! overwrites, other keys/sections survive; the document ADOPTS the entries). How a tuner
    //! records per-kernel race tables — "how much did it win by" must be answerable from the file.
def tune_sidecar_section_merge(section : string; var entries : table<string; JsonValue?>) : bool {
    let path = tune_manifest_path()
    if (empty(path)) {
        return false
    }
    var doc = load_sidecar_doc(path)
    var sec = doc?[section]
    if (sec == null || !(sec.value is _object)) {
        update(doc, section, JV(entries))   // adopts entries; frees a non-object leftover
    } else {
        var so & = unsafe(sec.value as _object)
        for (k, v in keys(entries), values(entries)) {
            if (so |> key_exists(k)) {
                delete_json(so[k])
            }
            so[k] = v   // the document adopts the entry
        }
        entries |> clear()   // adopted — the caller's table no longer owns them
    }
    let ok = save_sidecar_doc(path, doc)
    delete_json(doc)
    return ok
}

    //! Export this app's minted winners as the shipped defaults profile for CPU class `klass`:
    //! the sidecar's `"kernels"` plus origin/class/features provenance — race tables, runtime
    //! knobs and box identity stay behind. Check the file in under `[tune_scope(defaults=...)]`.
def tune_profile_export(outPath : string; klass : string) : bool {
    var doc = load_sidecar_doc(tune_manifest_path())
    let ks = doc?["kernels"]
    if (ks == null || !(ks.value is _object) || empty(ks as _object)) {
        print("llvm_tune: profile export refused - no kernels in {tune_manifest_path()} (mint first, and check the sidecar is not stale)\n")
        delete_json(doc)
        return false
    }
    var kernels <- {for (k, v in keys(ks as _object), values(ks as _object)); k => JV(v as _string); where v != null && v is _string}
    var prov : table<string; JsonValue?>
    let old = doc?["provenance"]
    if (old != null && old.value is _object) {
        for (k, v in keys(old as _object), values(old as _object)) {
            // a profile's validity is its version pin + feature class, never a commit: engine_sha stays behind with the box identity
            continue if (k == "binary" || k == "box" || k == "written" || k == "origin" || k == "class" || k == "engine_sha")
            if (v != null && v is _string) {
                prov[k] = JV(v as _string)
            }
        }
    }
    prov["origin"] = JV("profile")
    prov["class"] = JV(klass)
    prov["written"] = JV(iso8601_now())
    var out = JV({ "kernels" => JV(kernels), "provenance" => JV(prov) })
    let ok = fwrite(outPath, write_json(out))
    print(ok ? "llvm_tune: profile '{klass}' exported -> {outPath} ({length(ks as _object)} kernel entries)\n"
             : "llvm_tune: profile export FAILED writing {outPath}\n")
    delete_json(out)
    delete_json(doc)
    return ok
}

// ===== the tune history (~/.tune-history) =====

var private g_history_dir = ""

    //! Point the history at an explicit directory in-process (the env loads once at context
    //! init); arms `DAS_TUNE_HISTORY` for tuner children too.
def tune_set_history_dir(d : string) {
    g_history_dir = d
    set_env_variable("DAS_TUNE_HISTORY", d)
}

    //! This box's tune-history directory: `DAS_TUNE_HISTORY` env, else
    //! `<home>/.tune-history/<sanitized box identity>/`. Every mint archives here — sidecars
    //! are small and the history is the box's longitudinal health record; NEVER deleted.
def tune_history_dir() : string {
    if (!empty(g_history_dir)) {
        return g_history_dir
    }
    let env = g_env_tune.tune_history
    if (!empty(env)) {
        g_history_dir = env
        return g_history_dir
    }
    var home = env_value_of("HOME")
    if (empty(home)) {
        home = env_value_of("USERPROFILE")
    }
    var san = ""
    peek_data(tune_box_identity()) $(bytes) {
        san = build_string() $(w) {
            for (c in bytes) {
                let ch = int(c)
                let an = is_number(ch) || is_alpha(ch)
                w |> write(an ? to_char(ch) : "-")
            }
        }
    }
    g_history_dir = "{home}/.tune-history/{san}"
    return g_history_dir
}

    //! Archive the sidecar at `path` into the tune history: `verdict=="ok"` copies the file
    //! (self-contained — provenance + race tables ride inside), anything else leaves a marked
    //! `.FAILED.json` stub carrying `verdict`/`note`. Best-effort — never fails the mint.
def tune_history_archive(path : string; verdict : string; note : string) : string {
    let dir = tune_history_dir()
    var home = env_value_of("HOME")
    if (empty(home)) {
        home = env_value_of("USERPROFILE")
    }
    mkdir("{home}/.tune-history")   // best-effort parents; mkdir on an existing dir is a no-op failure
    mkdir(dir)
    let stamp = format_time(get_clock(), "%Y%m%d-%H%M%S")
    var stem = base_name(path)
    if (ends_with(stem, ".tune.json")) {
        stem = slice(stem, 0, length(stem) - length(".tune.json"))
    }
    var dest = ""
    var ok = false
    if (verdict == "ok") {
        dest = "{dir}/{stamp}_{stem}.tune.json"
        let text = fread(path)
        ok = !empty(text) && fwrite(dest, text)
    } else {
        dest = "{dir}/{stamp}_{stem}.FAILED.json"
        ok = fwrite(dest, "\{\"verdict\": \"{verdict}\", \"note\": \"{note}\", \"box\": \"{tune_box_identity()}\", \"written\": \"{iso8601_now()}\"\}")
    }
    if (!ok) {
        to_log(LOG_WARNING, "tune history: could not archive to {dest}\n")
        return ""
    }
    return dest
}

// ===== measurement statistics + the noise floor =====

//! Median of `samples` — the robust central estimate a tuner ranks finalists by (a min over
//! rounds is a lottery ticket on a non-silent box; the median is what the box actually does).
def tune_median(samples : array<double>) : double {
    if (empty(samples)) {
        return 0.0lf
    }
    var s := samples
    sort(s)
    let n = length(s)
    let m = (n & 1) == 1 ? s[n / 2] : 0.5lf * (s[n / 2 - 1] + s[n / 2])
    delete s
    return m
}

//! Coefficient of variation of `samples` in percent (sample sd over mean) — the tuner's noise
//! measure. 0 when fewer than two samples or the mean is not positive.
def tune_cv_pct(samples : array<double>) : double {
    let n = length(samples)
    if (n < 2) {
        return 0.0lf
    }
    var mean = 0.0lf
    for (s in samples) {
        mean += s
    }
    mean /= double(n)
    if (mean <= 0.0lf) {
        return 0.0lf
    }
    var dev2 = 0.0lf
    for (s in samples) {
        dev2 += (s - mean) * (s - mean)
    }
    return 100.0lf * sqrt(dev2 / double(n - 1)) / mean
}

//! One validation window's outcome for a recorded winner: what a self-validation re-race saw.
//! A harness fills one row per re-race window and renders the verdict across all of them
//! with `tune_validate_verdict`.
struct TuneValidateRow {
    challenger : string   //!< re-race winner when it beat the recorded one past the band, else ""
    gap : double          //!< recorded-winner vs re-race-winner gap, pct, same window
    drift : double        //!< recorded winner's re-race median vs its recorded race median, pct
    rejected : bool       //!< recorded winner failed correctness (or vanished) in the re-race
}

    //! Render a recorded winner's reproduction verdict from its validation windows: "" =
    //! reproduced. Rejection anywhere is fatal; a loss is real only when the SAME challenger
    //! took ≥2 windows — anything else is state scramble among twins; levels never enter.
def tune_validate_verdict(rows : array<TuneValidateRow>) : string {
    var lossname = ""
    var lossAll = true
    for (r in rows) {
        return "recorded winner rejected in a re-race" if (r.rejected)
        if (empty(r.challenger)) {
            lossAll = false
        } elif (empty(lossname)) {
            lossname = r.challenger
        } elif (lossname != r.challenger) {
            lossAll = false
        }
    }
    if (lossAll && !empty(lossname) && length(rows) >= 2) {
        let gtxt = join([for (r in rows); "{r.gap:0.2f}%"], " / ")
        return "consistently loses to {lossname} in both re-races ({gtxt})"
    }
    return ""
}

var private g_noise_thr = -1.0lf
var private g_noise_ovr = -1

    //! The probe-cv note threshold (percent): above it a tuner stamps the mint `noise: noisy`
    //! and races on margins; refusal is the harness's hard ceiling, far above. `DAS_TUNE_NOISE_CV`
    //! overrides (calibration; garbage ignored), else 2%; `paranoid` is ignored.
def tune_noise_threshold_pct(_paranoid : bool) : double {
    if (g_noise_thr < 0.0lf) {
        let v = g_env_tune.tune_noise_cv
        g_noise_thr = v > 0.0f ? double(v) : 0.0lf
    }
    return g_noise_thr > 0.0lf ? g_noise_thr : 2.0lf
}

    //! Recalibrate the noise gate in-process (the env loads once at context init, so a
    //! harness sets this instead of `set_env_variable`); the env is armed for tuner children.
def tune_set_noise_cv(pct : float) {
    g_noise_thr = pct > 0.0f ? double(pct) : 0.0lf
    set_env_variable("DAS_TUNE_NOISE_CV", "{pct}")
}

    //! True when `DAS_TUNE_NOISE_CV` recalibrated the gate — the announce hook for the
    //! overrides-are-visible rule (a threshold line a recalibration shaped must say so).
def tune_noise_threshold_overridden() : bool {
    tune_noise_threshold_pct(false)   // force the lazy seed; the paranoid arg does not affect the override bit
    return g_noise_thr > 0.0lf
}

    //! True when `DAS_TUNE_NOISE_OVERRIDE` asks to mint through the busy-box refusal anyway.
    //! The tuner that honors it must stamp the sidecar `noise: overridden` — the escape leaves
    //! a mark, so a later regression hunt can see the mint was taken on a loud box.
def tune_noise_override() : bool {
    if (g_noise_ovr < 0) {
        g_noise_ovr = g_env_tune.tune_noise_override ? 1 : 0
    }
    return g_noise_ovr == 1
}

    //! In-process twin of `DAS_TUNE_NOISE_OVERRIDE` (load-once env); arms children too.
def tune_set_noise_override(b : bool) {
    g_noise_ovr = b ? 1 : 0
    set_env_variable("DAS_TUNE_NOISE_OVERRIDE", b ? "1" : "0")
}

// ===== tune status (runtime) =====

//! One tuned function's final compile-time decision, as reported by `tune_status()`.
struct TuneStatus {
    fname : string
    scope : string          // [tune_scope] name, "" when env/app-manifest driven
    suffix : string         // winning perm suffix, "" for the reference body
    source : string         // "manifest" | "profile" | "fallback" | "reference"
    manifestPath : string   // the manifest the winner came from (or would come from)
    klass : string          // a fat build's clone class; "" for a single-stamp build
}

var private g_tune_status : array<TuneStatus>

//! Hook the `[tune_policy]`-injected [init] calls once per tuned function.
def tune_status_add(fname, scope, suffix, source, manifestPath : string; klass : string = "") {
    g_tune_status |> push(TuneStatus(fname = fname, scope = scope, suffix = suffix,
        source = source, manifestPath = manifestPath, klass = klass))
}

// ===== fat mode (runtime) =====

// the interpreter's copies of the two words; under the JIT the [llvm_code] stubs below read and
// write one LLVM-side global each instead (`llvm_tune_fat_word`), which every context of the
// process shares - a pooled fork whose init script never ran has these das globals at zero
var private g_tune_fat_mask = 0
var private g_tune_fat_built = 0

//! The classes this box runs a fat exe's clones for, one bit per class (`tune_fat_class_bit`);
//! 0 until the exe's `[init]` ran, which every dispatch reads as the baseline clone.
[llvm_code(name = "llvm_tune_fat_word::load", global = "das.tune.fat_mask")]
def tune_fat_mask() : int {
    return g_tune_fat_mask
}

[llvm_code(name = "llvm_tune_fat_word::store", global = "das.tune.fat_mask")]
def private tune_fat_mask_set(mask : int) {
    g_tune_fat_mask = mask
}

[llvm_code(name = "llvm_tune_fat_word::load", global = "das.tune.fat_built")]
def private tune_fat_built_word() : int {
    return g_tune_fat_built
}

[llvm_code(name = "llvm_tune_fat_word::store", global = "das.tune.fat_built")]
def private tune_fat_built_set(built : int) {
    g_tune_fat_built = built
}

//! True inside a fat exe once its `[init]` ran - what a library asks before it races the
//! runtime section at first start (a plain build has a policy rail and a tuner for that).
def tune_fat_built() : bool {
    return tune_fat_built_word() != 0
}

//! A class's bit in the fat mask: its position on its arch's ladder, most capable class first.
def tune_fat_class_bit(klass : string) : int {
    let row = jit_cpu_class_row(klass)
    for (i, c in count(), split(jit_cpu_class_ladder(row.arch), ";")) {
        if (c == klass) {
            return 1 << i
        }
    }
    return 0
}

//! True when this box carries every feature of the class - cpuid only, never the build's
//! baseline or a force env, because the question is what the silicon underneath can run.
def tune_fat_box_in_class(klass : string) : bool {
    let row = jit_cpu_class_row(klass)
    return false if (empty(row.name) || row.arch != get_architecture_name())
    for (f in split(row.features, ";")) {
        return false if (!empty(f) && !cpu_supports(f))
    }
    return true
}

//! A fat exe's `[init]`: refuse a box below the baseline, then set the mask to every class of the
//! ladder this box is in - or the one `DAS_TUNE_FAT_CLASS` pins, when the box is in it.
[arch(at = "../ARCHITECTURE_TARGET_FEATURES.md#fat-mode")]
def tune_fat_init(baseline : string) {
    let brow = jit_cpu_class_row(baseline)
    if (!tune_fat_box_in_class(baseline)) {
        panic("this program was built for the {baseline} CPU class ({brow.features}) and this {get_architecture_name()} box does not carry all of it")
    }
    let pin = strip(g_env_tune.tune_fat_class)
    if (!empty(pin) && !tune_fat_box_in_class(pin)) {
        panic("DAS_TUNE_FAT_CLASS={pin} pins a CPU class this box is not in ({jit_cpu_class_row(pin).features})")
    }
    if (!empty(pin)) {
        // the status rows precede this call: a pin names a class some clone carries, or it dispatches nothing
        var carried : array<string>
        var found = false
        for (s in g_tune_status) {
            continue if (empty(s.klass))
            found ||= s.klass == pin
            if (find_index(carried, s.klass) < 0) {
                carried |> push(s.klass)
            }
        }
        if (!found && !empty(carried)) {
            panic("DAS_TUNE_FAT_CLASS={pin} pins a CPU class this exe carries no clone for (it carries: {join(carried, ", ")})")
        }
        delete carried
    }
    var mask = 0
    var names : array<string>
    for (c in split(jit_cpu_class_ladder(brow.arch), ";")) {
        continue if (empty(c))
        if (empty(pin) ? tune_fat_box_in_class(c) : c == pin) {
            mask |= tune_fat_class_bit(c)
            names |> push(c)
        }
    }
    tune_fat_mask_set(mask)
    tune_fat_built_set(1)
    let pinned = empty(pin) ? "" : " (DAS_TUNE_FAT_CLASS={pin} pins the class)"
    to_log(LOG_INFO, "llvm_tune: fat kernels - this box runs the {join(names, ", ")} clones{pinned}\n")
    delete names
}

//! The program's tune decisions — one row per `[tune]` function. Populated whenever the
//! policy rail ran (a declared `[tune_policy]` or the auto default on any app with tune
//! scopes); empty under the gates (AOT, `-exe`, grid modes, `DAS_TUNE_MANIFEST`).
def tune_status() : array<TuneStatus> {
    return clone_to_move(g_tune_status)
}


// ===== [tune_scope] data — library-owned tuner wiring =====

// one registered scope: the harness that regenerates this library's kernels, plus the
// sidecar it writes into (the per-app file, resolved at scope-apply time)
struct private TuneScope {
    scopeName : string
    modName : string     // the declaring module — anchors the demanded-kernel AST walk
    covers : string      // ';'-joined extra modules whose [tune]/[tuned] kernels belong to this scope
    manifestPath : string
    tunerPath : string   // resolved against the declaring file; "" = no tuner declared
    versionKey : string      // provenance key the scope pins ("" = unpinned)
    versionExpected : string
    defaultsPath : string    // resolved against the declaring file; "" = no shipped profiles
}

// `version_of = "module/CONST"` on a [tune_scope]: winners are valid only while the sidecar's
// provenance records CONST's current (folded) value — the library bumps the const on kernel work.
// Provenance key defaults to the lowercased const name (`version_key=` overrides); false = err says why.
def private resolve_version_of(prog : Program?; spec, keyOverride : string;
                               var versionKey, versionExpected : string&; var err : string&) : bool {
    let slash = find(spec, "/")
    if (slash <= 0 || slash >= length(spec) - 1) {
        err = "version_of=\"module/CONST\" - got \"{spec}\""
        return false
    }
    let modName = slice(spec, 0, slash)
    let constName = slice(spec, slash + 1)
    var value = ""
    var modFound = false
    prog |> for_each_module() $(mod) {
        if ("{mod.name}" == modName) {
            modFound = true
            mod |> for_each_global() $(gv) {
                if ("{gv.name}" == constName) {
                    let ci = gv.init ?as ExprConstInt
                    if (ci != null) {
                        value = "{ci.value}"
                    }
                }
            }
        }
    }
    if (!modFound) {
        err = "version_of module '{modName}' is not in the compiling program"
        return false
    }
    if (empty(value)) {
        err = "version_of '{spec}' must name an int constant with a foldable initializer"
        return false
    }
    versionKey = empty(keyOverride) ? to_lower(constName) : keyOverride
    versionExpected = value
    return true
}

// declaring module name -> scope. [tune_policy] drives tuner runs off this bank; the [tune]
// read path itself is scope-independent (every module reads the app sidecar).
var private g_scopes : table<string; TuneScope>

    // every [tune] function plus every non-perm-pinned [tuned] one, read OFF THE AST: a macro
    // global would be a per-requiring-context copy and read empty across modules.
def private module_kernel_fnames(mod : Module?; var fnames : array<string>) {
    for_each_function(mod, "") $(fn) {
        var demand = false
        for (ann in fn.annotations) {
            if (ann.annotation.name == "tune") {
                demand = true
            } elif (ann.annotation.name == "tuned") {
                let pv = find_arg(ann.arguments, "perm")
                demand = !(pv is tString)
            }
        }
        if (demand) {
            fnames |> push("{fn.name}")
        }
    }
}

// the ';'-joined demanded-kernel list of a scope: its declaring module plus every covers=
// module present in the compiling program — the literal the policy guard injection carries
// into runtime, where the AST no longer exists
def private scope_fnames_joined(scope : TuneScope) : string {
    var wanted : table<string>
    wanted |> insert(scope.modName)
    for (m in split(scope.covers, ";")) {
        if (!empty(m)) {
            wanted |> insert(m)
        }
    }
    var fnames : array<string>
    program_for_each_module(compiling_program()) $(mod) {
        if (wanted |> key_exists("{mod.name}")) {
            module_kernel_fnames(mod, fnames)
        }
    }
    if (!empty(scope.versionKey)) {
        fnames |> push("{TUNE_VERSION_TOKEN}{scope.versionKey}={scope.versionExpected}")
    }
    let joined = join(fnames, ";")
    delete wanted
    delete fnames
    return joined
}

// every `requires=` a scope's [tune_perm] rows carry, read off the AST for the runtime guard
def private scope_gated_families_joined(scope : TuneScope) : string {
    var wanted : table<string>
    wanted |> insert(scope.modName)
    for (m in split(scope.covers, ";")) {
        if (!empty(m)) {
            wanted |> insert(m)
        }
    }
    var rows : array<string>
    program_for_each_module(compiling_program()) $(mod) {
        if (wanted |> key_exists("{mod.name}")) {
            for_each_function(mod, "") $(fn) {
                var isTune = false
                var exprs : array<string>
                for (ann in fn.annotations) {
                    if (ann.annotation.name == "tune") {
                        isTune = true
                    } elif (ann.annotation.name == "tune_perm") {
                        let rv = find_arg(ann.arguments, "requires")
                        if (rv is tString && !empty(rv as tString)) {
                            exprs |> push(rv as tString)
                        }
                    }
                }
                if (isTune && !empty(exprs)) {
                    rows |> push("{fn.name}={join(exprs, "+")}")
                }
                delete exprs
            }
        }
    }
    let joined = join(rows, ";")
    delete wanted
    delete rows
    return joined
}

//! One `[tune_scope]`'s identity + completeness, as reported by `tune_scopes_status()` —
//! the macro-time surface build orchestrators (the `-exe` release-deps JSON, daspkg) read
//! to decide whether to run tuners before a rebuild.
struct TuneScopeStatus {
    name : string       //!< [tune_scope] name
    manifest : string   //!< the app sidecar the winners live in
    tuner : string      //!< tuner harness path ("" = none declared)
    complete : bool     //!< every demanded kernel has a fresh sidecar entry and the version pin (if any) matched
    fallbacks : string  //!< ';'-joined fname=suffix stamps selected by this compile/host
    fat_unprofiled : string   //!< fat builds: ';'-joined [tune] kernels whose BASELINE clone came from no profile (fallback or reference) - what a fat release refuses to ship
}

    //! Every declared scope in `prog` with its per-key completeness against the app sidecar,
    //! read OFF THE AST, not this module's macro globals — those are per-requiring-context
    //! copies, so a caller in another macro module reads an empty bank. Deduped by name.
def tune_scopes_status(prog : Program?) : array<TuneScopeStatus> {
    var found <- declared_scopes(prog)
    // pass 2: each scope's demanded kernels across its declaring + covers= modules
    var rows : array<TuneScopeStatus>
    rows |> reserve(length(found))
    let mpath = tune_manifest_path()
    var compileStatus <- collect_status(prog)
    for (sc in found) {
        rows |> push(scope_status_row(prog, sc, mpath, compileStatus))
    }
    delete compileStatus
    delete found
    return <- rows
}

// pass 1: the declared scopes (name, tuner, covers, version pin) off the [tune_scope] annotations, deduped by name
def private declared_scopes(prog : Program?) : array<TuneScope> {
    var found : array<TuneScope>
    var seen : table<string>
    prog |> for_each_module() $(mod) {
        mod |> for_each_structure() $(st) {
            for (ann in st.annotations) {
                if (ann.annotation.name == "tune_scope") {
                    var sc = TuneScope(modName = "{mod.name}")
                    let nv = find_arg(ann.arguments, "name")
                    if (nv is tString) { sc.scopeName = nv as tString }
                    let tv = find_arg(ann.arguments, "tuner")
                    if (tv is tString) {
                        sc.tunerPath = resolve_decl_path("{st.at.fileInfo.name}", tv as tString)
                    }
                    let cv = find_arg(ann.arguments, "covers")
                    if (cv is tString) { sc.covers = cv as tString }
                    let vv = find_arg(ann.arguments, "version_of")
                    if (vv is tString) {
                        var keyOverride = ""
                        let kov = find_arg(ann.arguments, "version_key")
                        if (kov is tString) { keyOverride = kov as tString }
                        var verr = ""
                        // a bad spec already failed the library compile; a miss here just unpins
                        resolve_version_of(prog, vv as tString, keyOverride,
                            sc.versionKey, sc.versionExpected, verr)
                    }
                    if (!empty(sc.scopeName) && !(seen |> key_exists(sc.scopeName))) {
                        seen |> insert(sc.scopeName)
                        found |> push(sc)
                    }
                }
            }
        }
    }
    delete seen
    return <- found
}

// one scope's status row: its demanded kernels, their fallback stamps, and under fat the
// baseline clones that came from no profile
def private scope_status_row(prog : Program?; sc : TuneScope; mpath : string; compileStatus : array<TuneStatus>) : TuneScopeStatus {
    var wanted : table<string>
    wanted |> insert(sc.modName)
    for (m in split(sc.covers, ";")) {
        if (!empty(m)) {
            wanted |> insert(m)
        }
    }
    var fnames : array<string>
    prog |> for_each_module() $(mod) {
        if (wanted |> key_exists("{mod.name}")) {
            module_kernel_fnames(mod, fnames)
        }
    }
    if (!empty(sc.versionKey)) {
        fnames |> push("{TUNE_VERSION_TOKEN}{sc.versionKey}={sc.versionExpected}")
    }
    var fallbacks : array<string>
    var unprofiled : array<string>
    let fat = tune_mode() == "fat"
    let baseline = jit_baseline_class()
    for (fname in fnames) {
        for (st in compileStatus) {
            if (st.fname == fname && (fat ? st.klass == baseline : true)) {
                if (st.source != "manifest") {
                    fallbacks |> push("{fname}={st.suffix}")
                }
                if (fat && st.source != "profile") {
                    unprofiled |> push(fname)
                }
                break
            }
        }
    }
    let row = TuneScopeStatus(name = sc.scopeName, manifest = mpath,
        tuner = sc.tunerPath,
        complete = tune_sidecar_entries_complete(mpath, join(fnames, ";")),
        fallbacks = join(fallbacks, ";"), fat_unprofiled = join(unprofiled, ";"))
    delete wanted
    delete fnames
    delete fallbacks
    delete unprofiled
    return row
}

// A scope's version pin travels INSIDE the ';'-joined demand list as one `@version:key=value`
// token — every carrier of an fnames string (the guards, the scope-resolver contract, daspkg's
// status walk) then enforces it with no signature change. '@' can never begin a function name.
let TUNE_VERSION_TOKEN = "@version:"

// a malformed token never blocks — the [tune_scope] apply rejects those at declaration
def private version_token_satisfied(path : string; tok : string) : bool {
    let kv = slice(tok, length(TUNE_VERSION_TOKEN))
    let eq = find(kv, "=")
    return true if (eq < 0)
    return sidecar_prov_value(path, slice(kv, 0, eq)) == slice(kv, eq + 1)
}

def private entry_satisfied(path : string; manifest : table<string; string>; fname : string) : bool {
    if (fname |> starts_with(TUNE_VERSION_TOKEN)) {
        return version_token_satisfied(path, fname)
    }
    return manifest |> key_exists(fname)
}

//! The demand entries in the ';'-joined `fnames` lacking a sidecar entry — kernel names, plus
//! an unsatisfied `@version:` pin token verbatim; "" when complete. The bare-list sibling of
//! `tune_sidecar_verdict` for build orchestrators; the refusal messages print the verdict.
def tune_sidecar_missing_entries(path : string; fnames : string) : string {
    var found = false
    var manifest <- read_manifest(path, found)
    var missing <- [for (fname in split(fnames, ";"));
        fname;
        where !empty(fname) && !(found && entry_satisfied(path, manifest, fname))]
    delete manifest
    let r = join(missing, ";")
    delete missing
    return r
}

    //! Every kernel in the ';'-joined `fnames` has a `"kernels"` entry in the sidecar at `path`
    //! (companions ride their main entry; "reference" counts; a `@version:` token checks its
    //! provenance pin). Missing/stale/unparsable means no entries — file existence proves nothing.
def tune_sidecar_entries_complete(path : string; fnames : string) : bool {
    var found = false
    var manifest <- read_manifest(path, found)
    var complete = found || empty(fnames)
    if (found) {
        for (fname in split(fnames, ";")) {
            if (!empty(fname)) {
                complete &&= entry_satisfied(path, manifest, fname)
            }
        }
    }
    delete manifest
    return complete
}

//! The closed reason set `tune_sidecar_verdict` reports; `none` accompanies `complete`.
enum TuneSidecarReason {
    none
    absent
    stale_binary
    foreign_box
    unreadable
    version
    missing
}

//! Why the sidecar at `path` does not satisfy the ';'-joined `fnames` demand: `complete` true
//! means it does; otherwise `reason` says why and `detail` is the printable sentence the
//! refusal lines carry.
struct TuneSidecarVerdict {
    complete : bool
    reason : TuneSidecarReason
    detail : string
}

    //! The named-reason form of `tune_sidecar_entries_complete` — same verdict plus WHY, so an
    //! "untuned" refusal can say stale vs foreign vs missing instead of leaving the operator to
    //! diff provenance by hand. Check order mirrors `tune_sidecar_stale`: mtime, then box.
def tune_sidecar_verdict(path : string; fnames : string) : TuneSidecarVerdict {
    if (empty(fnames)) {
        return TuneSidecarVerdict(complete = true)
    }
    var sfs : FStat
    if (empty(path) || !stat(path, sfs)) {
        return TuneSidecarVerdict(reason = TuneSidecarReason.absent, detail = "no sidecar at '{path}'")
    }
    var bfs : FStat
    if (stat(running_binary(), bfs) && sfs.mtime < bfs.mtime) {
        return TuneSidecarVerdict(reason = TuneSidecarReason.stale_binary,
            detail = "sidecar predates the running binary ({sfs.mtime} < {bfs.mtime}); a rebuild invalidates measured winners")
    }
    let theirs = sidecar_box(path, "{sfs.mtime}")
    if (empty(theirs)) {
        // unparsable json and provenance-less files land here — not a tune sidecar at all
        return TuneSidecarVerdict(reason = TuneSidecarReason.unreadable,
            detail = "sidecar does not read as a tune sidecar (no readable provenance)")
    }
    if (box_match_key(theirs) != box_match_key(tune_box_identity())) {
        return TuneSidecarVerdict(reason = TuneSidecarReason.foreign_box,
            detail = "sidecar was minted on '{theirs}', this box is '{tune_box_identity()}'")
    }
    var found = false
    var inscope manifest <- read_manifest(path, found)
    if (!found) {
        // the race twin of the arm above: the file changed between the two reads
        return TuneSidecarVerdict(reason = TuneSidecarReason.unreadable,
            detail = "sidecar does not read as a tune sidecar (no readable provenance)")
    }
    var inscope missing : array<string>
    for (fname in split(fnames, ";")) {
        continue if (empty(fname))
        if (fname |> starts_with(TUNE_VERSION_TOKEN)) {
            if (!version_token_satisfied(path, fname)) {
                let kv = slice(fname, length(TUNE_VERSION_TOKEN))
                let eq = find(kv, "=")
                let key = slice(kv, 0, eq)
                let got = sidecar_prov_value(path, key)
                return TuneSidecarVerdict(reason = TuneSidecarReason.version, detail = (empty(got)
                    ? "sidecar records no {key} - minted before this pin; kernels have changed since"
                    : "sidecar was minted at {key} {got}, the code is at {slice(kv, eq + 1)}; kernel work invalidates winners"))
            }
        } elif (!(manifest |> key_exists(fname))) {
            missing |> push(fname)
        }
    }
    if (!empty(missing)) {
        let names = join(missing, ";")
        return TuneSidecarVerdict(reason = TuneSidecarReason.missing, detail = "missing entries: {names}")
    }
    return TuneSidecarVerdict(complete = true)
}

// ===== the perm grid =====

struct private TunePerm {
    suffix : string
    requires : string   // fallback-eligibility hint: "a,b" = AND, "a|b" = OR per element; "" = everywhere
    args : array<tuple<argname : string; argvalue : RttiValue>>
}

    // The baseline class's set (DAS_JIT_BASELINE) or else host cpuid, OR'd with the force env —
    // in lockstep with llvm_jit_common's x64_target_feature. requires= is an ELIGIBILITY hint for
    // fallback selection only; the generator's own decline predicate stays the emission truth.
def private fallback_feature_ok(name : string) : bool {
    let baseline = jit_baseline_class()
    if (!empty(baseline)) {
        if (jit_class_has(baseline, name)) return true
    } elif (cpu_supports(name)) {
        return true
    }
    for (part in split(g_env_jit.jit_x64_force_features, ",")) {
        if (part == name) return true
    }
    for (part in split(g_env_jit.jit_arm64_force_features, ",")) {
        if (part == name) return true
    }
    return false
}

//! True when a `requires=` feature expression passes on this box — a `,`-separated AND of
//! `|`-separated alternatives, matched against host CPU features plus the
//! `DAS_JIT_*_FORCE_FEATURES` overrides. An empty expression always passes.
def tune_requires_ok(expr : string) : bool {
    if (empty(expr)) return true
    for (conj in split(expr, ",")) {
        continue if (empty(conj))
        var anyOk = false
        for (alt in split(conj, "|")) {
            anyOk ||= !empty(alt) && fallback_feature_ok(alt)
        }
        if (!anyOk) return false
    }
    return true
}

//! Pick from a `;`-chain of `suffix` or `suffix:requires` entries — the first whose requires
//! passes on this box, so one annotation can ship a different default per ISA. "" when none
//! qualify. A chain with no `:` is a plain ordered list, and a single name is just that name.
def tune_pick_fallback(chain : string) : string {
    for (entry in split(chain, ";")) {
        continue if (empty(entry))
        let colon = find(entry, ":")
        if (colon < 0) return entry
        if (tune_requires_ok(slice(entry, colon + 1))) {
            return slice(entry, 0, colon)
        }
    }
    return ""
}

let private TUNE_KNOWN_FEATURES = "avx2;f16c;fma;sse4.2;avxvnni;avxvnniint8;avx512f;avx512bw;avx512vl;avx512vnni;avx512vbmi;amx-tile;amx-int8;neon;dotprod;i8mm;bf16;fullfp16;sve;lse"

    //! The ';'-joined subset of the known `requires=` feature names this box passes — stamped
    //! into every sidecar's provenance as `features`, which is what lets a shipped profile say
    //! exactly which gated seats its minting box could and could not race.
[arch(at = "../ARCHITECTURE_TARGET_FEATURES.md#x64-tier-gates")]
def tune_feature_fingerprint() : string {
    var have <- [for (f in split(TUNE_KNOWN_FEATURES, ";")); f; where fallback_feature_ok(f)]
    let joined = join(have, ";")
    delete have
    return joined
}

    //! This box's CPU feature class — the name a shipped defaults profile is keyed by. Classes
    //! follow the seats `requires=` can gate, not box models: two boxes of one class stamp the
    //! same winners, so one profile serves them all; an arch with no ladder gets a key no profile matches.
def tune_cpu_class() : string {
    let baseline = jit_baseline_class()
    if (!empty(baseline)) {
        return baseline
    }
    let arch = get_architecture_name()
    if (arch == "x86_64") {
        if (fallback_feature_ok("avx512vnni") && fallback_feature_ok("avx512bw")) {
            return (fallback_feature_ok("amx-int8") && fallback_feature_ok("amx-tile")) ? "x86-amx" : "x86-vnni512"
        }
        if (fallback_feature_ok("avxvnni")) {
            return "x86-vnni256"
        }
        return fallback_feature_ok("avx2") ? "x86-avx2" : "x86-base"
    }
    if (arch == "arm64" || arch == "aarch64") {
        return fallback_feature_ok("i8mm") ? "arm-i8mm" : "arm-neon"
    }
    return "{get_platform_name()}-{arch}"
}

    //! Profile candidates for this box, most specific first — a box above the best shipped
    //! profile adopts the nearest lower class and races only what that class could not.
def tune_class_chain() : array<string> {
    let ladder = jit_cpu_class_ladder(get_architecture_name())
    let own = tune_cpu_class()
    var chain : array<string>
    var seen = false
    for (c in split(ladder, ";")) {
        seen ||= c == own
        if (seen) {
            chain |> push(c)
        }
    }
    if (!seen) {
        chain |> push(own)
    }
    return <- chain
}

// "did the PROFILE's minting box pass this gate" is what separates covered from unlocked seats
def private requires_ok_in_fingerprint(requires : string; fingerprint : string) : bool {
    return true if (empty(requires))
    var have : table<string>
    for (f in split(fingerprint, ";")) {
        if (!empty(f)) {
            have |> insert(f)
        }
    }
    var ok = true
    for (conj in split(requires, ",")) {
        continue if (empty(conj))
        var anyOk = false
        for (alt in split(conj, "|")) {
            anyOk ||= !empty(alt) && have |> key_exists(alt)
        }
        if (!anyOk) {
            ok = false
            break
        }
    }
    delete have
    return ok
}

def private perm_requires_ok(p : TunePerm) : bool {
    return tune_requires_ok(p.requires)
}

def private suffix_piece(name : string; v : RttiValue) : string {
    if (v is tBool) return (v as tBool) ? name : "no{name}"
    if (v is tInt) return "{name}{v as tInt}"
    if (v is tString) return "{name}_{v as tString}"
    if (v is tFloat) return replace("{name}{v as tFloat}", ".", "d")
    return name
}

    // A function's annotations attach only AFTER every apply has run, so [tune] cannot read its
    // [tune_perm] siblings off func.annotations — each perm banks its row here instead.
var private g_perm_rows : table<string; array<TunePerm>>

def private perm_key(func : FunctionPtr) : string => "{compiling_module().name}::{func.name}"

// one TunePerm from one [tune_perm(...)] declaration's args; suffix= overrides the
// auto-derived name
def private build_perm_row(args : AnnotationArgumentList) : TunePerm {
    var row : TunePerm
    for (aa in args) {
        let v = get_annotation_argument_value(aa)
        if (aa.name == "suffix") {
            if (v is tString) { row.suffix = v as tString }
            continue
        }
        if (aa.name == "requires") {
    // consumed here — must not leak into the stamp args or the auto-derived suffix
            if (v is tString) { row.requires = v as tString }
            continue
        }
        row.args |> emplace((argname = string(aa.name), argvalue = v))
    }
    if (empty(row.suffix)) {
        var pieces <- [for (a in row.args); suffix_piece(a.argname, a.argvalue)]
        row.suffix = join(pieces, "_")
        delete pieces
    }
    return <- row
}

// one companion row: a sibling function stamped with the same perm as the tuned function
struct private TuneCompanion {
    fnname : string
    genkey : string
    @do_not_delete fn : FunctionPtr   // gc_node-owned by the module, not this row
}

var private g_companion_rows : table<string; array<TuneCompanion>>

// pull the banked companion rows for `func` and resolve each sibling function
def private take_companions(func : FunctionPtr; var errors : das_string) : array<TuneCompanion> {
    var comps : array<TuneCompanion>
    let key = perm_key(func)
    if (g_companion_rows |> key_exists(key)) {
        comps <- g_companion_rows[key]
        g_companion_rows |> erase(key)
    }
    var seen : table<string>
    for (c in comps) {
        if (seen |> key_exists(c.fnname)) {
            errors := "[tune] on {func.name}: duplicate companion '{c.fnname}'"
        } elif (c.fnname == func.name) {
            errors := "[tune] on {func.name}: a companion can't be the tuned function itself"
        } else {
            c.fn = compiling_module() |> find_unique_function(c.fnname, true)
            if (c.fn == null) {
                errors := "[tune] on {func.name}: companion function '{c.fnname}' not found - declare exactly one, before the tuned function, in the same module"
            } elif (c.fn.result.baseType == Type.autoinfer) {
                errors := "[tune] on {func.name}: companion {c.fnname} needs an explicit return type"
            } else {
    // up front, or a const-folded zero-arg companion keeps stale reference values at call sites
                c.fn.sideEffectFlags.userScenario = true
            }
        }
        if (!empty(errors)) {
            break
        }
        seen |> insert(c.fnname)
    }
    delete seen
    return <- comps
}

// pull the banked rows for `func` (moving them out of the registry) and validate the set
def private take_perms(func : FunctionPtr; var errors : das_string) : array<TunePerm> {
    var perms : array<TunePerm>
    let key = perm_key(func)
    if (g_perm_rows |> key_exists(key)) {
        perms <- g_perm_rows[key]
        g_perm_rows |> erase(key)
    }
    var seen : table<string>
    for (p in perms) {
        if (empty(p.suffix)) {
            errors := "[tune] on {func.name}: a [tune_perm] with no args needs an explicit suffix=\"...\""
        } elif (p.suffix == "reference") {
            errors := "[tune] on {func.name}: perm suffix 'reference' is reserved for the unstamped body"
        } elif (seen |> key_exists(p.suffix)) {
            errors := "[tune] on {func.name}: duplicate perm suffix '{p.suffix}'"
        }
        if (!empty(errors)) {
            break
        }
        seen |> insert(p.suffix)
    }
    delete seen
    return <- perms
}

// ===== stamping =====

    // The appended declaration's own apply does not run for macro-added annotations, so force
    // the side effects here — without userScenario the front end const-folds constant-arg calls
    // before they reach the JIT. tune_suffix/tune_from ride along as the stamped-truth record.
def private stamp_llvm_code(var fn : FunctionPtr; genKey : string; perm : TunePerm; from, source : string; klass : string = "") {
    var aargs : array<tuple<argname : string; argvalue : RttiValue>>
    aargs |> reserve(length(perm.args) + 5)
    aargs |> push((argname = "name", argvalue = RttiValue(tString = genKey)))
    for (a in perm.args) {
        aargs |> push((argname = a.argname, argvalue = a.argvalue))
    }
    aargs |> push((argname = "tune_suffix", argvalue = RttiValue(tString = perm.suffix)))
    aargs |> push((argname = "tune_from", argvalue = RttiValue(tString = from)))
    aargs |> push((argname = "tune_source", argvalue = RttiValue(tString = source)))
    if (!empty(klass)) {
        // the emitter lowers the clone for this class (llvm_jit try_llvm_code_function)
        aargs |> push((argname = "tune_class", argvalue = RttiValue(tString = klass)))
    }
    fn |> append_annotation("tune", "llvm_code", aargs)
    fn.sideEffectFlags.userScenario = true
    delete aargs
}

    // tune/test modes: one `<base>__<suffix>` clone per perm plus the `<base>_variants()`
    // registry, companions included. The base functions keep their reference bodies as the
    // oracle rows; clones are annotation-free (applies all ran before the clones were made).
def private stamp_grid(var func : FunctionPtr; genKey : string; perms : array<TunePerm>; comps : array<TuneCompanion>; var errors : das_string) : bool {
    var mod = compiling_module()
    for (p in perms) {
        var fclone = clone_function(func)
        fclone.name := "{func.name}__{p.suffix}"
        stamp_llvm_code(fclone, genKey, p, "", "grid")
        fclone.flags.generated = true
        if (!(mod |> add_function(fclone))) {
            errors := "[tune] on {func.name}: failed to add variant {fclone.name}"
            return false
        }
    }
    if (!emit_registry(mod, func, perms, errors)) {
        return false
    }
    for (c in comps) {
        for (p in perms) {
            var cclone = clone_function(c.fn)
            cclone.name := "{c.fnname}__{p.suffix}"
            stamp_llvm_code(cclone, c.genkey, p, "", "grid")
            cclone.flags.generated = true
            if (!(mod |> add_function(cclone))) {
                errors := "[tune] on {func.name}: failed to add companion variant {cclone.name}"
                return false
            }
        }
        if (!emit_registry(mod, c.fn, perms, errors)) {
            return false
        }
    }
    return true
}

// `<base>_variants() : array<tuple<string; <sig>>>` — ("reference", @@base) first, then one row
// per perm. `<sig>` is the base function's own type, so the registry is generic across kernels.
def private emit_registry(var mod : Module?; base : FunctionPtr; perms : array<TunePerm>; var errors : das_string) : bool {
    let sig = function_to_type(base)
    var body : array<ExpressionPtr>
    body |> reserve(length(perms) + 3)
    body |> push <| qmacro_expr() {
        var result : array<tuple<string; $t(sig)>>
    }
    var baseAddr = new ExprAddr(at = base.at, target := "{base.name}")
    body |> push <| qmacro_expr() {
        result |> push(("reference", $e(baseAddr)))
    }
    for (p in perms) {
        var fnAddr = new ExprAddr(at = base.at, target := "{base.name}__{p.suffix}")
        body |> push <| qmacro_expr() {
            result |> push(($v(p.suffix), $e(fnAddr)))
        }
    }
    body |> push <| qmacro_expr() {
        return <- result
    }
    var fn = qmacro_function("{base.name}_variants") $() : array<tuple<string; $t(sig)>> {
        $b(body)
    }
    fn.flags.generated = true
    if (!(mod |> add_function(fn))) {
        errors := "[tune] on {base.name}: failed to add {base.name}_variants"
        return false
    }
    return true
}

// ===== fat mode (compile time) =====

// `x86-vnni512` -> `x86_vnni512`, the class as an identifier piece
def private class_ident(klass : string) : string {
    return replace(klass, "-", "_")
}

// the classes a fat exe carries clones for: every class of the baseline's ladder that some
// scope ships a profile for, plus the baseline itself; most capable first, the baseline last
def private fat_ship_set(baseline : string) : array<string> {
    var ship : array<string>
    var ladder <- fat_ladder()
    for (c in ladder) {
        break if (c == baseline)
        var k = ""
        var p = ""
        var prof <- profile_kernels(k, p, c)
        delete prof
        if (!empty(p)) {
            ship |> push(c)
        }
    }
    ship |> push(baseline)
    delete ladder
    return <- ship
}

// one class's stamp for `fname`: the class profile's winner when the grid carries it and the
// class's set passes its requires=, else the first fallback the class's set passes, else "" (reference)
def private fat_pick(fname, klass : string; perms : array<TunePerm>; args : AnnotationArgumentList;
                     var source, path : string&; var errors : das_string) : string {
    let features = jit_cpu_class_row(klass).features
    var pk = ""
    var pp = ""
    var prof <- profile_kernels(pk, pp, klass)
    let suffix = prof?[fname] ?? ""
    delete prof
    source = ""
    path = ""
    if (!empty(suffix) && suffix != "reference") {
        for (p in perms) {
            if (p.suffix == suffix && requires_ok_in_fingerprint(p.requires, features)) {
                source = "profile"
                path = pp
                return suffix
            }
        }
    } elif (suffix == "reference") {
        source = "profile"
        path = pp
        return ""
    }
    let fv = find_arg(args, "fallback")
    if (fv is tString) {
        for (cand in split(fv as tString, ";")) {
            continue if (empty(cand))
            var known = false
            for (p in perms) {
                known ||= p.suffix == cand
                if (p.suffix == cand && requires_ok_in_fingerprint(p.requires, features)) {
                    source = "fallback"
                    return cand
                }
            }
            if (!known) {
                errors := "[tune] on {fname}: fallback perm '{cand}' is not in the [tune_perm] grid"
                return ""
            }
        }
    }
    source = "reference"
    return ""
}

// a clone of `fn` named for the class, stamped with `suffix` (none for the reference body)
def private add_fat_clone(fn : FunctionPtr; klass, genKey, suffix, from, source : string;
                          perms : array<TunePerm>; var errors : das_string) : string {
    var clone = clone_function(fn)
    let name = "{fn.name}__fat_{class_ident(klass)}"
    clone.name := name
    if (!empty(suffix)) {
        for (p in perms) {
            if (p.suffix == suffix) {
                stamp_llvm_code(clone, genKey, p, from, source, klass)
            }
        }
    }
    clone.flags.generated = true
    if (!(compiling_module() |> add_function(clone))) {
        errors := "[tune] on {fn.name}: failed to add the {klass} clone {name}"
    }
    return name
}

// `<clone>(<every argument of fn, by name>)`
def private fat_call(fn : FunctionPtr; cloneName : string) : ExpressionPtr {
    var call <- new ExprCall(at = fn.at, name := cloneName)
    for (arg in fn.arguments) {
        call.arguments |> emplace_new(new ExprVar(at = fn.at, name := "{arg.name}"))
    }
    return <- call
}

// one dispatch arm: `if ((tune_fat_mask() & bit) != 0) return <clone>(args)`, or the unguarded
// baseline call when bit is 0 (the last arm); a void kernel calls and returns
def private fat_arm(fn : FunctionPtr; cloneName : string; bit : int) : ExpressionPtr {
    var call <- fat_call(fn, cloneName)
    let isVoid = fn.result.baseType == Type.tVoid
    if (bit == 0) {
        if (isVoid) {
            // the last statement of the function: the call alone, one expression for the splice
            return <- qmacro_expr() {
                $e(call)
            }
        }
        return <- qmacro_expr() {
            return $e(call)
        }
    }
    if (isVoid) {
        return <- qmacro_expr() {
            if ((tune_fat_mask() & $v(bit)) != 0) {
                $e(call)
                return
            }
        }
    }
    return <- qmacro_expr() {
        if ((tune_fat_mask() & $v(bit)) != 0) {
            return $e(call)
        }
    }
}

    //! `DAS_TUNE_MODE=fat`: one `<name>__fat_<class>` clone per ship-set class, each stamped from its
    //! class's profile (else the fallback chain under the class's set), and the function's own body
    //! becomes the dispatch chain over `tune_fat_mask()`, baseline clone last. `-exe` only.
[arch(at = "../ARCHITECTURE_TARGET_FEATURES.md#fat-mode")]
def private stamp_fat(var func : FunctionPtr; genKey : string; perms : array<TunePerm>; var comps : array<TuneCompanion>;   // nolint:STYLE037 - one flat pass per class: pick, clone, companions, dispatch arm
                      args : AnnotationArgumentList; var errors : das_string) : bool {
    if (!tune_exe_gate()) {
        errors := "[tune] on {func.name}: DAS_TUNE_MODE=fat is a standalone-exe mode - build with llvm-jit -exe"
        return false
    }
    let baseline = jit_baseline_class()
    if (empty(baseline)) {
        errors := "[tune] on {func.name}: DAS_TUNE_MODE=fat needs DAS_JIT_BASELINE=<class>, the class the exe's plain code targets ({jit_cpu_class_names()})"
        return false
    }
    var ship <- fat_ship_set(baseline)
    var arms : array<ExpressionPtr>
    var picks : array<string>
    arms |> reserve(length(ship))
    picks |> reserve(length(ship))
    for (klass in ship) {
        var source = ""
        var path = ""
        let suffix = fat_pick("{func.name}", klass, perms, args, source, path, errors)
        break if (!empty(errors))
        let cloneName = add_fat_clone(func, klass, genKey, suffix, path, source, perms, errors)
        break if (!empty(errors))
        picks |> push("{klass}={empty(suffix) ? "reference" : suffix}/{source}")
        for (c in comps) {
            // a companion's own profile entry wins when the class's set can run it; else it follows the main
            var own = ""
            var csource = source
            var cpath = path
            if (source == "profile") {
                var pk = ""
                var pp = ""
                var prof <- profile_kernels(pk, pp, klass)
                own = prof?["{c.fnname}"] ?? ""
                delete prof
                var ownOk = false
                for (p in perms) {
                    ownOk ||= p.suffix == own && requires_ok_in_fingerprint(p.requires, jit_cpu_class_row(klass).features)
                }
                own = ownOk ? own : ""
            }
            if (empty(own)) {
                own = suffix
            } else {
                csource = "profile"
                cpath = path
            }
            add_fat_clone(c.fn, klass, c.genkey, own, cpath, csource, perms, errors)
            break if (!empty(errors))
        }
        break if (!empty(errors))
        arms |> push(fat_arm(func, cloneName, klass == baseline ? 0 : tune_fat_class_bit(klass)))
    }
    if (empty(errors)) {
        // the companions' own dispatch: same class arms, so a kernel and its layout query switch together
        for (c in comps) {
            var carms <- [for (klass in ship); fat_arm(c.fn, "{c.fnname}__fat_{class_ident(klass)}", klass == baseline ? 0 : tune_fat_class_bit(klass))]
            c.fn.body := qmacro_block() { $b(carms) }
            c.fn.sideEffectFlags.userScenario = true
        }
        func.body := qmacro_block() { $b(arms) }
        func.sideEffectFlags.userScenario = true
        if (tune_verbosity() != "silent") {
            print("llvm_tune: [tune] {func.name} (DAS_TUNE_MODE=fat): fat clones {join(picks, ", ")}\n")
        }
        // reference-row-only registries, so harness code referencing <name>_variants() still compiles
        let noPerms : array<TunePerm>
        var rok = emit_registry(compiling_module(), func, noPerms, errors)
        for (c in comps) {
            if (rok) {
                rok = emit_registry(compiling_module(), c.fn, noPerms, errors)
            }
        }
    }
    delete picks
    delete ship
    return empty(errors)
}

// ===== the annotations =====

    //! `[tune_perm(<generator args>)]` — one grid row for the `tune(...)` that must follow it in
    //! the same bracket. Args pass to the generator verbatim and fold into the JIT DLL cache
    //! key; `suffix="..."` overrides the auto-derived variant name.
def tune_perm_apply(var func : FunctionPtr; var _group : ModuleGroup;
                    args : AnnotationArgumentList; var _errors : das_string) : bool {
    g_perm_rows[perm_key(func)] |> emplace(build_perm_row(args))
    return true
}

    //! `[tune_companion(fn = "sibling", gen = "generator_key")]` — stamps `sibling` with the SAME
    //! perm from the same sidecar entry, so a kernel and (say) its repack-layout query can never
    //! desync. List these after the [tune_perm] grid and before tune(...). See `skills/tune.md`.
def tune_companion_apply(var func : FunctionPtr; var _group : ModuleGroup;
                         args : AnnotationArgumentList; var errors : das_string) : bool {
    // the bank doubles as the order guard: rows after tune(...) or before any perm are ignored
    if (!(g_perm_rows |> key_exists(perm_key(func)))) {
        errors := "[tune_companion] on {func.name}: list it AFTER the [tune_perm] rows and BEFORE tune(...) in the same bracket"
        return false
    }
    var row : TuneCompanion
    let fv = find_arg(args, "fn")
    if (fv is tString) { row.fnname = fv as tString }
    let gv = find_arg(args, "gen")
    if (gv is tString) { row.genkey = gv as tString }
    if (empty(row.fnname) || empty(row.genkey)) {
        errors := "[tune_companion] on {func.name}: fn=\"<function>\" and gen=\"<generator key>\" are both required"
        return false
    }
    g_companion_rows[perm_key(func)] |> emplace(row)
    return true
}

    //! `[tune(gen="key", fallback="suffix")]` — LAST in a bracket of `[tune_perm]` rows, which it
    //! consumes. Normal runs stamp one winner (sidecar > fallback > reference); tune/test runs
    //! stamp the whole grid plus a `<name>_variants()` registry. Mode contract: module docstring.
def tune_apply(var func : FunctionPtr; var _group : ModuleGroup; // nolint:STYLE037,STYLE038 — the [tune] resolution ladder: manifest/fallback/reference arms in precedence order
               args : AnnotationArgumentList; var errors : das_string) : bool {
    let gv = find_arg(args, "gen")
    if (!(gv is tString) || empty(gv as tString)) {
        errors := "[tune] on {func.name}: gen=\"<[llvm_code] generator key>\" is required"
        return false
    }
    let genKey = gv as tString
    var perms <- take_perms(func, errors)
    if (empty(errors) && empty(perms)) {
        errors := "[tune] on {func.name}: no [tune_perm(...)] rows - list them BEFORE tune(...) in the same annotation bracket"
    }
    var comps <- take_companions(func, errors)
    if (!empty(errors)) {
        delete perms
        delete comps
        return false
    }
    if (func.result.baseType == Type.autoinfer) {
    // function_to_type runs pre-infer: an implicit result reaches the registry as `auto`
        errors := "[tune] on {func.name}: an explicit return type is required (add ': void' or the real type)"
        delete perms
        delete comps
        return false
    }
    // overrides grid modes too: a stale DAS_TUNE_MODE must not leak stubs into generated C++
    if (tune_stamp_free_gate()) {
        let noPerms : array<TunePerm>
        var gok = emit_registry(compiling_module(), func, noPerms, errors)
        for (c in comps) {
            if (gok) {
                gok = emit_registry(compiling_module(), c.fn, noPerms, errors)
            }
        }
        delete perms
        delete comps
        return gok
    }
    let mode = tune_mode()
    if (mode == "tune" || mode == "test") {
        // visible generation cost: a silent permutation stamp reads as a hung compile from outside
        let ctail = empty(comps) ? "" : " (+{length(comps)} companions each)"
        print("llvm_tune: [tune] {func.name}: stamping {length(perms)} permutations{ctail}\n")
        let ok = stamp_grid(func, genKey, perms, comps, errors)
        delete perms
        delete comps
        return ok
    }
    if (mode == "fat") {
        let ok = stamp_fat(func, genKey, perms, comps, args, errors)
        delete perms
        delete comps
        return ok
    }
    if (mode != "normal") {
        errors := "[tune] on {func.name}: unknown DAS_TUNE_MODE '{mode}' (normal | tune | test | fat)"
        delete perms
        delete comps
        return false
    }
    if (g_env_tune.tune_policy == "reference") {
        let noPerms : array<TunePerm>
        var rok = emit_registry(compiling_module(), func, noPerms, errors)
        for (c in comps) {
            break if (!rok)
            rok = emit_registry(compiling_module(), c.fn, noPerms, errors)
        }
        delete perms
        delete comps
        return rok
    }
    // no entry falls back rather than dropping to reference, so a sidecar written before a kernel family landed cannot untune it
    let fname = "{func.name}"
    var manifestFound = false
    let mpath = tune_manifest_path()
    pin_module_cache_dependency(mpath)
    var manifest <- read_manifest(mpath, manifestFound)
    var suffix = ""
    var tuneSource = ""   // manifest | profile | fallback
    var tuneSourcePath = ""
    if (manifestFound) {
        suffix = manifest?[fname] ?? ""
        if (!empty(suffix)) {
            tuneSource = "manifest"
            tuneSourcePath = mpath
        }
    }
    var profileKlass = ""
    var profilePath = ""
    var profile <- empty(suffix) ? profile_kernels(profileKlass, profilePath) : no_kernels()
    if (empty(suffix)) {
        suffix = profile?[fname] ?? ""
        if (!empty(suffix)) {
            tuneSource = "profile"
            tuneSourcePath = profilePath
        }
    }
    // clear a winner this box can't run - ISA-gated (declines to reference) or absent from this build's grid (crash-loop) - so the fallback chain picks a legal perm
    if (!empty(suffix) && suffix != "reference") {
        var known = false
        var eligible = true
        for (p in perms) {
            if (p.suffix == suffix) {
                known = true
                eligible = perm_requires_ok(p)
                break
            }
        }
        if (!known || !eligible) {
            if (tune_verbosity() != "silent") {
                let why = known ? "fails its requires= on this box" : "is not in this build's grid"
                print("llvm_tune: {fname} {tuneSource} winner '{suffix}' {why} - using the fallback chain\n")
            }
            suffix = ""
            tuneSource = ""
            tuneSourcePath = ""
        }
    }
    var companionOwnSuffix : array<string>   // a companion's own entry when it names an eligible perm of this grid; empty follows the main
    companionOwnSuffix |> resize(length(comps))
    if (tuneSource == "manifest" || tuneSource == "profile") {
        for (ci, c in count(), comps) {
            let own = tuneSource == "manifest" ? (manifest?["{c.fnname}"] ?? "") : (profile?["{c.fnname}"] ?? "")
            for (p in perms) {
                if (p.suffix == own && perm_requires_ok(p)) {
                    companionOwnSuffix[ci] = own
                }
            }
        }
    }
    let fromPick = !empty(suffix)
    var ok = true
    if (empty(suffix)) {
        tuneSource = "fallback"
    // ';'-chain in declaration order, first passing requires= wins; none eligible => reference
        let fv = find_arg(args, "fallback")
        if (fv is tString) {
            for (cand in split(fv as tString, ";")) {
                continue if (empty(cand))
                var found = false
                for (p in perms) {
                    if (p.suffix == cand) {
                        found = true
                        if (perm_requires_ok(p)) {
                            suffix = cand
                        }
                        break
                    }
                }
                if (!found) {
                    errors := "[tune] on {func.name}: fallback perm '{cand}' is not in the [tune_perm] grid"
                    ok = false
                }
                break if (!ok || !empty(suffix))
            }
        }
    }
    delete manifest
    delete profile
    if (ok && !empty(suffix) && suffix != "reference") {
        ok = false
        for (p in perms) {
            if (p.suffix == suffix) {
                stamp_llvm_code(func, genKey, p, fromPick ? tuneSourcePath : "", tuneSource)
                for (ci, c in count(), comps) {
                    var stampedOwn = false
                    if (!empty(companionOwnSuffix[ci]) && companionOwnSuffix[ci] != suffix) {
                        for (q in perms) {
                            if (q.suffix == companionOwnSuffix[ci]) {
                                stamp_llvm_code(c.fn, c.genkey, q, tuneSourcePath, tuneSource)
                                stampedOwn = true
                            }
                        }
                        if (stampedOwn && tune_verbosity() == "verbose") {
                            print("llvm_tune: {c.fnname} <- {companionOwnSuffix[ci]} (its own entry, {tuneSourcePath})\n")
                        }
                    }
                    if (!stampedOwn) {
                        stamp_llvm_code(c.fn, c.genkey, p, fromPick ? tuneSourcePath : "", tuneSource)
                    }
                }
                if (tune_verbosity() == "verbose") {
                    print("llvm_tune: {fname} <- {suffix} ({fromPick ? tuneSourcePath : "fallback"})\n")
                }
                ok = true
                break
            }
        }
        if (!ok) {
            errors := "[tune] on {func.name}: perm '{suffix}' is not in the [tune_perm] grid"
        }
    }
    // reference-row-only registries, so harness code referencing <name>_variants() still compiles
    if (ok) {
        let noPerms : array<TunePerm>
        ok = emit_registry(compiling_module(), func, noPerms, errors)
        for (c in comps) {
            if (ok) {
                ok = emit_registry(compiling_module(), c.fn, noPerms, errors)
            }
        }
    }
    delete companionOwnSuffix
    delete perms
    delete comps
    return ok
}

// ===== [tune_scope] / [tune_policy] — the tuner-wiring UX rail =====

def private resolve_decl_path(declFile : string; p : string) : string {
    return is_absolute(p) ? p : path_join(dir_name(declFile), p)
}

def private module_compiled(name : string) : bool {
    var found = false
    let prog = get_ptr(compiling_program())
    if (prog != null) {
        prog |> for_each_module() $(mod) {
            found ||= "{mod.name}" == name
        }
    }
    return found
}

    //! `[tune_scope(name = "mylib", tuner = "harness.das")]` on a dummy struct in the library that
    //! owns the `[tune]` functions, declared ABOVE them: names the tuner that regenerates this
    //! library's winners in the ONE per-app sidecar. `tuner=` resolves against the declaring file.
def tune_scope_apply(var st : StructurePtr; var _group : ModuleGroup;
                     args : AnnotationArgumentList; var errors : das_string) : bool {
    if (tune_aot_gate()) return true
    let nv = find_arg(args, "name")
    if (!(nv is tString) || empty(nv as tString)) {
        errors := "[tune_scope]: name=\"<scope name>\" is required"
        return false
    }
    let sname = nv as tString
    var tuner = ""
    let tv = find_arg(args, "tuner")
    if (tv is tString) {
        tuner = resolve_decl_path(string(st.at.fileInfo.name), tv as tString)
    }
    var covers = ""
    let cv = find_arg(args, "covers")
    if (cv is tString) {
        covers = cv as tString
    }
    var versionKey = ""
    var versionExpected = ""
    let vv = find_arg(args, "version_of")
    if (vv is tString) {
        var keyOverride = ""
        let kv = find_arg(args, "version_key")
        if (kv is tString) {
            keyOverride = kv as tString
        }
        var verr = ""
        if (!resolve_version_of(get_ptr(compiling_program()), vv as tString, keyOverride, versionKey, versionExpected, verr)) {
            errors := "[tune_scope]: {verr}"
            return false
        }
    }
    var defaults = ""
    let dv = find_arg(args, "defaults")
    if (dv is tString) {
        defaults = resolve_decl_path(string(st.at.fileInfo.name), dv as tString)
    }
    if (!empty(defaults)) {
        for (m in split(covers, ";")) {
            continue if (empty(m))
            if (module_compiled(m)) {
                errors := "[tune_scope] '{sname}': covers= module '{m}' compiled before this scope, so its kernels could not read the defaults profile - require this scope's module from '{m}'"
                return false
            }
        }
    }
    // overwrite, not error: shared-module macro state persists across compiles in one process
    g_scopes["{compiling_module().name}"] = TuneScope(scopeName = sname,
        modName = "{compiling_module().name}", covers = covers,
        manifestPath = tune_manifest_path(), tunerPath = tuner,
        versionKey = versionKey, versionExpected = versionExpected,
        defaultsPath = defaults)
    return true
}

// The tune flags, parsed off the APP's command line — we share an argv we do not own, so this
// is the two-arg parse_args form (clargs skips unknown flag-shaped tokens). Never read
// positionals - an app's own `--model foo.gguf` leaves `foo.gguf` in there.
[CommandLineArgs]
struct private TuneCliArgs {
    @clarg_doc = "Re-tune this box's kernels before the app runs"
    tune : bool

    @clarg_name = "tune-quiet"
    @clarg_doc = "Tune with no output at all"
    tune_quiet : bool

    @clarg_name = "tune-verbose"
    @clarg_doc = "Tune with the full per-variant output instead of a progress display"
    tune_verbose : bool

    @clarg_name = "tune-only"
    @clarg_doc = "Comma-separated name filter: re-tune only kernel families whose name contains one of these tokens (implies --tune); every other sidecar entry survives"
    tune_only : string
}

// Adopts --tune-quiet/--tune-verbose/--tune-only and answers whether a forced tune was asked for
// (--tune, or --tune-only alone).
def private apply_cli_tune_flags() : bool {
    var r <- parse_args(type<TuneCliArgs>, get_user_args())
    if (r |> is_err) {
        return false
    }
    let cfg <- r |> move_unwrap
    if (cfg.tune_quiet) {
        tune_set_verbosity("silent")
    } elif (cfg.tune_verbose) {
        tune_set_verbosity("verbose")
    }
    if (!empty(cfg.tune_only)) {
        set_env_variable("DAS_TUNE_ONLY", cfg.tune_only)
    }
    return cfg.tune || !empty(cfg.tune_only)
}

//! The runtime read of `--tune` / `--tune-only` (and the verbosity flags), called by the injected
//! auto guard: true when this run asked for a forced tune.
def tune_cli_force() : bool {
    return apply_cli_tune_flags()
}

// the exact command a user (or the auto path) runs to tune one scope; the env prefix leads
// so the command never starts with a quote (cmd.exe strips a leading quote)
def private tuner_cmd(scope : TuneScope) : string {
    let win = get_platform_name() == "windows"
    // <das_root> can contain spaces: POSIX splits an unquoted VAR=/a b/x.json, Windows needs set "VAR=..."
    let envs = (win ?
        "set \"DAS_TUNE_MODE=tune\"&& set \"DAS_TUNE_MANIFEST={scope.manifestPath}\"&& " :
        "DAS_TUNE_MODE=tune DAS_TUNE_MANIFEST=\"{scope.manifestPath}\" ")
    return "{envs}\"{get_das_exe()}\" -jit \"{scope.tunerPath}\" -dasroot \"{get_das_root()}\""
}

// ===== progress events =====

//! Prefix of a tune progress event. Events are lines on stdout because the tuner runs as a
//! chain of child processes — a pipe is the only channel that spans them. Fields are `k=v`
//! split on spaces, so a VALUE MUST NOT CONTAIN A SPACE — it silently truncates at the first.
let TUNE_EVENT_PREFIX = "@tune "

struct private TuneProgress {
    active : bool
    drawn : bool
    scope : string
    kernel : string
    phase : string
    outerDone : int
    outerTotal : int
    innerDone : int
    innerTotal : int
    live : int
    startTicks : int64
}

var private g_prog : TuneProgress
var private g_verbosity = ""

//! Display verbosity from `DAS_TUNE_VERBOSITY`: `silent` (nothing), `normal` (the default —
//! a progress display and the closing summary) or `verbose` (every line the tuner prints, no
//! display). Anything unrecognized reads as `normal`, so a typo never silences the tune.
def tune_verbosity() : string {
    if (empty(g_verbosity)) {
        let v = g_env_tune.tune_verbosity
        g_verbosity = v == "silent" ? "silent" : (v == "verbose" ? "verbose" : "normal")
    }
    return g_verbosity
}

//! Override the verbosity a `--tune-quiet` / `--tune-verbose` flag selected, before any tuner
//! runs. Exported so the flag rail does not have to go through the environment twice.
def tune_set_verbosity(v : string) {
    g_verbosity = v == "silent" ? "silent" : (v == "verbose" ? "verbose" : "normal")
    set_env_variable("DAS_TUNE_VERBOSITY", g_verbosity)   // the tuner children inherit it
}

// A process draws the display only when its own stdout is a terminal. Otherwise it forwards
// events untouched, so whoever is capturing us (a supervisor, a parent tuner) can draw instead.
def private is_progress_renderer() : bool {
    return is_stdout_terminal()
}

// Emitting goes through the same relay as receiving, so the rule is uniform: a tuner run
// straight from a terminal draws its own display instead of spewing raw event lines, and one
// spawned by a parent forwards them for the parent to draw.
def private emit_event(kind : string; fields : string) {
    relay_line("{TUNE_EVENT_PREFIX}{kind} {fields}\n")
}

//! Announce how many kernels this tuner is about to work through. Call once per tuner half.
def tune_progress_plan(scope : string; total : int) {
    emit_event("plan", "scope={scope} total={total}")
}

var private g_kernel_begin_ticks = 0l

//! Enter a kernel: `total` is its own round budget, the inner counter's denominator.
def tune_progress_kernel_begin(name : string; total : int) {
    g_kernel_begin_ticks = ref_time_ticks()
    emit_event("begin", "name={name} total={total}")
}

//! One round of the current kernel. `phase` names the pass (`screening` / `narrowing` /
//! `finalists`) and `live` is how many grid rows are still in contention.
def tune_progress_kernel_step(i : int; phase : string; live : int) {
    emit_event("step", "i={i} phase={phase} live={live}")
}

//! Leave a kernel. `verdict` is `holds`, `beats`, `rejected` or `skipped`. Every `kernel_begin`
//! needs one, or the outer counter never reaches its plan. `ms` is the wall time since the
//! matching begin - what each family costs is answerable from any mint's log.
def tune_progress_kernel_end(name, winner, verdict : string) {
    let ms = get_time_usec(g_kernel_begin_ticks) / 1000
    emit_event("end", "name={name} winner={winner} verdict={verdict} ms={ms}")
}

def private event_field(parts : array<string>; key, dflt : string) : string {
    for (p in parts) {
        let eq = find(p, "=")
        if (eq > 0 && slice(p, 0, eq) == key) {
            return slice(p, eq + 1)
        }
    }
    return dflt
}

def private elapsed_text(sinceTicks : int64) : string {
    let secs = get_time_usec(sinceTicks) / 1000000
    let m = secs / 60
    let s = secs % 60
    if (m <= 0) return "{s}s"
    let pad = s < 10 ? "0" : ""
    return "{m}m{pad}{s}s"
}

//! The display as a plain string: bar, outer counter, kernel, pass + inner counter, live rows,
//! observed elapsed — every part of it measured, none of it predicted. Exposed so another front
//! end (a GUI, a supervisor's status page) can render the state without redoing the layout.
def tune_progress_line(cols : int) : string {
    let head = "{g_prog.outerDone}/{g_prog.outerTotal}"
    var tail = "  {g_prog.kernel}"
    if (g_prog.innerTotal > 0) {
        tail += "  {g_prog.phase} {g_prog.innerDone}/{g_prog.innerTotal}"
    }
    if (g_prog.live > 0) {
        tail += "  {g_prog.live} live"
    }
    tail += "  {elapsed_text(g_prog.startTicks)}"
    // the bar takes whatever is left after the text, clamped to something still readable
    let barw = clamp(cols - length(head) - length(tail) - 6, 8, 28)
    let filled = g_prog.outerTotal > 0 ? barw * g_prog.outerDone / g_prog.outerTotal : 0
    let bar = build_string() $(w) {
        w |> write("[")
        for (i in range(barw)) {
            w |> write(i < filled ? "#" : "-")
        }
        w |> write("] ")
    }
    return "{bar}{head}{tail}"
}

def private render_progress() {
    let cols = terminal_columns()
    let line = tune_progress_line(cols)
    // pad to the full width so a shorter line cannot leave the previous one's tail behind
    let padding = build_string() $(w) {
        for (_i in range(max(0, cols - length(line) - 1))) {
            w |> write(" ")
        }
    }
    print("\r{line}{padding}\r")
    g_prog.drawn = true
}

def private finish_progress() {
    if (g_prog.drawn) {
        print("\n")
    }
    g_prog.active = false
    g_prog.drawn = false
}

// Fold one event into the display state, drawing it when `render`. State is folded either way
// so a forwarding process still knows a display is live and can mute the raw chatter under it.
def private consume_event(line : string; render : bool) {
    var parts <- split(strip(slice(line, length(TUNE_EVENT_PREFIX))), " ")
    if (empty(parts)) {
        delete parts
        return
    }
    let kind = parts[0]
    if (kind == "plan") {
        // a fresh scope: one relay sees both tuner halves, so the previous scope's kernel must go
        g_prog = TuneProgress(
            scope = event_field(parts, "scope", ""),
            outerTotal = to_int(event_field(parts, "total", "0")),
            startTicks = ref_time_ticks(),
            active = true)
    } elif (kind == "begin") {
        g_prog.kernel = event_field(parts, "name", "")
        g_prog.innerTotal = to_int(event_field(parts, "total", "0"))
        g_prog.innerDone = 0
        g_prog.phase = ""
        g_prog.live = 0
    } elif (kind == "step") {
        g_prog.innerDone = to_int(event_field(parts, "i", "0"))
        g_prog.phase = event_field(parts, "phase", "")
        g_prog.live = to_int(event_field(parts, "live", "0"))
    } elif (kind == "end") {
        g_prog.outerDone++
        g_prog.innerTotal = 0
        g_prog.live = 0
    }
    delete parts
    if (render && g_prog.active) {
        render_progress()
    }
}

//! Fold one relayed line into the progress state, drawing nothing. True when the line WAS a
//! progress event — i.e. not tuner chatter. This plus `tune_progress_line` is the whole seam a
//! different front end (a GUI, a supervisor's status page) needs.
def tune_progress_feed(line : string) : bool {
    if (!starts_with(line, TUNE_EVENT_PREFIX)) return false
    consume_event(line, false)
    return true
}

//! True while a progress display is live — a `plan` event has arrived and no scope has ended.
def tune_progress_active() : bool {
    return g_prog.active
}

//! Drop the progress state, for a front end starting a fresh scope.
def tune_progress_reset() {
    g_prog = TuneProgress()
}

//! Print one of a tuner's own detail lines — the per-variant measurements the display replaces.
//! Shown under `verbose`, or whenever no display is live, so a tuner that emits no progress
//! events prints exactly as it always did. Use this instead of `print` for measurement chatter.
def tune_detail(text : string) {
    let v = tune_verbosity()
    if (v == "silent" || (v == "normal" && g_prog.active)) return
    finish_progress()
    print(text)
}

//! Print a line that must be seen even while a display is live — a failure, not chatter. Wipes
//! the bar's line first so the text is not written over it, and leaves the display armed so the
//! next event redraws the bar underneath. Use `tune_detail` for anything mutable.
def tune_progress_note(text : string) {
    if (tune_verbosity() == "silent") return
    if (g_prog.drawn) {
        let cols = terminal_columns()
        let wipe = build_string() $(w) {
            for (_i in range(max(0, cols - 1))) {
                w |> write(" ")
            }
        }
        print("\r{wipe}\r")
        g_prog.drawn = false
    }
    print(text)
}

//! Close a live display, so whatever prints next starts on its own line. A tuner that emitted
//! progress calls this before it returns; the relay does it automatically when a child exits.
def tune_progress_finish() {
    finish_progress()
}

// Forwarding events is unconditional; verbosity gates only what a human sees. Raw chatter is
// muted only while a display is actually LIVE — a tuner that emits no events must keep printing
// exactly as it always did, or adopting the protocol becomes a prerequisite for being visible.
def private relay_line(line : string) {
    let v = tune_verbosity()
    let renderer = is_progress_renderer()
    if (starts_with(line, TUNE_EVENT_PREFIX)) {
        consume_event(line, renderer && v == "normal")
        if (!renderer) {
            print(line)
        }
        return
    }
    if (v == "silent" || (v == "normal" && g_prog.active)) return
    finish_progress()
    print(line)
}

// The up-front promise. A tune is minutes of work that ends in a relaunch, so say both BEFORE
// the wait — a user (or a supervisor) staring at a silent process cannot infer either.
def private announce_tuning(scopeName : string; force, selfRestarts : bool; detail : string) {
    if (tune_verbosity() == "silent") return
    let why = force ? "re-tuning (--tune)" : "is untuned on this box ({detail})"
    let tail = (selfRestarts
        ? "the process restarts itself when it is done"
        : "the process exits when it is done, and must be restarted to pick up the winners")
    print("llvm_tune: scope '{scopeName}' {why} - tuning now. This takes a while; {tail}.\n")
}

// spawn the scope's tuner, relaying its output LIVE — RUNTIME only (the auto and restart guards).
// Streaming is load-bearing: buffering minutes of tune output leaves every supervisor blind.
// `onlyFilter` arms DAS_TUNE_ONLY for this one spawn and clears it after (the re-exec child must not inherit it).
def private run_scope_tuner(scope : TuneScope; onlyFilter : string = "") : bool {
    let cmd = "{tuner_cmd(scope)} 2>&1"
    if (tune_verbosity() != "silent") {
        print("llvm_tune: tuning scope '{scope.scopeName}' -> {scope.manifestPath}\n")
    }
    if (tune_verbosity() == "verbose") {
        print("llvm_tune: {cmd}\n")
    }
    if (!empty(onlyFilter)) {
        set_env_variable("DAS_TUNE_ONLY", onlyFilter)
    }
    let rc = unsafe(popen(cmd) $(f) {
        if (f != null) {
            while (!feof(f)) {
                relay_line(fgets(f))
            }
        }
    })
    if (!empty(onlyFilter)) {
        set_env_variable("DAS_TUNE_ONLY", "")
    }
    finish_progress()
    if (rc != 0 && tune_verbosity() != "silent") {
        print("llvm_tune: tuner for scope '{scope.scopeName}' exited with {rc}\n")
    }
    return rc == 0
}

// runtime accumulators for the auto/--tune guard: how many scopes just got tuned, and the
// re-exec'd child's exit code
var private g_tuned_scopes = 0
var private g_reexec_code = 0

var private g_mint_suppressed = ""

//! Suppress the auto/restart mint (from a library ``[init]``): untuned scopes keep fallback
//! stamps instead of paying the minutes-long tune. Process-wide across ALL scopes by design —
//! a dev-run escape, not per-scope policy. ``--tune`` still wins. `knob` prints verbatim.
def public tune_suppress_mint(knob : string) {
    g_mint_suppressed = knob
}

// ===== shipped defaults profiles ([tune_scope(defaults = "dir")]) =====

// adopt a shipped class profile into an untuned scope before any race: true = adopted, and
// `onlyFilter` is then the ','-joined families the caller still owes a race ("" = fully covered)
def private profile_try_adopt(defaultsDir, scopeName, manifestPath, fnames, gatedFamilies : string; var onlyFilter : string&) : bool { // nolint:STYLE037,STYLE038 — one straight adopt pipeline: locate, validate, merge, diff
    onlyFilter = ""
    // a runtime-redirected manifest path declines - adoption never lands in a file this guard was not compiled for
    return false if (empty(defaultsDir) || tune_manifest_path() != manifestPath)
    var verKey = ""
    var verVal = ""
    for (tok in split(fnames, ";")) {
        if (tok |> starts_with(TUNE_VERSION_TOKEN)) {
            let kv = slice(tok, length(TUNE_VERSION_TOKEN))
            let eq = find(kv, "=")
            if (eq >= 0) {
                verKey = slice(kv, 0, eq)
                verVal = slice(kv, eq + 1)
            }
        }
    }
    var klass = ""
    var ppath = ""
    var doc = locate_profile_doc(defaultsDir, verKey, verVal, false, klass, ppath)
    return false if (doc == null)
    var kernels : table<string; string>
    let ks = doc?["kernels"]
    if (ks != null && ks.value is _object) {
        for (k, v in keys(ks as _object), values(ks as _object)) {
            if (v != null && v is _string) {
                kernels[k] = v as _string
            }
        }
    }
    if (empty(kernels)) {
        print("llvm_tune: defaults profile '{klass}' has no kernel entries - skipped\n")
        delete_json(doc)
        return false
    }
    var race : array<string>
    for (tok in split(fnames, ";")) {
        continue if (empty(tok) || tok |> starts_with(TUNE_VERSION_TOKEN))
        if (!(kernels |> key_exists(tok))) {
            race |> push(tok)
        }
    }
    let pfeat = "{doc?["provenance"]?["features"] ?? ""}"
    for (row in split(gatedFamilies, ";")) {
        continue if (empty(row))
        let eq = find(row, "=")
        continue if (eq < 0)
        let fam = slice(row, 0, eq)
        continue if (race |> find_index(fam) >= 0)
        for (expr in split(slice(row, eq + 1), "+")) {
            continue if (empty(expr))
            if (tune_requires_ok(expr) && !requires_ok_in_fingerprint(expr, pfeat)) {
                race |> push(fam)
                break
            }
        }
    }
    if (empty(race)) {
        if (tune_verbosity() != "silent") {
            print("llvm_tune: scope '{scopeName}' is covered by the '{klass}' defaults profile ({length(kernels)} kernel entries) - stamped at compile time, no restart\n")
        }
        delete kernels
        delete race
        delete_json(doc)
        return true
    }
    if (!empty(verKey)) {
        tune_provenance_note(verKey, verVal)
    }
    tune_provenance_note("adopted_profile", klass)
    var noRuntime : JsonValue?
    let merged = tune_sidecar_merge(kernels, noRuntime)
    if (!merged) {
        print("llvm_tune: scope '{scopeName}' could not write adopted profile '{klass}' into {manifestPath}\n")
        delete kernels
        delete race
        delete_json(doc)
        return false
    }
    let tail = empty(race) ? "" : " - racing {length(race)} unlocked/new families"
    print("llvm_tune: scope '{scopeName}' adopted the '{klass}' defaults profile ({length(kernels)} kernel entries){tail}\n")
    onlyFilter = join(race, ",")
    delete kernels
    delete race
    delete_json(doc)
    return true
}

    //! Runtime half of `[tune_policy(missing = "auto")]` / `--tune`, injected at the top of `main`
    //! and followed by `tune_auto_reexec()`. Adopts a declared defaults profile first, then tunes
    //! the residue at RUNTIME (winners are compile-time stamps). A corrupt sidecar self-heals.
def tune_auto_prepare(scopeName : string; manifestPath : string; tunerPath : string; force : bool; fnames : string;
                      defaultsDir : string = ""; gatedFamilies : string = "") {
    let v = tune_sidecar_verdict(manifestPath, fnames)
    if (!force && v.complete) {
        return
    }
    if (g_env_tune.tune_relaunch > 0) {
        print("llvm_tune: scope '{scopeName}' is still untuned after the relaunch (DAS_TUNE_RELAUNCH={g_env_tune.tune_relaunch}; {v.detail}) - the tune did not converge; kernels run the stamps as compiled\n")
        return
    }
    var onlyFilter = ""
    if (!force && profile_try_adopt(defaultsDir, scopeName, manifestPath, fnames, gatedFamilies, onlyFilter) && empty(onlyFilter)) {
        return
    }
    if (!force && empty(onlyFilter) && scope_resolved_externally(scopeName, manifestPath, fnames)) {
        g_tuned_scopes++   // the re-exec picks the resolved sidecar up like a fresh tune
        return
    }
    if (!force && !empty(g_mint_suppressed)) {
        if (tune_verbosity() != "silent") {
            print("llvm_tune: scope '{scopeName}' is untuned ({v.detail}) - mint SUPPRESSED ({g_mint_suppressed}); kernels run their fallback\n")
        }
        return
    }
    announce_tuning(scopeName, force, true, v.detail)
    run_scope_tuner(TuneScope(scopeName = scopeName, manifestPath = manifestPath, tunerPath = tunerPath), onlyFilter)
    let after = tune_sidecar_verdict(manifestPath, fnames)
    if (after.complete) {
        g_tuned_scopes++
    } else {
        print("llvm_tune: tuner left scope '{scopeName}' incomplete in {manifestPath} - {after.detail}\n")
    }
}

    //! Relaunch the process so a fresh compile stamps the just-written manifests — they cannot be
    //! adopted mid-run. `--tune` / `--tune-only` are stripped from the relaunch so the child converges. True means
    //! the child ran and main must return `tune_reexec_code()` immediately.
def tune_auto_reexec() : bool {
    if (g_tuned_scopes == 0) {
        return false
    }
    // aliases a process-global Array header — do not delete
    var argv <- get_command_line_arguments()
    var args : array<string>
    args |> reserve(length(argv))
    var skipTuneOnlyValue = false
    for (a in argv) {
        if (skipTuneOnlyValue) {
            skipTuneOnlyValue = false
            continue
        }
        continue if (a == "--tune" || starts_with(a, "--tune-only="))
        if (a == "--tune-only") {
            skipTuneOnlyValue = true
            continue
        }
        args |> push("{a}")
    }
    print("llvm_tune: re-launching to apply the fresh manifests\n")
    set_env_variable("DAS_TUNE_RELAUNCH", "{g_env_tune.tune_relaunch + 1}")
    let rc = unsafe(popen_argv(args, 0.0) $(f) {
        if (f != null) {
            while (!feof(f)) {
                print(fgets(f))
            }
        }
    })
    delete args
    g_reexec_code = rc
    return true
}

//! The re-exec'd child's exit code — what an `int` main returns after `tune_auto_reexec()`.
def tune_reexec_code() : int {
    return g_reexec_code
}

// the pre-tune scope resolver: consulted by the auto and restart guards before spawning the
// tuner; an app registers one from an [init] (which runs before the guard at the top of main)
var private g_scope_resolver : function<(scopeName : string; manifestPath : string; fnames : string) : bool>
var private g_has_scope_resolver = false

    //! Register a resolver the auto/restart guards consult before spawning a scope's tuner —
    //! e.g. an exchange client downloading a matching sidecar. True = "the scope may now be
    //! satisfied"; completeness is re-checked, never trusted. Never consulted under `--tune`.
def tune_set_scope_resolver(resolver : function<(scopeName : string; manifestPath : string; fnames : string) : bool>) {
    g_scope_resolver = resolver
    g_has_scope_resolver = true
}

// resolver consult shared by both guards: true = the scope is complete now, skip the tuner
// (the resolver prints its own restart marker — the guards add nothing on this path)
def private scope_resolved_externally(scopeName : string; manifestPath : string; fnames : string) : bool {
    if (!g_has_scope_resolver) {
        return false
    }
    return (invoke(g_scope_resolver, scopeName, manifestPath, fnames)
        && tune_sidecar_entries_complete(manifestPath, fnames))
}

    //! True when the file named by `DAS_TUNE_CONTROL` exists — a supervisor's stop request.
    //! Tuners poll at kernel-family boundaries and abort without minting, so the measurement
    //! in flight always completes.
def tune_interrupt_requested() : bool {
    let p = g_env_tune.tune_control
    if (empty(p)) {
        return false
    }
    var fs : FStat
    return stat(p, fs)
}

    //! The `DAS_TUNE_POLICY` override, verbatim ("" = none) — how a sibling stamping macro
    //! (e.g. a library's `[tuned]` loop-hint rail) honors `reference` and `fallback` too.
def tune_policy_env() : string {
    return g_env_tune.tune_policy
}

    //! The `--tune-only` re-mint filter (`DAS_TUNE_ONLY`), verbatim: comma-separated name
    //! tokens, empty = race everything.
def tune_only_filter() : string {
    return g_env_tune.tune_only
}

    //! True when family `name` should race under the `--tune-only` filter - no filter set, or
    //! some comma-separated token is a substring of the name. A harness consults this at each
    //! family boundary; a skipped family writes no entry, so its sidecar seat survives.
def tune_family_selected(name : string) : bool {
    let f = g_env_tune.tune_only
    if (empty(f)) {
        return true
    }
    for (tok in split(f, ",")) {
        let t = strip(tok)
        if (!empty(t) && find(name, t) >= 0) {
            return true
        }
    }
    return false
}

    //! Runtime half of `[tune_policy(missing = "restart")]`, injected as a guard at the top of
    //! the app's `main`: adopts a declared defaults profile, then consults the scope resolver, else runs the tuner; true = main
    //! returns immediately (winners are compile-time stamps), false = scope complete, carry on.
def tune_restart_needed(scopeName : string; manifestPath : string; tunerPath : string; fnames : string;
                        defaultsDir : string = ""; gatedFamilies : string = "") : bool {
    let v = tune_sidecar_verdict(manifestPath, fnames)
    if (v.complete) {
        return false
    }
    var onlyFilter = ""
    if (profile_try_adopt(defaultsDir, scopeName, manifestPath, fnames, gatedFamilies, onlyFilter) && empty(onlyFilter)) {
        return false
    }
    if (empty(onlyFilter) && scope_resolved_externally(scopeName, manifestPath, fnames)) {
        return true   // resolved without a tune; the resolver printed its own restart marker
    }
    if (!empty(g_mint_suppressed)) {
        // false = carry on with fallback stamps, do not tune and do not stop main
        if (tune_verbosity() != "silent") {
            print("llvm_tune: scope '{scopeName}' is untuned ({v.detail}) - mint SUPPRESSED ({g_mint_suppressed}); kernels run their fallback\n")
        }
        return false
    }
    if (empty(tunerPath)) {
        print("llvm_tune: scope '{scopeName}' is untuned on this box ({v.detail})\n")
        print("llvm_tune: no tuner declared - produce {manifestPath} with the library's tuner, then restart\n")
        return true
    }
    announce_tuning(scopeName, false, false, v.detail)
    run_scope_tuner(TuneScope(scopeName = scopeName, manifestPath = manifestPath, tunerPath = tunerPath), onlyFilter)
    let after = tune_sidecar_verdict(manifestPath, fnames)
    if (after.complete) {
        print("llvm_tune: scope '{scopeName}' tuned -> {manifestPath}\nllvm_tune: restart to apply the winners\n")
    } else {
        print("llvm_tune: tuner left scope '{scopeName}' incomplete in {manifestPath} - {after.detail}\n")
    }
    return true
}

// final stamped truth per [tune] function, read off the AST (the tune_suffix/tune_from args
// stamp_llvm_code records) — no macro-state bank, so shared-context reuse across compiles
// can't pollute it
def private collect_status(prog : Program?) : array<TuneStatus> {
    var rows : array<TuneStatus>
    let sidecar = tune_manifest_path()
    prog |> for_each_module() $(mod) {
        let modName = "{mod.name}"
        var scopeName = ""
        if (g_scopes |> key_exists(modName)) {
            scopeName = g_scopes[modName].scopeName
        }
        for_each_function(mod, "") $(fn) {
            var isTuned = false
            var suffix = ""
            var from = ""
            var stampedSource = ""
            var klass = ""
            for (ann in fn.annotations) {
                if (ann.annotation.name == "tune") {
                    isTuned = true
                } elif (ann.annotation.name == "llvm_code") {
                    let sv = find_arg(ann.arguments, "tune_suffix")
                    if (sv is tString) { suffix = sv as tString }
                    let fv = find_arg(ann.arguments, "tune_from")
                    if (fv is tString) { from = fv as tString }
                    let tv = find_arg(ann.arguments, "tune_source")
                    if (tv is tString) { stampedSource = tv as tString }
                    let cv = find_arg(ann.arguments, "tune_class")
                    if (cv is tString) { klass = cv as tString }
                }
            }
            // a fat build's rows are its clones, one per class - the [tune] function itself is the dispatch
            let fatName = "{fn.name}"
            let fatAt = find(fatName, "__fat_")
            if (fatAt > 0 && fn.flags.generated && (!empty(klass) || tune_mode() == "fat")) {
                let base = slice(fatName, 0, fatAt)
                if (empty(klass)) {
                    klass = replace(slice(fatName, fatAt + length("__fat_")), "_", "-")
                }
                let source = !empty(stampedSource) ? stampedSource : "reference"
                rows |> push(TuneStatus(fname = base, scope = scopeName,
                    suffix = suffix, source = source, manifestPath = from, klass = klass))
            } elif (isTuned && tune_mode() != "fat") {
                let source = !empty(stampedSource) ? stampedSource : (empty(suffix) ? "reference" : (empty(from) ? "fallback" : "manifest"))
                let mpath = !empty(from) ? from : sidecar
                rows |> push(TuneStatus(fname = "{fn.name}", scope = scopeName,
                    suffix = suffix, source = source, manifestPath = mpath))
            }
        }
    }
    return <- rows
}

// warn once per (scope, manifest) per process — infer passes re-run, and shared macro-module
// state persists across compiles in one process
var private g_manifest_warned : table<string>

// DAS_TUNE_MANIFEST disables the policy rail (explicit manifest = manual) — but a pointed-at
// file that reads untuned means every kernel silently stamps its fallback while the operator
// believes the winners are live: the one measurement hazard worth a loud line
def private warn_manifest_untuned() {
    return if (empty(g_env_tune.tune_manifest) || tune_stamp_free_gate() || tune_mode() != "normal")
    for (scope in values(g_scopes)) {
        let key = "{scope.scopeName}|{scope.manifestPath}"
        continue if (g_manifest_warned |> key_exists(key))
        pin_module_cache_dependency(scope.manifestPath)
        let v = tune_sidecar_verdict(scope.manifestPath, scope_fnames_joined(scope))
        continue if (v.complete)
        g_manifest_warned |> insert(key)
        print("llvm_tune: WARNING DAS_TUNE_MANIFEST={g_env_tune.tune_manifest}: scope '{scope.scopeName}' reads untuned ({v.detail}); kernels run fallback stamps\n")
    }
}

    // AOT runs no policy; grid-mode children manage their own manifest (also the recursion guard);
    // a DAS_TUNE_MANIFEST override means manual. -exe is NOT here — it runs no policy either but
    // still owes the status [init], so its call sites re-admit it via exe_status_report().
def private tune_policy_gated() : bool {
    return (tune_stamp_free_gate() || tune_mode() != "normal"
        || !empty(g_env_tune.tune_manifest))
}

    // -exe owes the status [init] even under a DAS_TUNE_MANIFEST redirect (report ≠ policy —
    // else a redirect-built exe ships stamp-less rows); stamp-free/grid still win
def private exe_status_report() : bool {
    return tune_exe_gate() && !tune_stamp_free_gate() && (tune_mode() == "normal" || tune_mode() == "fat")
}

// the runtime status table: one tune_status_add per [tune] function, off the stamped AST
// truth — emitted by policy runs AND by -exe builds (an artifact must be able to report
// which perms it carries: log_tune_status / tune_status work inside a standalone exe)
def private emit_status_init(var errors : string&) {
    var rows <- collect_status(get_ptr(compiling_program()))
    if (!empty(rows)) {
        var body : array<ExpressionPtr>
        body |> reserve(length(rows) + 1)
        for (row in rows) {
            body |> push <| qmacro_expr() {
                tune_status_add($v(row.fname), $v(row.scope), $v(row.suffix),
                    $v(row.source), $v(row.manifestPath), $v(row.klass))
            }
        }
        if (tune_mode() == "fat") {
            // after the rows, so a pin is checked against the clones the exe carries; an unset mask reads as the baseline
            let baseline = jit_baseline_class()
            body |> push <| qmacro_expr() {
                tune_fat_init($v(baseline))
            }
        }
        var fn = qmacro_function("`tune`status`init") $() : void {
            $b(body)
        }
        fn |> append_annotation("$", "init")
        fn.flags.generated = true
        if (!(compiling_module() |> add_function(fn))) {
            errors = "[tune_policy]: duplicate declaration (one per program)"
        }
    }
    delete rows
}

// the policy core, shared by the declared annotation and the auto default (the infer pass
// below): resolve the policy (env > declared), then per incomplete scope warn / error /
// inject the auto or restart guard into `func` (the app's main)
def private apply_tune_policy(var func : FunctionPtr; declared : string; var errors : string&) : bool { // nolint:STYLE037,STYLE038 — the policy dispatch ladder: one arm per flavor and gate
    var policy = declared
    {
        let penv = g_env_tune.tune_policy
        if (!empty(penv)) {
            policy = penv
            // the announce fires where the override changes the outcome - `reference` alters every stamp even on a fully tuned box
            if (tune_verbosity() != "silent") {
                print("llvm_tune: policy '{policy}' set by DAS_TUNE_POLICY (declared/default '{declared}')\n")
            }
        }
        if (policy != "fallback" && policy != "reference" && policy != "warn" && policy != "error" && policy != "auto" && policy != "restart") {
            errors = "[tune_policy] on {func.name}: missing=\"{policy}\" - use fallback | reference | warn | error | auto | restart"
            return false
        }
        if (declared == "reference" && penv != "reference") {
            // the [tune] stamps in required libraries compile before this root exists and read the env only
            errors = "[tune_policy] on {func.name}: missing=\"reference\" is an environment override (DAS_TUNE_POLICY=reference), not a declarable value"
            return false
        }
        let force = apply_cli_tune_flags()
        if (force && empty(g_scopes)) {
            print("llvm_tune: --tune requested but no [tune_scope] is declared by any required library\n")
        }
    // snapshot: values() locks the table for the loop, and the body reads g_scopes indirectly
        var scopes <- [for (scope in values(g_scopes)); scope]
        var seen : table<string>
        var autoScopes : array<TuneScope>
        for (scope in scopes) {
            continue if (seen |> key_exists(scope.scopeName))
            seen |> insert(scope.scopeName)
            // per-scope KEY completeness, not file existence — the sidecar is shared
            let scopeFnames = scope_fnames_joined(scope)
            pin_module_cache_dependency(scope.manifestPath)
            let v = tune_sidecar_verdict(scope.manifestPath, scopeFnames)
            continue if (!force && v.complete && policy != "auto")
            if (force || policy == "auto") {
    // a runtime guard tunes and RE-EXECS: restamping a required module's compiled function is a cross-module mutation re-infer cannot survive
                if (empty(scope.tunerPath)) {
                    if (!v.complete) {
                        errors = "[tune_policy]: scope '{scope.scopeName}' declares no tuner= - cannot {force ? "--tune" : "auto-tune"}; produce {scope.manifestPath} manually"
                    }
                } else {
                    autoScopes |> push(scope)
                }
            } elif (policy == "error") {
                errors = "[tune_policy]: scope '{scope.scopeName}' is untuned on this box ({v.detail}) - run: {empty(scope.tunerPath) ? "<no tuner declared>" : tuner_cmd(scope)} (or start with --tune)"
            } elif (policy == "warn") {
                print("llvm_tune: WARNING scope '{scope.scopeName}' is untuned on this box ({v.detail}); fallback stamps in effect\nllvm_tune: run: {empty(scope.tunerPath) ? "<no tuner declared>" : tuner_cmd(scope)} (or start with --tune)\n")
            } elif (policy == "restart") {
    // graceful guard, not exit(): a hard exit from an [init] skips environment shutdown
                if (!(func.body is ExprBlock)) {
                    errors = "[tune_policy]: restart - {func.name} has no body block"
                } elif (func.result.baseType != Type.tVoid && func.result.baseType != Type.tInt
                        && func.result.baseType != Type.autoinfer) {
                    // autoinfer = undeclared result, void by convention for main
                    errors = "[tune_policy]: restart - {func.name} must return void or int"
                } else {
                    var guard : ExpressionPtr
                    let scopeGated = scope_gated_families_joined(scope)
                    if (func.result.baseType == Type.tInt) {
                        guard = qmacro_expr() {
                            if (tune_restart_needed($v(scope.scopeName), $v(scope.manifestPath), $v(scope.tunerPath), $v(scopeFnames),
                                    $v(scope.defaultsPath), $v(scopeGated))) {
                                return 3
                            }
                        }
                    } else {
                        guard = qmacro_expr() {
                            if (tune_restart_needed($v(scope.scopeName), $v(scope.manifestPath), $v(scope.tunerPath), $v(scopeFnames),
                                    $v(scope.defaultsPath), $v(scopeGated))) {
                                return
                            }
                        }
                    }
                    var blk = unsafe(reinterpret<ExprBlock?>(func.body))
                    blk.list |> push(guard, 0)
                }
            }
            break if (!empty(errors))
        }
        delete seen
        delete scopes
    // order is prepare(s) first, re-exec last, both ahead of the original body
        if (empty(errors) && !empty(autoScopes)) {
            if (!(func.body is ExprBlock)) {
                errors = "[tune_policy]: {func.name} has no body block"
            } elif (func.result.baseType != Type.tVoid && func.result.baseType != Type.tInt
                    && func.result.baseType != Type.autoinfer) {
                // autoinfer = undeclared result, void by convention for main
                errors = "[tune_policy]: auto/--tune - {func.name} must return void or int"
            } else {
                var guards : array<ExpressionPtr>
                guards |> reserve(length(autoScopes) + 1)
                for (scope in autoScopes) {
                    let fnames = scope_fnames_joined(scope)
                    let gated = scope_gated_families_joined(scope)
                    guards |> push <| qmacro_expr() {
                        tune_auto_prepare($v(scope.scopeName), $v(scope.manifestPath), $v(scope.tunerPath), tune_cli_force(), $v(fnames),
                            $v(scope.defaultsPath), $v(gated))
                    }
                }
                if (func.result.baseType == Type.tInt) {
                    guards |> push <| qmacro_expr() {
                        if (tune_auto_reexec()) {
                            return tune_reexec_code()
                        }
                    }
                } else {
                    guards |> push <| qmacro_expr() {
                        if (tune_auto_reexec()) {
                            return
                        }
                    }
                }
                var blk = unsafe(reinterpret<ExprBlock?>(func.body))
                var gi = length(guards) - 1
                while (gi >= 0) {
                    blk.list |> push(guards[gi], 0)
                    gi--
                }
            }
        }
        delete autoScopes
        // with auto now tuning at runtime the rows always reflect this compile's stamps
        if (empty(errors)) {
            emit_status_init(errors)
        }
    }
    return empty(errors)
}

    //! `[tune_policy(missing = "...")]` on the app's `main`: what to do about scopes whose sidecar
    //! entries are missing or stale — `fallback` | `warn` | `error` | `auto` (THE DEFAULT, declared
    //! or not) | `restart`. Full semantics and the escapes: `skills/tune.md`.
def tune_policy_apply(var func : FunctionPtr; var _group : ModuleGroup;
                      args : AnnotationArgumentList; var errors : das_string) : bool {
    // the manifest warning is the default pass's job — it fires for every root, this apply's gate included
    if (tune_policy_gated() && !exe_status_report()) {
        return true
    }
    var errs = ""
    if (tune_exe_gate()) {
        // standalone build: no policy, but the artifact self-reports its stamps
        emit_status_init(errs)
    } else {
        var policy = "auto"
        let mv = find_arg(args, "missing")
        if (mv is tString) {
            policy = mv as tString
        }
        apply_tune_policy(func, policy, errs)
    }
    if (!empty(errs)) {
        errors := errs
        return false
    }
    return true
}

// inert once-only marker the TuneDefaultPolicy pass stamps onto main — appending an
// annotation RUNS its apply (add_function_annotation), so the marker must not be
// [tune_policy] itself (that would re-enter the policy with whatever env is set)
[function_macro(name = "tune_policy_default_applied")]
class private TunePolicyDefaultMarker : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; var errors : das_string) : bool {
        return true
    }
}

    //! The "untuned does not start" default: a program root with a `main`, some `[tune_scope]` in
    //! play and no declared `[tune_policy]` behaves as `missing = "auto"`. Escapes and the
    //! once-only marker: `skills/tune.md`. A root without `main` never tunes (dastest files).
def tune_default_policy_apply(_prog : ProgramPtr; _mod : Module?) : bool {
    return false if (is_in_completion() || !empty("{compiling_module().name}") || empty(g_scopes))
    warn_manifest_untuned()   // self-gates on DAS_TUNE_MANIFEST; covers exe and declared-policy roots too
    return false if (tune_policy_gated() && !exe_status_report())
    var main_fn = compiling_module() |> find_unique_function("main", false)
    return false if (main_fn == null)
    for (ann in main_fn.annotations) {
        // declared policy wins; the marker means this pass already ran
        return false if (ann.annotation.name == "tune_policy"
            || ann.annotation.name == "tune_policy_default_applied")
    }
    main_fn |> append_annotation("llvm_tune", "tune_policy_default_applied")
    var errs = ""
    var ok = true
    if (tune_exe_gate()) {
        // standalone build: no policy, but the artifact self-reports its stamps
        emit_status_init(errs)
    } else {
        let flavor = jit_cli_opt_level() == 0 ? "fallback" : "auto"
        ok = apply_tune_policy(main_fn, flavor, errs)
    }
    if (!ok || !empty(errs)) {
    // macro_error mid-infer-pass is dropped by the retry loop — print + panic actually stops it
        print("llvm_tune: {errs}\n")
        panic("llvm_tune default policy: {errs}")
    }
    return true   // modified — re-run infer over the injected status/guard
}
