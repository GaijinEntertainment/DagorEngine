options gen2

require dastest/testing_boost
require daslib/module_path
require daslib/strings_boost
require daslib/fio
require strings
require daslib/json
require daslib/json_boost

// Per-app sidecar round-trip, declaration-free: copy the client template into a temp
// directory (the sidecar resolves beside the ROOT SCRIPT — <client>.tune.json), spawn it
// once with "write" — tune_manifest_set targets that sidecar and records k2 — then spawn it
// again: the fresh compile stamps man_add from the sidecar, so RESULT moves from the
// fallback k1 tier (6) to the k2 tier (7). A third arm back-dates the sidecar older than
// the daslang binary: STALE reads as absent, so the stamps drop back to fallback.

def private spawn_client(cmd : string; var lines : array<string>) : int {
    // verbose: the `llvm_tune: <fn> <- <perm>` stamp lines this test reads are the stamped-truth
    // proof, and they are verbose-only now (one per kernel is a wall in front of a real app)
    let env = (get_platform_name() == "windows"
        ? "set DAS_TUNE_VERBOSITY=verbose&& "
        : "DAS_TUNE_VERBOSITY=verbose ")
    // cmd.exe /c strips the first and last quote of a line that starts with a quote — but the env
    // prefix now leads, so the line never starts with a quote and no sacrificial wrap is needed
    let full = "{env}{cmd}"
    var rc : int
    unsafe {
        rc = popen_timeout(full, 300.0) $(f) {
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
        }
    }
    return rc
}

[test]
def test_llvm_tune_manifest_roundtrip(t : T?) {
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
    let rcw = spawn_client("\"{bin}\" -jit \"{clientPath}\" -- write", wlines)
    t |> equal(rcw, 0)
    var wrote = false
    for (ln in wlines) {
        wrote ||= ln |> starts_with("WRITE ok")
    }
    t |> success(wrote)
    t |> success(!empty(fread(sidecarPath)))

    var rlines : array<string>
    let rcr = spawn_client("\"{bin}\" -jit \"{clientPath}\"", rlines)
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

    // STALE sidecar (older than the daslang binary) reads as absent: back to fallback tier
    let backRes = set_mtime_result(sidecarPath, mktime(2000, 1, 1, 0, 0, 0))
    t |> success(!(backRes is error), "could not back-date the sidecar")
    var slines : array<string>
    let rcs = spawn_client("\"{bin}\" -jit \"{clientPath}\"", slines)
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
    unsafe {
        var prov = jdoc?["provenance"]
        if (prov != null) {
            update(prov, "box", JV("some-other-box|foreign-arch|foreign-hw"))
        }
    }
    fwrite(sidecarPath, write_json(jdoc))
    delete_json(jdoc)
    var flines : array<string>
    let rcf = spawn_client("\"{bin}\" -jit \"{clientPath}\"", flines)
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

    remove(sidecarPath)
    remove(clientPath)
    delete wlines
    delete rlines
    delete slines
}
