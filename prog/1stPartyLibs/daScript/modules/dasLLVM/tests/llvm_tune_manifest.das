options gen2

require dastest/testing_boost
require daslib/module_path
require daslib/strings_boost
require daslib/fio
require strings
require daslib/json
require daslib/json_boost
require llvm/daslib/llvm_tune   // tune_box_identity — the applied_box adoption arm

// Per-app sidecar round-trip, declaration-free: copy the client template into a temp
// directory (the sidecar resolves beside the ROOT SCRIPT — <client>.tune.json), spawn it
// once with "write" — tune_manifest_set targets that sidecar and records k2 — then spawn it
// again: the fresh compile stamps man_add from the sidecar, so RESULT moves from the
// fallback k1 tier (6) to the k2 tier (7). A third arm back-dates the sidecar older than
// the daslang binary: STALE reads as absent, so the stamps drop back to fallback. Every child
// runs -no-module-cache: the back-date simulates a rebuild, and a real rebuild re-keys the
// default cache, while an identical-content back-date alone would be a cache hit.

def private spawn_client(cmd : string; var lines : array<string>) : int {
    // verbose: the `llvm_tune: <fn> <- <perm>` stamp lines this test reads are the stamped-truth
    // proof, and they are verbose-only now (one per kernel is a wall in front of a real app)
    let env = (get_platform_name() == "windows"
        ? "set DAS_TUNE_VERBOSITY=verbose&& "
        : "DAS_TUNE_VERBOSITY=verbose ")
    // cmd.exe /c strips the first and last quote of a line that starts with a quote — but the env
    // prefix now leads, so the line never starts with a quote and no sacrificial wrap is needed
    let full = "{env}{cmd}"
    let rc = unsafe(popen_timeout(full, 300.0) $(f) {
        if (f == null) {
            return
        }
        while (!feof(f)) {
            let ln = fgets(f)
            if (ln |> starts_with("WRITE ") || ln |> starts_with("RESULT ") || ln |> starts_with("KV ")
                    || ln |> starts_with("OTHER ") || ln |> starts_with("REF ") || ln |> starts_with("llvm_tune:")) {
                lines |> push(strip("{ln}"))
            }
        }
    })
    return rc
}

