options gen2

require dastest/testing_boost
require daslib/module_path
require daslib/strings_boost
require daslib/fio
require strings
require _spawn_common
require daslib/json
require daslib/json_boost

// [tune_scope]/[tune_policy]/--tune/tune_status round-trip through child daslang processes
// (the policy rail is compile-time). Two libraries each ship a scope + tuner; both write the
// ONE per-app sidecar — <client>.tune.json beside the spawned client script (the per-app
// location IS the feature) — via the upsert API, so the second tuner must preserve the first
// scope's keys. Per-scope "tuned" is KEY completeness, not file existence: after tuner 1
// writes the sidecar, scope 2 must still tune. Children mostly run interp (stamp prints and
// STATUS lines are mode-independent); only the auto flow runs -jit to prove the restamped
// tier executes (RESULT 7 = 2+3+k2). Every child runs -no-module-cache: these arms assert
// what a FRESH compile stamps and prints; the cache-facing rail is llvm_tune_profiles.das.

def private tune_env(policy : string) : string {
    // verbose: the per-kernel `<fn> <- <perm>` stamp lines this test asserts on are the
    // stamped-truth proof, and they are verbose-only now (34 of them is a wall before an app starts)
    return (get_platform_name() == "windows" ?
        "set DAS_TUNE_MODE=&& set DAS_TUNE_MANIFEST=&& set DAS_TUNE_VERBOSITY=verbose&& set DAS_TUNE_POLICY={policy}&& " :
        "DAS_TUNE_MODE= DAS_TUNE_MANIFEST= DAS_TUNE_VERBOSITY=verbose DAS_TUNE_POLICY={policy} ")
}

def private has(lines : array<string>; needle : string) : bool {
    for (ln in lines) {
        if (find(ln, needle) >= 0) return true
    }
    return false
}

def private count_hits(lines : array<string>; needle : string) : int {
    var n = 0
    for (ln in lines) {
        if (find(ln, needle) >= 0) {
            n++
        }
    }
    return n
}

