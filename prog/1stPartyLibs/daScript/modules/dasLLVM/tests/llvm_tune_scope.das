options gen2

require dastest/testing_boost
require daslib/module_path
require daslib/strings_boost
require daslib/fio
require strings

// [tune_scope]/[tune_policy]/--tune/tune_status round-trip through child daslang processes
// (the policy rail is compile-time). Two libraries each ship a scope + tuner; both write the
// ONE per-app sidecar — <client>.tune.json beside the spawned client script (the per-app
// location IS the feature) — via the upsert API, so the second tuner must preserve the first
// scope's keys. Per-scope "tuned" is KEY completeness, not file existence: after tuner 1
// writes the sidecar, scope 2 must still tune. Children mostly run interp (stamp prints and
// STATUS lines are mode-independent); only the auto flow runs -jit to prove the restamped
// tier executes (RESULT 7 = 2+3+k2).

def private tune_env(policy : string) : string {
    // verbose: the per-kernel `<fn> <- <perm>` stamp lines this test asserts on are the
    // stamped-truth proof, and they are verbose-only now (34 of them is a wall before an app starts)
    return (get_platform_name() == "windows" ?
        "set DAS_TUNE_MODE=&& set DAS_TUNE_MANIFEST=&& set DAS_TUNE_VERBOSITY=verbose&& set DAS_TUNE_POLICY={policy}&& " :
        "DAS_TUNE_MODE= DAS_TUNE_MANIFEST= DAS_TUNE_VERBOSITY=verbose DAS_TUNE_POLICY={policy} ")
}

// env prefix leads, so no sacrificial-quote wrap is needed (cmd.exe only strips a LEADING quote)
def private spawn_child(cmd : string; var lines : array<string>) : int {
    var rc : int
    unsafe {
        rc = popen_timeout("{cmd} 2>&1", 300.0) $(f) {
            if (f == null) {
                return
            }
            while (!feof(f)) {
                let ln = strip(fgets(f))
                if (!empty(ln)) {
                    lines |> push("{ln}")
                }
            }
        }
    }
    return rc
}

def private has(lines : array<string>; needle : string) : bool {
    for (ln in lines) {
        if (find(ln, needle) >= 0) return true
    }
    return false
}

[test]
def test_llvm_tune_scope_policy(t : T?) {
    let args <- get_command_line_arguments()  // locked view — do not delete
    let bin = args[0]

    // Documentation is a host policy, so it is visible while required modules
    // compile: no manifests, fallback stamps, status init, or tune policy.
    let docsClient = "{get_this_module_dir()}/llvm_tune_scope_client_docs.das"
    let docsSidecar = "{get_this_module_dir()}/llvm_tune_scope_client_docs.tune.json"
    remove(docsSidecar)
    var ldocs : array<string>
    t |> equal(spawn_child("{tune_env("error")}\"{bin}\" -documentation \"{docsClient}\"", ldocs), 0)
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
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" \"{client}\"", l1), 0)
    t |> success(l1 |> has("scope_add <- k1 (fallback)"))
    t |> success(l1 |> has("scope_mul <- k2 (fallback)"))
    t |> success(l1 |> has("STATUS scope_add|llvm_tune_scope_probe|k1|fallback"))
    t |> success(l1 |> has("STATUS scope_mul|llvm_tune_scope_probe2|k2|fallback"))
    t |> success(l1 |> has("RESULT 5"))   // interp = reference body regardless of stamp

    // 2. policy=error: the compile must fail, naming the untuned scope + the tuner command
    var l2 : array<string>
    t |> success(spawn_child("{tune_env("error")}\"{bin}\" \"{client}\"", l2) != 0)
    t |> success(l2 |> has("untuned"))

    // 3. policy=auto (-jit): both tuners run mid-start into the ONE sidecar (tuner 2's
    //    upsert preserves tuner 1's key — key completeness, not file existence, is what
    //    makes scope 2 tune after scope 1 already created the file), then the re-exec'd
    //    compile stamps both winners and the k2 tier actually executes
    var l3 : array<string>
    t |> equal(spawn_child("{tune_env("auto")}\"{bin}\" -jit \"{client}\"", l3), 0)
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
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" \"{client}\"", l4), 0)
    t |> success(l4 |> has("scope_add <- k2 ("))
    t |> success(l4 |> has("scope_mul <- k0 ("))
    t |> success(!(l4 |> has("FAKE TUNER")))

    // 5. --tune forces a re-tune even though the sidecar is complete
    var l5 : array<string>
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" \"{client}\" -- --tune", l5), 0)
    t |> success(l5 |> has("FAKE TUNER ->"))
    t |> success(l5 |> has("FAKE TUNER2 ->"))

    // 6. policy=restart: each start tunes one incomplete scope via the main-guard and asks
    //    for a restart (no RESULT); with both scopes' keys present, main runs
    remove(sidecar)
    var l6a : array<string>
    t |> equal(spawn_child("{tune_env("restart")}\"{bin}\" \"{client}\"", l6a), 0)
    t |> success(l6a |> has("restart to apply the winners"))
    t |> success(!(l6a |> has("RESULT")))
    var l6b : array<string>
    t |> equal(spawn_child("{tune_env("restart")}\"{bin}\" \"{client}\"", l6b), 0)
    t |> success(l6b |> has("restart to apply the winners"))
    t |> success(!(l6b |> has("RESULT")))
    var l6c : array<string>
    t |> equal(spawn_child("{tune_env("restart")}\"{bin}\" \"{client}\"", l6c), 0)
    t |> success(l6c |> has("RESULT 5"))

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
    t |> equal(spawn_child("{tune_env("error")}\"{bin}\" -jit -exe \"{client}\"", l8), 0)
    t |> success(l8 |> has("scope_add <- k1 (fallback)"))
    t |> success(!(l8 |> has("untuned")))

    // 9. NO [tune_policy] declared: the auto DEFAULT — "untuned does not start". The first
    //    start tunes both scopes and re-execs, so ONE launch already runs the tuned tier;
    //    the second start converges (no tuner runs)
    let autoClient = "{get_this_module_dir()}/llvm_tune_scope_client_auto.das"
    let autoSidecar = "{get_this_module_dir()}/llvm_tune_scope_client_auto.tune.json"
    remove(autoSidecar)
    var l9 : array<string>
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" -jit \"{autoClient}\"", l9), 0)
    t |> success(l9 |> has("FAKE TUNER ->"))
    t |> success(l9 |> has("FAKE TUNER2 ->"))
    t |> success(l9 |> has("RESULT 7"))
    t |> success(l9 |> has("STATUS scope_add|llvm_tune_scope_probe|k2|manifest"))
    var l9b : array<string>
    t |> equal(spawn_child("{tune_env("")}\"{bin}\" -jit \"{autoClient}\"", l9b), 0)
    t |> success(!(l9b |> has("FAKE TUNER")))
    t |> success(l9b |> has("RESULT 7"))

    // 10. DAS_TUNE_POLICY=fallback is the kill switch for the default: no tuning, fallback
    //     stamps (RESULT 6 = 2+3+k1)
    remove(autoSidecar)
    var l10 : array<string>
    t |> equal(spawn_child("{tune_env("fallback")}\"{bin}\" -jit \"{autoClient}\"", l10), 0)
    t |> success(!(l10 |> has("FAKE TUNER")))
    t |> success(l10 |> has("RESULT 6"))

    remove(sidecar)
    remove(autoSidecar)
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
}