[test]
def test_llvm_tune_manifest_roundtrip(t : T?) {   // nolint:STYLE038 - flat sequential sidecar-state arms over shared child-spawn state; no arm stands alone
    if (!jit_enabled()) {
        // the generator only fires under the LLVM JIT; the child needs -jit too
        t |> success(true)
        return
    }
    let args <- get_command_line_arguments()  // locked view — do not delete
    let bin = args[0]
    let tmpRes = create_temp_file_result("llvm_tune_manifest_", ".das")
    if (tmpRes is error) {
        t |> failure("could not create a temp file: {tmpRes as error}")
        return
    }
    let clientPath = tmpRes as value
    let tmplText = fread("{get_this_module_dir()}/llvm_tune_manifest_client.das")
    t |> success(!empty(tmplText))
    t |> success(fwrite(clientPath, tmplText))
    // the per-app sidecar: <root-script-stem>.tune.json beside the spawned script
    let sidecarPath = "{slice(clientPath, 0, length(clientPath) - 4)}.tune.json"

    var wlines : array<string>
    let rcw = spawn_client("\"{bin}\" -no-module-cache -jit -jit-no-cache \"{clientPath}\" -- write", wlines)
    t |> equal(rcw, 0)
    var wrote = false
    for (ln in wlines) {
        wrote ||= ln |> starts_with("WRITE ok")
    }
    t |> success(wrote)
    t |> success(!empty(fread(sidecarPath)))

    var rlines : array<string>
    let rcr = spawn_client("\"{bin}\" -no-module-cache -jit -jit-no-cache \"{clientPath}\"", rlines)
    t |> equal(rcr, 0)
    var stamped = false
    var result7 = false
    var kv2 = false
    var other6 = false
    var ref5 = false
    for (ln in rlines) {
        stamped ||= ln |> starts_with("llvm_tune: man_add <- k2")
        result7 ||= ln == "RESULT 7"
        kv2 ||= ln == "KV 2"
        other6 ||= ln == "OTHER 6"
        ref5 ||= ln == "REF 5"
    }
    t |> success(stamped)  // the sidecar entry stamped at [tune] time
    t |> success(result7)  // and the k2 tier actually ran
    t |> success(kv2)      // and the companion was stamped from the same entry
    t |> success(other6)   // no sidecar entry -> the fallback stamp (k1) stands
    t |> success(ref5)     // explicit "reference" entry -> the original body despite fallback

    // the companion's OWN sidecar entry (the gemv-seat rail): man_kv stamps from it while man_add keeps k2
    var olines : array<string>
    let rco = spawn_client("\"{bin}\" -no-module-cache -jit -jit-no-cache \"{clientPath}\" -- write-own", olines)
    t |> equal(rco, 0)
    var ownLines : array<string>
    let rcor = spawn_client("\"{bin}\" -no-module-cache -jit -jit-no-cache \"{clientPath}\"", ownLines)
    t |> equal(rcor, 0)
    var ownResult7 = false
    var ownKv1 = false
    for (ln in ownLines) {
        ownResult7 ||= ln == "RESULT 7"
        ownKv1 ||= ln == "KV 1"
    }
    t |> success(ownResult7, "the main entry still stamps k2")
    t |> success(ownKv1, "the companion stamps from its own entry")

    // STALE sidecar (older than the daslang binary) reads as absent: back to fallback tier
    let backRes = set_mtime_result(sidecarPath, mktime(2000, 1, 1, 0, 0, 0))
    t |> success(!(backRes is error), "could not back-date the sidecar")
    var slines : array<string>
    let rcs = spawn_client("\"{bin}\" -no-module-cache -jit -jit-no-cache \"{clientPath}\"", slines)
    t |> equal(rcs, 0)
    var staleStampedK2 = false
    var staleResult6 = false
    for (ln in slines) {
        staleStampedK2 ||= ln |> starts_with("llvm_tune: man_add <- k2")
        staleResult6 ||= ln == "RESULT 6"
    }
    t |> success(!staleStampedK2)  // stale winners must not stamp
    t |> success(staleResult6)     // fallback k1 tier: 2+3+1

    // BOX-IDENTITY gate: a sidecar written on another box (foreign provenance.box) reads as
    // stale even when fresher than the binary — the cross-box-copy hazard. Rewrite the file
    // with the box swapped, touch it fresh, and the client must fall back again.
    let sidecarText = fread(sidecarPath)
    t |> success(find(sidecarText, "\"box\"") >= 0, "provenance carries the box identity stamp")
    var jerr = ""
    var jdoc = read_json(sidecarText, jerr)
    t |> success(jdoc != null, "sidecar parses: {jerr}")
    // asserted on the parsed value, not the JSON text: a Windows home is escaped there
    let recorded_binary = "{jdoc?["provenance"]?["binary"] ?? ""}"
    t |> success(!empty(recorded_binary) && tilde_home(recorded_binary) == recorded_binary, "a minted sidecar names no home directory - the binary path is spelled ~: {recorded_binary}")
    var prov = jdoc?["provenance"]
    if (prov != null) {
        update(prov, "box", JV("some-other-box|foreign-arch|foreign-hw"))
    }
    fwrite(sidecarPath, write_json(jdoc))
    delete_json(jdoc)
    var flines : array<string>
    let rcf = spawn_client("\"{bin}\" -no-module-cache -jit -jit-no-cache \"{clientPath}\"", flines)
    t |> equal(rcf, 0)
    var foreignStampedK2 = false
    var foreignResult6 = false
    for (ln in flines) {
        foreignStampedK2 ||= ln |> starts_with("llvm_tune: man_add <- k2")
        foreignResult6 ||= ln == "RESULT 6"
    }
    t |> success(!foreignStampedK2)  // foreign-box winners must not stamp
    t |> success(foreignResult6)     // fallback tier again
    delete flines

    // APPLIED-BOX adoption: the same foreign sidecar plus provenance.applied_box naming THIS
    // box (what a scope resolver stamps when adopting a sibling box's mint) must stamp again,
    // while provenance.box keeps naming the box that measured it.
    var jerr2 = ""
    var jdoc2 = read_json(fread(sidecarPath), jerr2)
    t |> success(jdoc2 != null, "foreign sidecar parses: {jerr2}")
    var prov2 = jdoc2?["provenance"]
    if (prov2 != null) {
        update(prov2, "applied_box", JV(tune_box_identity()))
    }
    fwrite(sidecarPath, write_json(jdoc2))
    delete_json(jdoc2)
    var alines : array<string>
    let rca = spawn_client("\"{bin}\" -no-module-cache -jit -jit-no-cache \"{clientPath}\"", alines)
    t |> equal(rca, 0)
    var adoptedStampedK2 = false
    var adoptedResult7 = false
    for (ln in alines) {
        adoptedStampedK2 ||= ln |> starts_with("llvm_tune: man_add <- k2")
        adoptedResult7 ||= ln == "RESULT 7"
    }
    t |> success(adoptedStampedK2)  // applied_box naming this box re-admits the winners
    t |> success(adoptedResult7)
    delete alines

    remove(sidecarPath)
    remove(clientPath)
    delete wlines
    delete rlines
    delete slines
}

