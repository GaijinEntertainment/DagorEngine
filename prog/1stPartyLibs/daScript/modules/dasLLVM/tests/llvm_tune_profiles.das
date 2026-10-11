options gen2

require dastest/testing_boost
require daslib/module_path
require daslib/strings_boost
require daslib/fio
require strings
require _spawn_common
require llvm/daslib/llvm_tune

// The shipped-defaults-profile rail ([tune_scope(defaults = ...)]) through child daslang
// processes: an untuned start is COVERED by the class profile - the compile stamps its picks
// (source=profile), no tuner spawn, no relaunch, no sidecar written; a profile whose minting
// box lacked a feature this box has adopts AND races ONLY the unlocked family (DAS_TUNE_ONLY
// armed for the tuner, cleared after), which does write and relaunch; no profile file = the
// old full race. A scope that ships defaults must compile before the modules it covers -
// cant_tune_scope_after_covered.das proves the apply refuses the other order. The re-exec arms
// DAS_TUNE_RELAUNCH for its child, and a child that still reads the scope as untuned stops
// instead of tuning and relaunching again (case 8 - the fuse against a process chain). The fixture set
// is COPIED into a per-process temp directory and every
// spawn runs from the copy: [tune_scope(defaults=...)] resolves against the declaring file
// and the per-app sidecar resolves beside the root script, so the defaults dir, the profile
// files and the sidecar all land under the copy and the tracked fixtures beside this file
// are never written or removed. A profile's name carries this box's tune_cpu_class(), and
// its features field is this box's fingerprint (or empty, to fake a less-capable minting
// box).

