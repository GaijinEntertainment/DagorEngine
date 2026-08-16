options gen2
options indenting = 4
options no_aot = true

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

//! Generalized tune framework (gemm_generator_plan.md part 2). `[tune_perm(...), ...,
//! tune(gen="key", fallback="suffix")]` — perm rows first, `tune` last — turns one reference
//! function into a tuned `[llvm_code]` kernel family. `[tune_companion(fn=, gen=)]` rows
//! (between the perm rows and `tune`) stamp sibling functions with the SAME perm from the same
//! manifest entry — the two-function stamp (a kernel and its repack-layout query can never
//! desync). `DAS_TUNE_MODE` selects the compile-time mode:
//!  - normal (default): stamp exactly one winner onto the function itself — the manifest
//!    entry when one exists, else the `fallback=` perm, else the reference body (a manifest
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
//! loop-hint perms), `"runtime"` (library runtime knobs), `"provenance"` (who wrote it).
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

//! Compile-time tune mode from the `DAS_TUNE_MODE` env var: "normal" (default), "tune", "test".
def tune_mode() : string {
    let m = get_env_variable("DAS_TUNE_MODE")
    return empty(m) ? "normal" : m
}

    //! True when this compile must stay stamp-free: `policies.tune_frozen` (the cross-box
    //! artifact mark) or an AOT-consuming run without the JIT (a stamp changes the semantic
    //! hash). Every tune annotation goes inert; only reference-row registries are emitted.
def tune_aot_gate() : bool {
    return (compiling_program().policies.tune_frozen
        || (compiling_program().policies.aot && !compiling_program().policies.jit_enabled))
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

    // argv[0] verbatim, NOT get_das_exe(): that derives the interpreter-to-SPAWN, which does
    // not exist inside a released bundle and mis-anchored the exe sidecar + staleness checks.
def private running_binary() : string {
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

    //! The tune sidecar location: `DAS_TUNE_MANIFEST` env, else a harness override, else
    //! `<app>.tune.json` beside the app (the root script, or the binary when there is none).
    //! Never "" — every app has a sidecar location.
def tune_manifest_path() : string {
    let env = get_env_variable("DAS_TUNE_MANIFEST")
    if (!empty(env)) return env
    if (!empty(g_runtime_manifest)) return g_runtime_manifest
    let app = tune_app_script()
    return sidecar_for(empty(app) ? running_binary() : app)
}

var private g_box_identity = ""

    //! This box's tune identity: platform|arch|hardware|OS-build|cpu. Measurements are a
    //! property of the box AND its OS (GPU fallback perf moves with macOS updates), so a
    //! sidecar carrying a different identity reads as stale — the cross-box-copy hazard.
def tune_box_identity() : string {
    if (!empty(g_box_identity)) {
        return g_box_identity
    }
    var hw = ""
    let plat = get_platform_name()
    if (plat == "windows") {
        hw = "{get_env_variable("PROCESSOR_IDENTIFIER")}|{get_env_variable("OS")}"
    } elif (plat == "osx" || plat == "darwin") {   // get_platform_name says "darwin" on macOS
        unsafe {
            popen("sysctl -n hw.model kern.osversion machdep.cpu.brand_string 2>/dev/null") $(f) {
                while (!feof(f)) {
                    let ln = strip(fgets(f))
                    if (!empty(ln)) {
                        hw = empty(hw) ? ln : "{hw}|{ln}"
                    }
                }
            }
        }
    } else {
        unsafe {
            popen("uname -r 2>/dev/null") $(f) {
                hw = strip(fgets(f))
            }
        }
        fopen("/proc/cpuinfo", "rb") $(f) {
            if (f != null) {
                while (!feof(f)) {
                    let ln = fgets(f)
                    if (ln |> starts_with("model name")) {
                        let ix = find(ln, ":")
                        hw = "{hw}|{strip(slice(ln, ix + 1))}"
                        break
                    }
                }
            }
        }
    }
    g_box_identity = "{plat}|{get_architecture_name()}|{hw}"
    return g_box_identity
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
            box = doc?["provenance"]?["box"] ?? ""
            delete_json(doc)
        }
    }
    g_box_cache[key] = box
    return box
}

    //! True when the sidecar predates the running binary OR carries a different box identity
    //! (missing counts as different — pre-identity files re-tune once); a stale sidecar reads
    //! as ABSENT everywhere, killing the copy-a-stale-file and copy-from-another-box traps.