// tune_sidecar_verdict: every refusal reason, in-process (the verdict is plain runtime code).
// The boolean predicate must agree with the verdict on every case — the alignment invariant.
[test]
def test_llvm_tune_sidecar_verdict(t : T?) {
    let tmpRes = create_temp_file_result("llvm_tune_verdict_", ".tune.json")
    if (tmpRes is error) {
        t |> failure("could not create a temp file: {tmpRes as error}")
        return
    }
    let path = tmpRes as value
    let ourBox = tune_box_identity()
    t |> success(fwrite(path, "\{\"kernels\":\{\"a\":\"k1\",\"b\":\"k2\"\},\"provenance\":\{\"box\":\"{ourBox}\",\"myver\":\"2\"\}\}"))

    var v = tune_sidecar_verdict("{path}.nope", "a")
    t |> equal(v.reason, TuneSidecarReason.absent)
    v = tune_sidecar_verdict(path, "a;b")
    t |> success(v.complete)
    v = tune_sidecar_verdict(path, "a;b;c")
    t |> equal(v.reason, TuneSidecarReason.missing)
    t |> success(find(v.detail, "c") >= 0)
    v = tune_sidecar_verdict(path, "")   // empty demand is always satisfied
    t |> success(v.complete)
    v = tune_sidecar_verdict(path, "a;;b")   // empty segments are skipped, not demanded
    t |> success(v.complete)

    // the @version: pin token rides the demand list like a kernel name
    v = tune_sidecar_verdict(path, "a;@version:myver=2")
    t |> success(v.complete)
    v = tune_sidecar_verdict(path, "a;@version:myver=3")
    t |> equal(v.reason, TuneSidecarReason.version)
    t |> success(find(v.detail, "myver 2") >= 0)
    v = tune_sidecar_verdict(path, "a;@version:nover=1")
    t |> equal(v.reason, TuneSidecarReason.version)
    t |> success(find(v.detail, "records no nover") >= 0)
    v = tune_sidecar_verdict(path, "a;@version:noequals")   // malformed token never blocks
    t |> success(v.complete)
    t |> equal(tune_sidecar_entries_complete(path, "a;@version:myver=2"), true)
    t |> equal(tune_sidecar_entries_complete(path, "a;@version:myver=3"), false)
    // the bare-list API carries the unsatisfied pin token verbatim beside missing kernels
    t |> equal(tune_sidecar_missing_entries(path, "a;zzz;@version:myver=3"), "zzz;@version:myver=3")

    // a numeric provenance value (hand-edited sidecar) reads equal to its string form
    t |> success(fwrite(path, "\{\"kernels\":\{\"a\":\"k1\"\},\"provenance\":\{\"box\":\"{ourBox}\",\"myver\":2\}\}"))
    t |> success(!(set_mtime_result(path, mktime(2032, 1, 1, 0, 0, 0)) is error))
    v = tune_sidecar_verdict(path, "a;@version:myver=2")
    t |> success(v.complete)

    // stale beats everything else: back-dated under the binary, the same complete file refuses
    t |> success(!(set_mtime_result(path, mktime(2000, 1, 1, 0, 0, 0)) is error))
    v = tune_sidecar_verdict(path, "a")
    t |> equal(v.reason, TuneSidecarReason.stale_binary)
    t |> equal(tune_sidecar_entries_complete(path, "a"), false)

    // foreign box: fresh mtime, another box's provenance (distinct mtimes defeat the
    // per-path+mtime box cache — same-second rewrites would reuse the first observation)
    t |> success(fwrite(path, "\{\"kernels\":\{\"a\":\"k1\"\},\"provenance\":\{\"box\":\"some|other|box|id|entirely\"\}\}"))
    t |> success(!(set_mtime_result(path, mktime(2033, 1, 1, 0, 0, 0)) is error))
    v = tune_sidecar_verdict(path, "a")
    t |> equal(v.reason, TuneSidecarReason.foreign_box)
    t |> success(find(v.detail, "some|other|box|id|entirely") >= 0)

    // garbage and provenance-less files read unreadable, not foreign-box-of-nothing
    t |> success(fwrite(path, "not json at all"))
    t |> success(!(set_mtime_result(path, mktime(2034, 1, 1, 0, 0, 0)) is error))
    v = tune_sidecar_verdict(path, "a")
    t |> equal(v.reason, TuneSidecarReason.unreadable)
    t |> equal(tune_sidecar_entries_complete(path, "a"), false)

    // the race twin: rewrite the file unparsable at the SAME mtime as a good read — the box
    // cache still answers for (path, mtime), so the manifest read is what discovers the change
    t |> success(fwrite(path, "\{\"kernels\":\{\"a\":\"k1\"\},\"provenance\":\{\"box\":\"{ourBox}\"\}\}"))
    t |> success(!(set_mtime_result(path, mktime(2035, 1, 1, 0, 0, 0)) is error))
    v = tune_sidecar_verdict(path, "a")
    t |> success(v.complete)
    t |> success(fwrite(path, "not json either"))
    t |> success(!(set_mtime_result(path, mktime(2035, 1, 1, 0, 0, 0)) is error))
    v = tune_sidecar_verdict(path, "a")
    t |> equal(v.reason, TuneSidecarReason.unreadable)

    remove(path)
}