//! the tune environment a spawned child sees: everything cleared, verbose, and the relaunch
//! depth given - "" for the process the test spawns, "1" for what a re-exec'd child sees
def private tune_env_prefix(relaunch : string) : string {
    return (get_platform_name() == "windows" ?
        "set DAS_TUNE_MODE=&& set DAS_TUNE_MANIFEST=&& set DAS_TUNE_VERBOSITY=verbose&& set DAS_TUNE_POLICY=&& set DAS_TUNE_ONLY=&& set DAS_TUNE_RELAUNCH={relaunch}&& " :
        "DAS_TUNE_MODE= DAS_TUNE_MANIFEST= DAS_TUNE_VERBOSITY=verbose DAS_TUNE_POLICY= DAS_TUNE_ONLY= DAS_TUNE_RELAUNCH={relaunch} ")
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

def private has(lines : array<string>; needle : string) : bool {
    for (ln in lines) {
        if (find(ln, needle) >= 0) {
            return true
        }
    }
    return false
}

def private write_profile(dir, klass, winner, features : string) {
    mkdir(dir)
    let body = "\{ \"kernels\" : \{ \"prof_add\" : \"{winner}\" \}, \"provenance\" : \{ \"features\" : \"{features}\" \} \}"
    fwrite(path_join(dir, "{klass}.tune-defaults.json"), body)
}

// The working copy's defaults dir is named apart from the dir the scope fixture declares, so
// no path this test writes is ever spelled like the tracked defaults dir beside it.
let private WORK_DEFAULTS = "tune_defaults"
let private SCOPE_FIXTURE = "llvm_tune_profiles_lib.das"
let private DECLARED_DEFAULTS = "defaults = \"./"

def private copy_fixture(t : T?; name, dstdir : string) {
    let text = fread("{get_this_module_dir()}/{name}")
    t |> success(!empty(text), "fixture {name} is unreadable")
    t |> success(fwrite(path_join(dstdir, name), text), "could not copy {name} into {dstdir}")
}

def private redirect_defaults(src : string) : string {
    let at = find(src, DECLARED_DEFAULTS)
    return src if (at < 0)
    let from = at + length(DECLARED_DEFAULTS)
    let close = find(src, "\"", from)
    return src if (close < 0)
    return "{slice(src, 0, at)}{DECLARED_DEFAULTS}{WORK_DEFAULTS}{slice(src, close)}"
}

def private copy_scope_fixture(t : T?; dstdir : string) {
    let src = fread("{get_this_module_dir()}/{SCOPE_FIXTURE}")
    let text = redirect_defaults(src)
    t |> success(text != src, "{SCOPE_FIXTURE} declares no relative {DECLARED_DEFAULTS}...\" for the copy to redirect")
    t |> success(fwrite(path_join(dstdir, SCOPE_FIXTURE), text), "could not copy {SCOPE_FIXTURE} into {dstdir}")
}

[test]
def test_llvm_tune_profiles(t : T?) { // nolint:STYLE038 - one flat spawn-and-assert rail per scenario; a split would hide the sequence the scenarios build on
    if (!jit_enabled()) {
        // no generator tier without the jit, so no stamped-tier fingerprint to read
        t |> success(true)
        return
    }
    let args <- get_command_line_arguments()  // aliases a process-global Array header - do not delete
    let bin = args[0]
    let dirRes = create_temp_directory_result("llvm_tune_profiles_")
    if (dirRes is error) {
        t |> failure("could not create a temp dir: {dirRes as error}")
        return
    }
    let work = dirRes as value
    copy_fixture(t, "llvm_tune_profiles_client.das", work)
    copy_fixture(t, "llvm_tune_profiles_tuner.das", work)
    copy_scope_fixture(t, work)
    let client = path_join(work, "llvm_tune_profiles_client.das")
    let sidecar = path_join(work, "llvm_tune_profiles_client.tune.json")
    let defdir = path_join(work, WORK_DEFAULTS)
    let klass = tune_cpu_class()
    let profile = path_join(defdir, "{klass}.tune-defaults.json")

    // 1. no profile file: the plain auto flow - full race (empty filter), the tuner's k2 runs
    remove(sidecar)
    remove(profile)
    var noProfile : array<string>
    t |> equal(spawn_child("{tune_env_prefix("")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{client}\"", noProfile), 0, "case 1 exit code")
    t |> success(noProfile |> has("FAKE TUNER"), "case 1: no profile - the tuner runs\n{join(noProfile, "\n")}")
    t |> success(noProfile |> has("ONLY=''"), "case 1: the full race carries no --tune-only filter")
    t |> success(!(noProfile |> has("defaults profile")), "case 1: nothing mentions a profile")
    t |> success(noProfile |> has("RESULT 7"), "case 1: the tuner's k2 tier ran")
    delete noProfile

    // 2. full-cover profile (this box's own fingerprint): the compile stamps its winner, no tuner, no relaunch, no sidecar
    remove(sidecar)
    write_profile(defdir, klass, "k0", tune_feature_fingerprint())
    var fullCover : array<string>
    t |> equal(spawn_child("{tune_env_prefix("")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{client}\"", fullCover), 0, "case 2 exit code")
    t |> success(fullCover |> has("covered by the '{klass}' defaults profile"), "case 2: the guard reports the box covered\n{join(fullCover, "\n")}")
    t |> success(!(fullCover |> has("FAKE TUNER")), "case 2: no tuner on a covered box")
    t |> success(!(fullCover |> has("re-launching")), "case 2: a covered box compiles once")
    t |> success(fullCover |> has("RESULT 5"), "case 2: the profile's k0 tier ran")
    t |> success(fullCover |> has("STATUS prof_add|llvm_tune_profiles_probe|k0|profile"), "case 2: the stamp's source is the profile")
    var coveredStat : FStat
    t |> success(!stat(sidecar, coveredStat), "case 2: a covered box writes no sidecar")
    t |> success(fullCover |> has("(1/1 kernels tuned for this box)"), "case 2: the status count reads a profile stamp as tuned")
    t |> success(!(fullCover |> has("run with --tune")), "case 2: no --tune hint on a covered box")
    delete fullCover

    // 3. a profile minted with no features leaves the k3 requires= seat unlocked here: adopt AND race just that family - the tuner's k2 outranks the adopted k0
    remove(sidecar)
    write_profile(defdir, klass, "k0", "")
    var unlockedSeat : array<string>
    t |> equal(spawn_child("{tune_env_prefix("")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{client}\"", unlockedSeat), 0, "case 3 exit code")
    t |> success(unlockedSeat |> has("racing 1 unlocked/new families"), "case 3: the unlocked seat races\n{join(unlockedSeat, "\n")}")
    t |> success(unlockedSeat |> has("ONLY='prof_add'"), "case 3: the residue filter names the family")
    t |> success(unlockedSeat |> has("RESULT 7"), "case 3: the raced k2 tier ran")
    t |> success(unlockedSeat |> has("STATUS prof_add|llvm_tune_profiles_probe|k2|manifest"), "case 3: the raced winner stamps from the sidecar")
    t |> equal(count_hits(unlockedSeat, "RELAUNCH=1"), 1, "case 3: the re-exec arms depth 1 for its child, and the child does not relaunch")
    t |> success(!(unlockedSeat |> has("RELAUNCH=2")), "case 3: no process ever runs at depth 2")
    delete unlockedSeat

    // 4. --tune outranks the profile: full re-race, no adoption
    var forcedTune : array<string>
    t |> equal(spawn_child("{tune_env_prefix("")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{client}\" -- --tune", forcedTune), 0, "case 4 exit code")
    t |> success(!(forcedTune |> has("defaults profile")), "case 4: --tune skips adoption\n{join(forcedTune, "\n")}")
    t |> success(forcedTune |> has("FAKE TUNER"), "case 4: --tune runs the tuner on a complete sidecar")
    delete forcedTune

    // 5. --tune-only converges: the relaunch strips both spellings, so exactly one tuner runs and
    //    the child does not re-tune forever (the strip arms are what keep it from looping)
    var onlyOne : array<string>
    t |> equal(spawn_child("{tune_env_prefix("")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{client}\" -- --tune-only prof_add", onlyOne), 0, "case 5 exit code")
    t |> equal(count_hits(onlyOne, "FAKE TUNER"), 1, "case 5: --tune-only runs exactly one tuner\n{join(onlyOne, "\n")}")
    t |> success(onlyOne |> has("ONLY='prof_add'"), "case 5: the filter reaches the tuner")
    t |> success(onlyOne |> has("RESULT 7"), "case 5: the re-minted k2 tier ran")
    delete onlyOne
    var onlyEq : array<string>
    t |> equal(spawn_child("{tune_env_prefix("")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{client}\" -- --tune-only=prof_add", onlyEq), 0, "case 5b exit code")
    t |> equal(count_hits(onlyEq, "FAKE TUNER"), 1, "case 5b: the = spelling runs exactly one tuner\n{join(onlyEq, "\n")}")
    delete onlyEq

    // 6. the class chain: only a LOWER class's profile on disk, and the box adopts it (the
    //    residue race then covers whatever its own class unlocks)
    var chain <- tune_class_chain()
    t |> equal(chain[0], tune_cpu_class(), "the box's own class heads its ladder - a class outside the chain could never adopt its own profile")
    if (length(chain) > 1) {
        let lower = chain[length(chain) - 1]
        remove(sidecar)
        remove(profile)
        write_profile(defdir, lower, "k0", tune_feature_fingerprint())
        var lowerAdopt : array<string>
        t |> equal(spawn_child("{tune_env_prefix("")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{client}\"", lowerAdopt), 0, "case 6 exit code")
        t |> success(lowerAdopt |> has("covered by the '{lower}' defaults profile"), "case 6: the lower class's profile covers the box\n{join(lowerAdopt, "\n")}")
        t |> success(!(lowerAdopt |> has("FAKE TUNER")), "case 6: no tuner under a lower-class profile")
        delete lowerAdopt
        remove(path_join(defdir, "{lower}.tune-defaults.json"))
    }
    delete chain

    // 7. DAS_TUNE_POLICY=reference serves the original bodies and announces itself even on a
    //    fully tuned box (the sidecar from case 5 is complete)
    var refRun : array<string>
    let refEnv = (get_platform_name() == "windows" ?
        "set DAS_TUNE_MODE=&& set DAS_TUNE_MANIFEST=&& set DAS_TUNE_VERBOSITY=verbose&& set DAS_TUNE_POLICY=reference&& set DAS_TUNE_ONLY=&& " :
        "DAS_TUNE_MODE= DAS_TUNE_MANIFEST= DAS_TUNE_VERBOSITY=verbose DAS_TUNE_POLICY=reference DAS_TUNE_ONLY= ")
    t |> equal(spawn_child("{refEnv}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{client}\"", refRun), 0, "case 7 exit code")
    t |> success(refRun |> has("policy 'reference' set by DAS_TUNE_POLICY"), "case 7: the reference policy announces itself\n{join(refRun, "\n")}")
    t |> success(refRun |> has("STATUS prof_add|llvm_tune_profiles_probe||reference"), "case 7: the stamp reads reference")
    t |> success(refRun |> has("RESULT 5"), "case 7: the original body ran")
    delete refRun

    // 8. the relaunch fuse: a process that already IS the relaunched child (DAS_TUNE_RELAUNCH=1)
    //    and still reads the scope as untuned neither tunes nor relaunches - it says so and runs
    //    its fallback stamps, so a tune that cannot converge stops at one restart
    remove(sidecar)
    remove(profile)
    var fused : array<string>
    t |> equal(spawn_child("{tune_env_prefix("1")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{client}\"", fused), 0, "case 8 exit code")
    t |> success(fused |> has("did not converge"), "case 8: the relaunched child reports the non-convergence\n{join(fused, "\n")}")
    t |> success(fused |> has("DAS_TUNE_RELAUNCH=1"), "case 8: the report names the depth knob")
    t |> success(!(fused |> has("FAKE TUNER")), "case 8: no second tuner")
    t |> success(!(fused |> has("re-launching")), "case 8: no second relaunch")
    t |> success(fused |> has("|fallback"), "case 8: the kernels run their fallback stamps")
    t |> success(fused |> has("(0/1 kernels tuned for this box)"), "case 8: the status count reads a fallback stamp as untuned")
    t |> success(fused |> has("run with --tune"), "case 8: the --tune hint fires below the profile tier")
    delete fused

    rmdir_rec(work)
}

[export]
def main {
    return
}