[test]
def test_llvm_tune_scope_policy(t : T?) {   // nolint:STYLE038 - flat numbered policy-flavor arms over shared child-spawn state; no arm stands alone
    let args <- get_command_line_arguments()  // locked view — do not delete
    let bin = args[0]

    // Documentation is a host policy, so it is visible while required modules
    // compile: no manifests, fallback stamps, status init, or tune policy.
    let docsClient = "{get_this_module_dir()}/llvm_tune_scope_client_docs.das"
    let docsSidecar = "{get_this_module_dir()}/llvm_tune_scope_client_docs.tune.json"
    remove(docsSidecar)
    var ldocs : array<string>
    t |> equal(spawn_child("{tune_env("error")}\"{bin}\" -no-module-cache -documentation \"{docsClient}\"", ldocs), 0)
    t |> success(!(ldocs |> has("llvm_tune:")))
    t |> success(!(ldocs |> has("untuned")))
    t |> success(ldocs |> has("RESULT 5"))
    t |> success(!fexist(docsSidecar))
    delete ldocs

    if (!jit_enabled()) {
        // the auto flow's RESULT fingerprint needs the generator tier; children need -jit too
        t |> success(true)
        return
    }
    let client = "{get_this_module_dir()}/llvm_tune_scope_client.das"
    let sidecar = "{get_this_module_dir()}/llvm_tune_scope_client.tune.json"
    remove(sidecar)

    // 1. policy=fallback (the annotation default), no sidecar: fallback stamps, quiet
    var l1 : array<string>
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" -no-module-cache \"{client}\"", l1), 0)
    t |> success(l1 |> has("scope_add <- k1 (fallback)"))
    t |> success(l1 |> has("scope_mul <- k2 (fallback)"))
    t |> success(l1 |> has("STATUS scope_add|llvm_tune_scope_probe|k1|fallback"))
    t |> success(l1 |> has("STATUS scope_mul|llvm_tune_scope_probe2|k2|fallback"))
    t |> success(l1 |> has("RESULT 5"))   // interp = reference body regardless of stamp

    // 2. policy=error: the compile must fail, naming the untuned scope + the tuner command
    var l2 : array<string>
    t |> success(spawn_child("{tune_env("error")}\"{bin}\" -no-module-cache \"{client}\"", l2) != 0)
    t |> success(l2 |> has("untuned"))

    // 3. policy=auto (-jit): both tuners run mid-start into the ONE sidecar (tuner 2's
    //    upsert preserves tuner 1's key — key completeness, not file existence, is what
    //    makes scope 2 tune after scope 1 already created the file), then the re-exec'd
    //    compile stamps both winners and the k2 tier actually executes
    var l3 : array<string>
    t |> equal(spawn_child("{tune_env("auto")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{client}\"", l3), 0)
    t |> success(l3 |> has("FAKE TUNER ->"))
    t |> success(l3 |> has("FAKE TUNER2 ->"))
    t |> success(l3 |> has("scope_add <- k2 ("))
    t |> success(l3 |> has("scope_mul <- k0 ("))
    t |> success(l3 |> has("RESULT 7"))             // 2+3+k2 — the sidecar tier ran
    t |> success(l3 |> has("RESULT2 5"))            // 2+3+k0 — its own scope's winner
    t |> success(l3 |> has("STATUS scope_add|llvm_tune_scope_probe|k2|manifest"))
    t |> success(l3 |> has("STATUS scope_mul|llvm_tune_scope_probe2|k0|manifest"))
    t |> success(!empty(fread(sidecar)))

    // 4. plain start with the sidecar present: [tune]-time resolution, no policy activity
    var l4 : array<string>
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" -no-module-cache \"{client}\"", l4), 0)
    t |> success(l4 |> has("scope_add <- k2 ("))
    t |> success(l4 |> has("scope_mul <- k0 ("))
    t |> success(!(l4 |> has("FAKE TUNER")))

    // 5. --tune forces a re-tune even though the sidecar is complete
    var l5 : array<string>
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" -no-module-cache \"{client}\" -- --tune", l5), 0)
    t |> success(l5 |> has("FAKE TUNER ->"))
    t |> success(l5 |> has("FAKE TUNER2 ->"))

    // 6. policy=restart: each start tunes one incomplete scope via the main-guard and asks
    //    for a restart (no RESULT); with both scopes' keys present, main runs
    remove(sidecar)
    var l6a : array<string>
    t |> equal(spawn_child("{tune_env("restart")}\"{bin}\" -no-module-cache \"{client}\"", l6a), 0)
    t |> success(l6a |> has("restart to apply the winners"))
    t |> success(!(l6a |> has("RESULT")))
    var l6b : array<string>
    t |> equal(spawn_child("{tune_env("restart")}\"{bin}\" -no-module-cache \"{client}\"", l6b), 0)
    t |> success(l6b |> has("restart to apply the winners"))
    t |> success(!(l6b |> has("RESULT")))
    var l6c : array<string>
    t |> equal(spawn_child("{tune_env("restart")}\"{bin}\" -no-module-cache \"{client}\"", l6c), 0)
    t |> success(l6c |> has("RESULT 5"))

    // 6b. policy=restart + a registered scope resolver: the guard consults it INSTEAD of
    //     spawning the tuner — the resolver writes the winner (the exchange-client shape),
    //     prints its own restart marker, and the next start stamps what it wrote
    let resolverClient = "{get_this_module_dir()}/llvm_tune_scope_client_resolver.das"
    let resolverSidecar = "{get_this_module_dir()}/llvm_tune_scope_client_resolver.tune.json"
    remove(resolverSidecar)
    var l6r : array<string>
    t |> equal(spawn_child("{tune_env("restart")}\"{bin}\" -no-module-cache \"{resolverClient}\"", l6r), 0)
    t |> success(l6r |> has("RESOLVER llvm_tune_scope_probe ->"))
    t |> success(!(l6r |> has("FAKE TUNER")))
    t |> success(l6r |> has("exchange: sidecar applied, restart to apply the winners"))
    t |> success(!(l6r |> has("RESULT")))
    var l6s : array<string>
    t |> equal(spawn_child("{tune_env("restart")}\"{bin}\" -no-module-cache \"{resolverClient}\"", l6s), 0)
    t |> success(l6s |> has("STATUS scope_add|llvm_tune_scope_probe|k2|manifest"))
    t |> success(l6s |> has("RESULT 5"))   // interp = reference body; the STATUS row is the stamp proof
    remove(resolverSidecar)
    delete l6r
    delete l6s

    // 7. AOT gate: policy=error + missing sidecar must stay COMPLETELY silent
    remove(sidecar)
    let outRes = create_temp_file_result("llvm_tune_scope_aot_", ".cpp")
    if (outRes is error) {
        t |> failure("could not create a temp file: {outRes as error}")
        return
    }
    let aotOut = outRes as value
    var l7 : array<string>
    t |> equal(spawn_child("{tune_env("error")}\"{bin}\" -aot \"{client}\" \"{aotOut}\"", l7), 0)
    t |> success(!(l7 |> has("llvm_tune:")))
    t |> success(!(l7 |> has("untuned")))

    // 8. -exe gate: stamping runs (fallback, no sidecar) but the policy rail is dead
    var l8 : array<string>
    t |> equal(spawn_child("{tune_env("error")}\"{bin}\" -no-module-cache -jit -exe \"{client}\"", l8), 0)
    t |> success(l8 |> has("scope_add <- k1 (fallback)"))
    t |> success(!(l8 |> has("untuned")))

    // 9. NO [tune_policy] declared: the auto DEFAULT — "untuned does not start". The first
    //    start tunes both scopes and re-execs, so ONE launch already runs the tuned tier;
    //    the second start converges (no tuner runs)
    let autoClient = "{get_this_module_dir()}/llvm_tune_scope_client_auto.das"
    let autoSidecar = "{get_this_module_dir()}/llvm_tune_scope_client_auto.tune.json"
    remove(autoSidecar)
    var l9 : array<string>
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{autoClient}\"", l9), 0)
    t |> success(l9 |> has("FAKE TUNER ->"))
    t |> success(l9 |> has("FAKE TUNER2 ->"))
    t |> success(l9 |> has("RESULT 7"))
    t |> success(l9 |> has("STATUS scope_add|llvm_tune_scope_probe|k2|manifest"))
    var l9b : array<string>
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{autoClient}\"", l9b), 0)
    t |> success(!(l9b |> has("FAKE TUNER")))
    t |> success(l9b |> has("RESULT 7"))

    // 10. DAS_TUNE_POLICY=fallback is the kill switch for the default: no tuning, fallback
    //     stamps (RESULT 6 = 2+3+k1)
    remove(autoSidecar)
    var l10 : array<string>
    t |> equal(spawn_child("{tune_env("fallback")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{autoClient}\"", l10), 0)
    t |> success(!(l10 |> has("FAKE TUNER")))
    t |> success(l10 |> has("RESULT 6"))

    // 11. --jit-opt-level=0 flips the injected DEFAULT to fallback (winners are raced under
    //     O3 codegen — an O0 run must not block on the tuner to mint them): no tuning,
    //     fallback stamps, same RESULT 6 as the env kill switch
    remove(autoSidecar)
    var l11 : array<string>
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{autoClient}\" -- --jit-opt-level=0", l11), 0)
    t |> success(!(l11 |> has("FAKE TUNER")))
    t |> success(l11 |> has("RESULT 6"))

    // 12. DAS_TUNE_POLICY overrides the O0 default in the other direction too: env auto beats
    //     the injected fallback — the tuner runs and the tuned tier serves
    remove(autoSidecar)
    var l12 : array<string>
    t |> equal(spawn_child("{tune_env("auto")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{autoClient}\" -- --jit-opt-level=0", l12), 0)
    t |> success(l12 |> has("FAKE TUNER ->"))
    t |> success(l12 |> has("RESULT 7"))

    // 13. tune_suppress_mint from a library [init] (the dev-run escape): the auto DEFAULT would
    //     tune, but the mint is suppressed — no tuner, fallback stamps, the suppression says why
    let supClient = "{get_this_module_dir()}/llvm_tune_scope_client_suppress.das"
    let supSidecar = "{get_this_module_dir()}/llvm_tune_scope_client_suppress.tune.json"
    remove(supSidecar)
    var l13 : array<string>
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{supClient}\"", l13), 0)
    t |> success(!(l13 |> has("FAKE TUNER")))
    t |> success(l13 |> has("mint SUPPRESSED (TEST_SUPPRESS=1)"))
    t |> success(l13 |> has("RESULT 6"))
    t |> success(!fexist(supSidecar))

    // 14. version_of= pin: the tuner stamps provenance.testver beside the winner, so the mint
    //     satisfies the pin and the next start converges; a sidecar whose kernel keys are all
    //     present but whose recorded version differs must refuse BY NAME
    let verClient = "{get_this_module_dir()}/llvm_tune_scope_client_ver.das"
    let verSidecar = "{get_this_module_dir()}/llvm_tune_scope_client_ver.tune.json"
    remove(verSidecar)
    var l14 : array<string>
    t |> equal(spawn_child("{tune_env("auto")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{verClient}\"", l14), 0)
    t |> success(l14 |> has("FAKE TUNER VER ->"))
    t |> success(l14 |> has("FAKE TUNER VER2 ->"))   // the version_key= scope tunes too
    t |> success(l14 |> has("RESULT 7"))
    t |> success(l14 |> has("RESULT2 7"))
    var l14b : array<string>
    t |> equal(spawn_child("{tune_env("auto")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{verClient}\"", l14b), 0)
    t |> success(!(l14b |> has("FAKE TUNER")))   // both pins satisfied by the mint — no re-tune loop
    t |> success(l14b |> has("RESULT 7"))
    t |> success(l14b |> has("RESULT2 7"))
    // swap the recorded version: complete kernel keys, wrong provenance — refused, both values named
    var verr = ""
    var vdoc = read_json(fread(verSidecar), verr)
    t |> success(vdoc != null, "ver sidecar parses: {verr}")
    var vprov = vdoc?["provenance"]
    if (vprov != null) {
        update(vprov, "testver", JV("1"))
    }
    fwrite(verSidecar, write_json(vdoc))
    delete_json(vdoc)
    var l14c : array<string>
    t |> success(spawn_child("{tune_env("error")}\"{bin}\" -no-module-cache \"{verClient}\"", l14c) != 0)
    t |> success(l14c |> has("minted at testver 1"))
    t |> success(l14c |> has("the code is at 2"))

    // 14d. tune_scopes_status carries the pin: the -exe release-deps JSON reports the
    //      version-stale scope incomplete while its version_key= sibling stays complete —
    //      the daspkg `--quick` inherit gate reads exactly this
    let verJson = "{get_this_module_dir()}/llvm_tune_scope_ver_deps.json"
    remove(verJson)
    var l14d : array<string>
    t |> equal(spawn_child("{tune_env("error")}\"{bin}\" -no-module-cache -jit -exe --list-shared-modules \"{verJson}\" \"{verClient}\"", l14d), 0)
    var derr = ""
    var ddoc = read_json(fread(verJson), derr)
    t |> success(ddoc != null, "release-deps json parses: {derr}")
    var sawProbe = false
    var sawProbe2 = false
    let dscopes = ddoc?["tune_scopes"]
    if (dscopes != null && dscopes is _array) {
        for (e in dscopes as _array) {
            let sname = e?["name"] ?? ""
            if (sname == "llvm_tune_scope_verprobe") {
                sawProbe = true
                t |> equal(e?["complete"] ?? true, false)   // testver mismatch -> incomplete
            } elif (sname == "llvm_tune_scope_verprobe2") {
                sawProbe2 = true
                t |> equal(e?["complete"] ?? false, true)   // tv2 pin satisfied -> complete
            }
        }
    }
    delete_json(ddoc)
    t |> success(sawProbe, "verprobe row present in tune_scopes")
    t |> success(sawProbe2, "verprobe2 row present in tune_scopes")
    remove(verJson)

    // 15. DAS_TUNE_MANIFEST pointing at a file that reads untuned: the policy rail is dead by
    //     design, but ONE loud warning names the reason; the run proceeds on fallback stamps
    let mfPath = "{get_this_module_dir()}/llvm_tune_scope_no_such.tune.json"
    let mfEnv = (get_platform_name() == "windows" ?
        "set DAS_TUNE_MODE=&& set DAS_TUNE_VERBOSITY=verbose&& set DAS_TUNE_POLICY=&& set DAS_TUNE_MANIFEST={mfPath}&& " :
        "DAS_TUNE_MODE= DAS_TUNE_VERBOSITY=verbose DAS_TUNE_POLICY= DAS_TUNE_MANIFEST={mfPath} ")
    var l15 : array<string>
    t |> equal(spawn_child("{mfEnv}\"{bin}\" -no-module-cache \"{client}\"", l15), 0)
    t |> equal(l15 |> count_hits("WARNING DAS_TUNE_MANIFEST"), 2)   // once per scope, deduped
    t |> success(l15 |> has("no sidecar at"))
    t |> success(!(l15 |> has("FAKE TUNER")))
    t |> success(l15 |> has("RESULT 5"))
    // same warning from the DEFAULT-policy seat (no [tune_policy] declared)
    var l15b : array<string>
    t |> equal(spawn_child("{mfEnv}\"{bin}\" -no-module-cache \"{autoClient}\"", l15b), 0)
    t |> equal(l15b |> count_hits("WARNING DAS_TUNE_MANIFEST"), 2)
    t |> success(!(l15b |> has("FAKE TUNER")))

    remove(verSidecar)
    delete l14
    delete l14b
    delete l14c
    delete l14d
    delete l15
    delete l15b

    remove(sidecar)
    remove(autoSidecar)
    remove(supSidecar)
    remove(aotOut)
    delete l1
    delete l2
    delete l3
    delete l4
    delete l5
    delete l6a
    delete l6b
    delete l6c
    delete l7
    delete l8
    delete l9
    delete l9b
    delete l10
    delete l11
    delete l12
    delete l13
}