def tune_sidecar_stale(path : string) : bool {
    var sfs : FStat
    return false if (!stat(path, sfs))   // absent is "missing", not "stale"
    var bfs : FStat
    if (stat(running_binary(), bfs) && sfs.mtime < bfs.mtime) {
        return true
    }
    return sidecar_box(path, "{sfs.mtime}") != tune_box_identity()
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

// the sidecar document for read-modify-write: parsed when fresh, a new empty object when
// absent, unparsable, or STALE (stale measurements are invalid; the first save makes the
// file newer than the binary so the rest of the run appends)
def private load_sidecar_doc(path : string) : JsonValue? {
    var doc : JsonValue?
    if (!tune_sidecar_stale(path)) {
        let text = fread(path)
        if (!empty(text)) {
            var err = ""
            doc = read_json(text, err)
        }
    }
    if (doc == null || !(doc.value is _object)) {
        var fresh : table<string; JsonValue?>
        doc = JV(fresh)
    }
    return doc
}

// refresh "provenance" (base identity + any tune_provenance_note entries) and write back
def private save_sidecar_doc(path : string; var doc : JsonValue?) : bool {
    var prov = JV((binary = running_binary(), platform = get_platform_name(),
        arch = get_architecture_name(), box = tune_box_identity(), written = iso8601_now()))
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

// ===== the tune history (~/.tune-history) =====

var private g_history_dir = ""

    //! This box's tune-history directory: `DAS_TUNE_HISTORY` env, else
    //! `<home>/.tune-history/<sanitized box identity>/`. Every mint archives here — sidecars
    //! are small and the history is the box's longitudinal health record; NEVER deleted.
def tune_history_dir() : string {
    if (!empty(g_history_dir)) {
        return g_history_dir
    }
    let env = get_env_variable("DAS_TUNE_HISTORY")
    if (!empty(env)) {
        g_history_dir = env
        return g_history_dir
    }
    var home = get_env_variable("HOME")
    if (empty(home)) {
        home = get_env_variable("USERPROFILE")
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
    var home = get_env_variable("HOME")
    if (empty(home)) {
        home = get_env_variable("USERPROFILE")
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

    //! The probe-cv ceiling (percent) above which a tuner must refuse to race: winners picked
    //! in louder noise are noise. `DAS_TUNE_NOISE_CV` overrides (calibration; garbage ignored),
    //! else 2% normal / 1% paranoid — confirmed empirically on the quiet boxes during rollout.
def tune_noise_threshold_pct(paranoid : bool) : double {
    if (g_noise_thr < 0.0lf) {
        let v = to_float(get_env_variable("DAS_TUNE_NOISE_CV"))
        g_noise_thr = v > 0.0f ? double(v) : 0.0lf
    }
    return g_noise_thr > 0.0lf ? g_noise_thr : (paranoid ? 1.0lf : 2.0lf)
}

    //! True when `DAS_TUNE_NOISE_OVERRIDE` asks to tune through a failing noise gate anyway.
    //! The tuner that honors it must stamp the sidecar `noise: overridden` — the escape leaves
    //! a mark, so a later regression hunt can see the mint was taken on a loud box.
def tune_noise_override() : bool {
    if (g_noise_ovr < 0) {
        let v = get_env_variable("DAS_TUNE_NOISE_OVERRIDE")
        g_noise_ovr = (!empty(v) && v != "0") ? 1 : 0
    }
    return g_noise_ovr == 1
}

// ===== tune status (runtime) =====

//! One tuned function's final compile-time decision, as reported by `tune_status()`.
struct TuneStatus {
    fname : string
    scope : string          // [tune_scope] name, "" when env/app-manifest driven
    suffix : string         // winning perm suffix, "" for the reference body
    source : string         // "manifest" | "fallback" | "reference"
    manifestPath : string   // the manifest the winner came from (or would come from)
}

var private g_tune_status : array<TuneStatus>

//! Hook the `[tune_policy]`-injected [init] calls once per tuned function.
def tune_status_add(fname, scope, suffix, source, manifestPath : string) {
    g_tune_status |> push(TuneStatus(fname = fname, scope = scope, suffix = suffix,
        source = source, manifestPath = manifestPath))
}

//! The program's tune decisions — one row per `[tune]` function. Populated whenever the
//! policy rail ran (a declared `[tune_policy]` or the auto default on any app with tune
//! scopes); empty under the gates (AOT, `-exe`, grid modes, `DAS_TUNE_MANIFEST`).
def tune_status() : array<TuneStatus> {
    return clone_to_move(g_tune_status)
}

    //! Log the tune status table at `LOG_INFO` — the shared "am I tuned?" surface, prefixed by
    //! `appName`. Prints `<n>/<total> kernels tuned for this box`, one line per function, and a
    //! `--tune` hint when any kernel sits on a fallback/reference tier. No-op when empty.
def log_tune_status(appName : string) {
    var st <- tune_status()
    if (empty(st)) {
        delete st
        return
    }
    var tuned = 0
    for (s in st) {
        tuned += s.source == "manifest" ? 1 : 0
    }
    to_log(LOG_INFO, "{appName}: tune status ({tuned}/{length(st)} kernels tuned for this box):\n")
    for (s in st) {
        to_log(LOG_INFO, "  {s.fname}: {empty(s.suffix) ? "reference" : s.suffix} ({s.source})\n")
    }
    if (tuned < length(st)) {
        to_log(LOG_INFO, "{appName}: run with --tune to tune the remaining kernels for this box\n")
    }
    delete st
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
    let joined = join(fnames, ";")
    delete wanted
    delete fnames
    return joined
}

//! One `[tune_scope]`'s identity + completeness, as reported by `tune_scopes_status()` —
//! the macro-time surface build orchestrators (the `-exe` release-deps JSON, daspkg) read
//! to decide whether to run tuners before a rebuild.
struct TuneScopeStatus {
    name : string       //!< [tune_scope] name
    manifest : string   //!< the app sidecar the winners live in
    tuner : string      //!< tuner harness path ("" = none declared)
    complete : bool     //!< every [tune] kernel of this scope has a fresh sidecar entry
    fallbacks : string  //!< ';'-joined fname=suffix stamps selected by this compile/host
}

    //! Every declared scope in `prog` with its per-key completeness against the app sidecar,
    //! read OFF THE AST, not this module's macro globals — those are per-requiring-context
    //! copies, so a caller in another macro module reads an empty bank. Deduped by name.
def tune_scopes_status(prog : Program?) : array<TuneScopeStatus> {
    // pass 1: the declared scopes (name, tuner, covers) off the [tune_scope] annotations
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
                    if (!empty(sc.scopeName) && !(seen |> key_exists(sc.scopeName))) {
                        seen |> insert(sc.scopeName)
                        found |> push(sc)
                    }
                }
            }
        }
    }
    // pass 2: each scope's demanded kernels across its declaring + covers= modules
    var rows : array<TuneScopeStatus>
    rows |> reserve(length(found))
    let mpath = tune_manifest_path()
    var compileStatus <- collect_status(prog)
    for (sc in found) {
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
        var fallbacks : array<string>
        for (fname in fnames) {
            for (st in compileStatus) {
                if (st.fname == fname && st.source != "manifest") {
                    fallbacks |> push("{fname}={st.suffix}")
                    break
                }
            }
        }
        rows |> push(TuneScopeStatus(name = sc.scopeName, manifest = mpath,
            tuner = sc.tunerPath,
            complete = tune_sidecar_entries_complete(mpath, join(fnames, ";")),
            fallbacks = join(fallbacks, ";")))
        delete wanted
        delete fnames
        delete fallbacks
    }
    delete compileStatus
    delete found
    delete seen
    return <- rows
}

//! The demanded kernels in the ';'-joined `fnames` that LACK a sidecar entry — "" when
//! complete. The loud half of the completeness check: a tuner that doesn't record a kernel
//! it should re-tunes every start, and this names the drift so it's fixed in one glance.
def tune_sidecar_missing_entries(path : string; fnames : string) : string {
    var found = false
    var manifest <- read_manifest(path, found)
    var missing <- [for (fname in split(fnames, ";"));
        fname;
        where !empty(fname) && !(found && manifest |> key_exists(fname))]
    delete manifest
    let r = join(missing, ";")
    delete missing
    return r
}

    //! Every kernel in the ';'-joined `fnames` has a `"kernels"` entry in the sidecar at `path`
    //! (companions ride their main entry; "reference" counts). Missing/stale/unparsable means no
    //! entries. This is the "is this scope tuned" predicate — file existence proves nothing.
def tune_sidecar_entries_complete(path : string; fnames : string) : bool {
    var found = false
    var manifest <- read_manifest(path, found)
    var complete = found || empty(fnames)
    if (found) {
        for (fname in split(fnames, ";")) {
            if (!empty(fname)) {
                complete &&= manifest |> key_exists(fname)
            }
        }
    }
    delete manifest
    return complete
}

// ===== the perm grid =====

struct private TunePerm {
    suffix : string
    requires : string   // fallback-eligibility hint: "a,b" = AND, "a|b" = OR per element; "" = everywhere
    args : array<tuple<argname : string; argvalue : RttiValue>>
}

    // Host cpuid OR'd with the DAS_JIT_*_FORCE_FEATURES override — keep in lockstep with
    // llvm_jit_common's x64_target_feature. requires= is an ELIGIBILITY hint for fallback
    // selection only; the generator's own decline predicate stays the emission truth.
def private fallback_feature_ok(name : string) : bool {
    if (cpu_supports(name)) return true
    for (part in split(get_env_variable("DAS_JIT_X64_FORCE_FEATURES"), ",")) {
        if (part == name) return true
    }
    for (part in split(get_env_variable("DAS_JIT_ARM64_FORCE_FEATURES"), ",")) {
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
def private stamp_llvm_code(var fn : FunctionPtr; genKey : string; perm : TunePerm; from : string) {
    var aargs : array<tuple<argname : string; argvalue : RttiValue>>
    aargs |> reserve(length(perm.args) + 3)
    aargs |> push((argname = "name", argvalue = RttiValue(tString = genKey)))
    for (a in perm.args) {
        aargs |> push((argname = a.argname, argvalue = a.argvalue))
    }
    aargs |> push((argname = "tune_suffix", argvalue = RttiValue(tString = perm.suffix)))
    aargs |> push((argname = "tune_from", argvalue = RttiValue(tString = from)))
    fn |> append_annotation("llvm_code", "llvm_code", aargs)
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
        stamp_llvm_code(fclone, genKey, p, "")
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
            stamp_llvm_code(cclone, c.genkey, p, "")
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

// ===== the annotations =====

    //! `[tune_perm(<generator args>)]` — one grid row for the `tune(...)` that must follow it in
    //! the same bracket. Args pass to the generator verbatim and fold into the JIT DLL cache
    //! key; `suffix="..."` overrides the auto-derived variant name.
[function_macro(name = "tune_perm")]
class private TunePermAnnotation : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        g_perm_rows[perm_key(func)] |> emplace(build_perm_row(args))
        return true
    }
}

    //! `[tune_companion(fn = "sibling", gen = "generator_key")]` — stamps `sibling` with the SAME
    //! perm from the same sidecar entry, so a kernel and (say) its repack-layout query can never
    //! desync. List these after the [tune_perm] grid and before tune(...). See `skills/tune.md`.
[function_macro(name = "tune_companion")]
class private TuneCompanionAnnotation : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
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
}

    //! `[tune(gen="key", fallback="suffix")]` — LAST in a bracket of `[tune_perm]` rows, which it
    //! consumes. Normal runs stamp one winner (sidecar > fallback > reference); tune/test runs
    //! stamp the whole grid plus a `<name>_variants()` registry. Mode contract: module docstring.
[function_macro(name = "tune")]
class private TuneAnnotation : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup; // nolint:STYLE037,STYLE038 — the [tune] resolution ladder: manifest/fallback/reference arms in precedence order
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
        if (mode != "normal") {
            errors := "[tune] on {func.name}: unknown DAS_TUNE_MODE '{mode}' (normal | tune | test)"
            delete perms
            delete comps
            return false
        }
    // no entry falls back rather than dropping to reference, so a sidecar written before a kernel family landed cannot untune it
        let fname = "{func.name}"
        var manifestFound = false
        let mpath = tune_manifest_path()
        var manifest <- read_manifest(mpath, manifestFound)
        var suffix = ""
        if (manifestFound) {
            suffix = manifest?[fname] ?? ""
        }
        let fromManifest = !empty(suffix)
        var ok = true
        if (empty(suffix)) {
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
        if (ok && !empty(suffix) && suffix != "reference") {
            ok = false
            for (p in perms) {
                if (p.suffix == suffix) {
                    stamp_llvm_code(func, genKey, p, fromManifest ? mpath : "")
                    for (c in comps) {
                        stamp_llvm_code(c.fn, c.genkey, p, fromManifest ? mpath : "")
                    }
                    if (tune_verbosity() == "verbose") {
                        print("llvm_tune: {fname} <- {suffix} ({fromManifest ? mpath : "fallback"})\n")
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
        delete perms
        delete comps
        return ok
    }
}

// ===== [tune_scope] / [tune_policy] — the tuner-wiring UX rail =====

def private resolve_decl_path(declFile : string; p : string) : string {
    return is_absolute(p) ? p : path_join(dir_name(declFile), p)
}

    //! `[tune_scope(name = "mylib", tuner = "harness.das")]` on a dummy struct in the library that
    //! owns the `[tune]` functions, declared ABOVE them: names the tuner that regenerates this
    //! library's winners in the ONE per-app sidecar. `tuner=` resolves against the declaring file.
[structure_macro(name = "tune_scope")]
class private TuneScopeAnnotation : AstStructureAnnotation {
    def override apply(var st : StructurePtr; var group : ModuleGroup;
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
        // overwrite, not error: shared-module macro state persists across compiles in one process
        g_scopes["{compiling_module().name}"] = TuneScope(scopeName = sname,
            modName = "{compiling_module().name}", covers = covers,
            manifestPath = tune_manifest_path(), tunerPath = tuner)
        return true
    }
}

// The tune flags, parsed off the APP's command line — we share an argv we do not own, so this
// is the two-arg parse_args form (clargs skips unknown flag-shaped tokens). Keep every field a
// bool and never read positionals: an app's own `--model foo.gguf` leaves `foo.gguf` in there.
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
}

// Adopts --tune-quiet/--tune-verbose and answers whether --tune was passed. Runs at MACRO
// time, so the verbosity has to travel by env: the runtime guard is a different context and
// would not see this context's global. get_user_args also gets the standalone-exe slice right.
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
    return cfg.tune
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
        let v = get_env_variable("DAS_TUNE_VERBOSITY")
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

//! Enter a kernel: `total` is its own round budget, the inner counter's denominator.
def tune_progress_kernel_begin(name : string; total : int) {
    emit_event("begin", "name={name} total={total}")
}

//! One round of the current kernel. `phase` names the pass (`screening` / `narrowing` /
//! `finalists`) and `live` is how many grid rows are still in contention.
def tune_progress_kernel_step(i : int; phase : string; live : int) {
    emit_event("step", "i={i} phase={phase} live={live}")
}

//! Leave a kernel. `verdict` is `holds`, `beats`, `rejected` or `skipped`. Every `kernel_begin`
//! needs one, or the outer counter never reaches its plan.
def tune_progress_kernel_end(name, winner, verdict : string) {
    emit_event("end", "name={name} winner={winner} verdict={verdict}")
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
def private announce_tuning(scopeName : string; force, selfRestarts : bool) {
    if (tune_verbosity() == "silent") return
    let why = force ? "re-tuning (--tune)" : "is untuned on this box"
    let tail = (selfRestarts
        ? "the process restarts itself when it is done"
        : "the process exits when it is done, and must be restarted to pick up the winners")
    print("llvm_tune: scope '{scopeName}' {why} - tuning now. This takes a while; {tail}.\n")
}

// spawn the scope's tuner, relaying its output LIVE — RUNTIME only (the auto and restart
// guards); never from macro context. Streaming is load-bearing: a tune is minutes of work, and
// buffering it to a file left us and any supervisor with no signal until the child exited.
def private run_scope_tuner(scope : TuneScope) : bool {
    let cmd = "{tuner_cmd(scope)} 2>&1"
    if (tune_verbosity() != "silent") {
        print("llvm_tune: tuning scope '{scope.scopeName}' -> {scope.manifestPath}\n")
    }
    if (tune_verbosity() == "verbose") {
        print("llvm_tune: {cmd}\n")
    }
    var rc : int
    unsafe {
        rc = popen(cmd) $(f) {
            if (f != null) {
                while (!feof(f)) {
                    relay_line(fgets(f))
                }
            }
        }
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

    //! Runtime half of `[tune_policy(missing = "auto")]` / `--tune`, injected at the top of the
    //! app's `main` and followed by `tune_auto_reexec()`. Tuning happens at RUNTIME because the
    //! winners are compile-time stamps. An empty or corrupt sidecar reads as untuned, so it self-heals.
def tune_auto_prepare(scopeName : string; manifestPath : string; tunerPath : string; force : bool; fnames : string) {
    if (!force && tune_sidecar_entries_complete(manifestPath, fnames)) {
        return
    }
    announce_tuning(scopeName, force, true)
    run_scope_tuner(TuneScope(scopeName = scopeName, manifestPath = manifestPath, tunerPath = tunerPath))
    if (tune_sidecar_entries_complete(manifestPath, fnames)) {
        g_tuned_scopes++
    } else {
        print("llvm_tune: tuner left scope '{scopeName}' incomplete in {manifestPath} - missing: {tune_sidecar_missing_entries(manifestPath, fnames)}\n")
    }
}

    //! Relaunch the process so a fresh compile stamps the just-written manifests — they cannot be
    //! adopted mid-run. `--tune` is stripped from the relaunch so the child converges. True means
    //! the child ran and main must return `tune_reexec_code()` immediately.
def tune_auto_reexec() : bool {
    if (g_tuned_scopes == 0) {
        return false
    }
    // aliases a process-global Array header — do not delete
    var argv <- get_command_line_arguments()
    var args : array<string>
    args |> reserve(length(argv))
    for (a in argv) {
        if (a != "--tune") {
            args |> push("{a}")
        }
    }
    print("llvm_tune: re-launching to apply the fresh manifests\n")
    var rc : int
    unsafe {
        rc = popen_argv(args, 0.0) $(f) {
            if (f != null) {
                while (!feof(f)) {
                    print(fgets(f))
                }
            }
        }
    }
    delete args
    g_reexec_code = rc
    return true
}

//! The re-exec'd child's exit code — what an `int` main returns after `tune_auto_reexec()`.
def tune_reexec_code() : int {
    return g_reexec_code
}

    //! Runtime half of `[tune_policy(missing = "restart")]`, injected as a guard at the top of the
    //! app's `main`. Runs the tuner, prints the restart banner, and returns true so main returns
    //! immediately (the winners are compile-time stamps). False = scope complete, carry on.
def tune_restart_needed(scopeName : string; manifestPath : string; tunerPath : string; fnames : string) : bool {
    if (tune_sidecar_entries_complete(manifestPath, fnames)) {
        return false
    }
    if (empty(tunerPath)) {
        print("llvm_tune: scope '{scopeName}' is untuned on this box\n")
        print("llvm_tune: no tuner declared - produce {manifestPath} with the library's tuner, then restart\n")
        return true
    }
    announce_tuning(scopeName, false, false)
    run_scope_tuner(TuneScope(scopeName = scopeName, manifestPath = manifestPath, tunerPath = tunerPath))
    if (tune_sidecar_entries_complete(manifestPath, fnames)) {
        print("llvm_tune: scope '{scopeName}' tuned -> {manifestPath}\nllvm_tune: restart to apply the winners\n")
    } else {
        print("llvm_tune: tuner left scope '{scopeName}' incomplete in {manifestPath} - missing: {tune_sidecar_missing_entries(manifestPath, fnames)}\n")
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
            for (ann in fn.annotations) {
                if (ann.annotation.name == "tune") {
                    isTuned = true
                } elif (ann.annotation.name == "llvm_code") {
                    let sv = find_arg(ann.arguments, "tune_suffix")
                    if (sv is tString) { suffix = sv as tString }
                    let fv = find_arg(ann.arguments, "tune_from")
                    if (fv is tString) { from = fv as tString }
                }
            }
            if (isTuned) {
                let source = empty(suffix) ? "reference" : (empty(from) ? "fallback" : "manifest")
                let mpath = !empty(from) ? from : sidecar
                rows |> push(TuneStatus(fname = "{fn.name}", scope = scopeName,
                    suffix = suffix, source = source, manifestPath = mpath))
            }
        }
    }
    return <- rows
}

    // AOT runs no policy; grid-mode children manage their own manifest (also the recursion guard);
    // a DAS_TUNE_MANIFEST override means manual. -exe is NOT here — it runs no policy either but
    // still owes the status [init], so its call sites re-admit it via exe_status_report().
def private tune_policy_gated() : bool {
    return (tune_stamp_free_gate() || tune_mode() != "normal"
        || !empty(get_env_variable("DAS_TUNE_MANIFEST")))
}

    // -exe owes the status [init] even under a DAS_TUNE_MANIFEST redirect (report ≠ policy —
    // else a redirect-built exe ships stamp-less rows); stamp-free/grid still win
def private exe_status_report() : bool {
    return tune_exe_gate() && !tune_stamp_free_gate() && tune_mode() == "normal"
}

// the runtime status table: one tune_status_add per [tune] function, off the stamped AST
// truth — emitted by policy runs AND by -exe builds (an artifact must be able to report
// which perms it carries: log_tune_status / tune_status work inside a standalone exe)
def private emit_status_init(var errors : string&) {
    var rows <- collect_status(get_ptr(compiling_program()))
    if (!empty(rows)) {
        var body : array<ExpressionPtr>
        body |> reserve(length(rows))
        for (row in rows) {
            body |> push <| qmacro_expr() {
                tune_status_add($v(row.fname), $v(row.scope), $v(row.suffix),
                    $v(row.source), $v(row.manifestPath))
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
        let penv = get_env_variable("DAS_TUNE_POLICY")
        if (!empty(penv)) {
            policy = penv
        }
        if (policy != "fallback" && policy != "warn" && policy != "error" && policy != "auto" && policy != "restart") {
            errors = "[tune_policy] on {func.name}: missing=\"{policy}\" - use fallback | warn | error | auto | restart"
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
            continue if (!force && tune_sidecar_entries_complete(scope.manifestPath, scope_fnames_joined(scope)))
            if (force || policy == "auto") {
    // a runtime guard tunes and RE-EXECS: restamping a required module's compiled function is a cross-module mutation re-infer cannot survive
                if (empty(scope.tunerPath)) {
                    errors = "[tune_policy]: scope '{scope.scopeName}' declares no tuner= - cannot {force ? "--tune" : "auto-tune"}; produce {scope.manifestPath} manually"
                } else {
                    autoScopes |> push(scope)
                }
            } elif (policy == "error") {
                errors = "[tune_policy]: scope '{scope.scopeName}' is untuned on this box - run: {empty(scope.tunerPath) ? "<no tuner declared>" : tuner_cmd(scope)} (or start with --tune)"
            } elif (policy == "warn") {
                print("llvm_tune: WARNING scope '{scope.scopeName}' is untuned on this box (fallback stamps in effect)\nllvm_tune: run: {empty(scope.tunerPath) ? "<no tuner declared>" : tuner_cmd(scope)} (or start with --tune)\n")
            } elif (policy == "restart") {
    // graceful guard, not exit(): a hard exit from an [init] skips environment shutdown
                if (!(func.body is ExprBlock)) {
                    errors = "[tune_policy]: restart - {func.name} has no body block"
                } elif (func.result.baseType != Type.tVoid && func.result.baseType != Type.tInt
                        && func.result.baseType != Type.autoinfer) {
                    // autoinfer = undeclared result, void by convention for main
                    errors = "[tune_policy]: restart - {func.name} must return void or int"
                } else {
                    let fnames = scope_fnames_joined(scope)
                    var guard : ExpressionPtr
                    if (func.result.baseType == Type.tInt) {
                        guard = qmacro_expr() {
                            if (tune_restart_needed($v(scope.scopeName), $v(scope.manifestPath), $v(scope.tunerPath), $v(fnames))) {
                                return 3
                            }
                        }
                    } else {
                        guard = qmacro_expr() {
                            if (tune_restart_needed($v(scope.scopeName), $v(scope.manifestPath), $v(scope.tunerPath), $v(fnames))) {
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
                    guards |> push <| qmacro_expr() {
                        tune_auto_prepare($v(scope.scopeName), $v(scope.manifestPath), $v(scope.tunerPath), $v(force), $v(fnames))
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
[function_macro(name = "tune_policy")]
class private TunePolicyAnnotation : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
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
}

// inert once-only marker the TuneDefaultPolicy pass stamps onto main — appending an
// annotation RUNS its apply (add_function_annotation), so the marker must not be
// [tune_policy] itself (that would re-enter the policy with whatever env is set)
[function_macro(name = "tune_policy_default_applied")]
class private TunePolicyDefaultMarker : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        return true
    }
}

    //! The "untuned does not start" default: a program root with a `main`, some `[tune_scope]` in
    //! play and no declared `[tune_policy]` behaves as `missing = "auto"`. Escapes and the
    //! once-only marker: `skills/tune.md`. A root without `main` never tunes (dastest files).
[infer_macro]
class private TuneDefaultPolicy : AstPassMacro {
    def override apply(prog : ProgramPtr; _mod : Module?) : bool {
    // fires only for roots that SEE llvm_tune, so a scope-owning library must re-export it public
        return false if (is_in_completion() || !empty("{compiling_module().name}")
            || empty(g_scopes) || (tune_policy_gated() && !exe_status_report()))
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
            ok = apply_tune_policy(main_fn, "auto", errs)
        }
        if (!ok || !empty(errs)) {
    // macro_error mid-infer-pass is dropped by the retry loop — print + panic actually stops it
            print("llvm_tune: {errs}\n")
            panic("llvm_tune default policy: {errs}")
        }
        return true   // modified — re-run infer over the injected status/guard
    }
}
