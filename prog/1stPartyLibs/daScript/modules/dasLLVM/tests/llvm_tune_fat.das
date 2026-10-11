options gen2

require dastest/testing_boost
require daslib/module_path
require daslib/strings_boost
require daslib/fio
require strings
require _spawn_common
require llvm/daslib/llvm_tune
require llvm/daslib/llvm_cpu_class

// The fat mode (DAS_TUNE_MODE=fat under DAS_JIT_BASELINE=<class>, -exe) through child daslang
// processes: the build stamps one clone per class the scope ships a profile for, the exe picks
// the clone from cpuid at startup (the companion switching with it), DAS_TUNE_FAT_CLASS pins the
// pick and refuses a class the box is not in, and the mode refuses a non-exe build and a build
// with no baseline. The fixture set is COPIED into a per-process temp directory, the scope's
// defaults dir redirected into the copy, so the tracked fixtures beside this file are never
// written. Profiles are written for the ladder's lowest class (the baseline, k0) and its top
// class (k2); which clone RESULT reports depends on whether this box is in the top class.

def private tune_env_prefix(mode, baseline, pin : string) : string {
    return (get_platform_name() == "windows" ?
        "set DAS_TUNE_MODE={mode}&& set DAS_JIT_BASELINE={baseline}&& set DAS_TUNE_FAT_CLASS={pin}&& set DAS_TUNE_MANIFEST=&& set DAS_TUNE_VERBOSITY=verbose&& set DAS_TUNE_POLICY=&& set DAS_LOG_LEVEL=info&& " :
        "DAS_TUNE_MODE={mode} DAS_JIT_BASELINE={baseline} DAS_TUNE_FAT_CLASS={pin} DAS_TUNE_MANIFEST= DAS_TUNE_VERBOSITY=verbose DAS_TUNE_POLICY= DAS_LOG_LEVEL=info ")
}

def private has(lines : array<string>; needle : string) : bool {
    for (ln in lines) {
        if (find(ln, needle) >= 0) {
            return true
        }
    }
    return false
}

// the `attributes #N = { ... }` line of the IR function whose `define` line carries `needle`, "" when absent
def private attr_group_of(lines : array<string>; needle : string) : string {
    for (ln in lines) {
        continue if (!(ln |> starts_with("define")) || find(ln, needle) < 0)
        let hashAt = rfind(ln, " #")
        continue if (hashAt < 0)
        let rest = slice(ln, hashAt + 1)
        let sp = find(rest, " ")
        let group = sp < 0 ? rest : slice(rest, 0, sp)
        let key = "attributes {group} = "
        for (al in lines) {
            if (al |> starts_with(key)) {
                return al
            }
        }
    }
    return ""
}

def private write_profile(dir, klass, winner : string) {
    mkdir(dir)
    let features = jit_cpu_class_row(klass).features
    let body = "\{ \"kernels\" : \{ \"fat_add\" : \"{winner}\" \}, \"provenance\" : \{ \"features\" : \"{replace(features, ";", ";")}\" \} \}"
    fwrite(path_join(dir, "{klass}.tune-defaults.json"), body)
}

let private WORK_DEFAULTS = "tune_defaults"
let private SCOPE_FIXTURE = "llvm_tune_fat_lib.das"
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
def test_llvm_tune_fat(t : T?) { // nolint:STYLE038 - one flat spawn-and-assert rail per scenario; a split would hide the sequence the scenarios build on
    if (!jit_enabled()) {
        t |> success(true)
        return
    }
    let args <- get_command_line_arguments()  // aliases a process-global Array header - do not delete
    let bin = args[0]
    let dirRes = create_temp_directory_result("llvm_tune_fat_")
    if (dirRes is error) {
        t |> failure("could not create a temp dir: {dirRes as error}")
        return
    }
    let work = dirRes as value
    copy_fixture(t, "llvm_tune_fat_client.das", work)
    copy_scope_fixture(t, work)
    let client = path_join(work, "llvm_tune_fat_client.das")
    let exe = path_join(work, "llvm_tune_fat_client")
    let defdir = path_join(work, WORK_DEFAULTS)
    var ladder <- [for (c in split(jit_cpu_class_ladder(get_architecture_name()), ";")); c; where !empty(c)]
    t |> success(length(ladder) >= 2, "this arch has a class ladder to be fat over")
    let top = ladder[0]
    let low = ladder[length(ladder) - 1] == "x86-base" ? "x86-avx2" : ladder[length(ladder) - 1]
    let boxInTop = tune_fat_box_in_class(top)
    write_profile(defdir, low, "k0")
    write_profile(defdir, top, "k2")

    // 1. the fat build: one clone per class with a profile, the baseline's from its own profile;
    //    the IR dump shows each clone a noinline function lowered for its own class
    var lb : array<string>
    let build_rc = spawn_child("{tune_env_prefix("fat", low, "")}\"{bin}\" -no-module-cache -exe -output \"{exe}\" \"{client}\" -- --jit-dump", lb)
    t |> equal(build_rc, 0, "case 1: the fat exe builds\n{join(lb, "\n")}")
    t |> success(lb |> has("fat clones {top}=k2/profile, {low}=k0/profile"), "case 1: the build names every class clone and its source")
    t |> success(lb |> has("DAS_JIT_BASELINE={low}"), "case 1: the build targets the baseline class")
    t |> success(!(lb |> has("targeting the current box")), "case 1: a fat exe never targets the box")
    let topAttrs = attr_group_of(lb, "fat_add__fat_{replace(top, "-", "_")}")
    let lowAttrs = attr_group_of(lb, "fat_add__fat_{replace(low, "-", "_")}")
    let topOnly = jit_cpu_class_ladder(get_architecture_name()) |> starts_with("x86") ? "+avx512vnni" : "+i8mm"
    t |> success(find(topAttrs, "\"target-features\"=\"{jit_class_plus_features(top)}\"") >= 0, "case 1: the {top} clone carries its class's features: {topAttrs}")
    t |> success(find(topAttrs, "noinline") >= 0, "case 1: the {top} clone is a function, never inlined into the baseline: {topAttrs}")
    t |> success(find(lowAttrs, "\"target-cpu\"=\"{jit_cpu_class_row(low).cpu}\"") >= 0 && find(lowAttrs, topOnly) < 0, "case 1: the {low} clone carries the baseline's cpu and none of the top class's features: {lowAttrs}")
    t |> success(lb |> has("@das.tune.fat_mask = internal unnamed_addr global i32 0"), "case 1: the dispatch mask is an LLVM-side word, not a das global")

    let can_run = get_platform_name() != "windows" && das_is_dll_build()
    if (can_run && fexist("{exe}.exe")) {
        // 2. the exe dispatches from cpuid: the top clone when this box is in the class, else the baseline's
        var lr : array<string>
        let run_rc = spawn_child("{tune_env_prefix("", "", "")}\"{exe}.exe\"", lr)
        t |> equal(run_rc, 0, "case 2: the fat exe runs\n{join(lr, "\n")}")
        t |> success(lr |> has(boxInTop ? "RESULT 6 KV 2 VOID 2 FORK 6" : "RESULT 4 KV 0 VOID 2 FORK 4"), "case 2: the {boxInTop ? top : low} clone and its companion ran, the void kernel's arm dispatched, and a pooled fork whose init never ran dispatched to the same clone (box in {top}: {boxInTop})\n{join(lr, "\n")}")
        t |> success(lr |> has("STATUS fat_add|{low}|k0|profile|active"), "case 2: the baseline clone is always active")
        t |> success(lr |> has("STATUS fat_add|{top}|k2|profile|{boxInTop ? "active" : "idle"}"), "case 2: the top clone's row says whether this box runs it")
        t |> success(lr |> has("fat kernels - this box runs the"), "case 2: the init announces the classes")
        t |> success(lr |> has("fat_add [{low} <- this box]: k0 (profile)"), "case 2: the status log marks the classes this box runs")
        delete lr

        // 3. DAS_TUNE_FAT_CLASS pins the pick to the baseline whatever cpuid says
        var lp : array<string>
        t |> equal(spawn_child("{tune_env_prefix("", "", low)}\"{exe}.exe\"", lp), 0, "case 3: the pinned exe runs")
        t |> success(lp |> has("RESULT 4 KV 0 VOID 2 FORK 4"), "case 3: the pinned baseline clone ran, in the pooled fork too\n{join(lp, "\n")}")
        t |> success(lp |> has("DAS_TUNE_FAT_CLASS={low} pins the class"), "case 3: the pin announces itself")
        delete lp

        // 4. a pin to a class this box is not in refuses to start
        if (!boxInTop) {
            var lf : array<string>
            t |> success(spawn_child("{tune_env_prefix("", "", top)}\"{exe}.exe\"", lf) != 0, "case 4: a pin outside the box refuses")
            t |> success(lf |> has("pins a CPU class this box is not in"), "case 4: the refusal names the pin\n{join(lf, "\n")}")
            delete lf
        }

        // 4b. a pin to a class the box is in but the exe carries no clone for refuses to start -
        //     only the top and the baseline are profiled, so a middle class of the ladder has none
        if (length(ladder) > 2 && tune_fat_box_in_class(ladder[1])) {
            var lm : array<string>
            t |> success(spawn_child("{tune_env_prefix("", "", ladder[1])}\"{exe}.exe\"", lm) != 0, "case 4b: a pin the exe carries no clone for refuses")
            t |> success(lm |> has("carries no clone for"), "case 4b: the refusal says the exe carries no such clone\n{join(lm, "\n")}")
            delete lm
        }
    }

    // 5. fat is an exe mode: a -jit run refuses by name
    var lj : array<string>
    t |> success(spawn_child("{tune_env_prefix("fat", low, "")}\"{bin}\" -no-module-cache -jit -jit-no-cache \"{client}\"", lj) != 0, "case 5: fat outside -exe refuses")
    t |> success(lj |> has("standalone-exe mode"), "case 5: the refusal names the mode\n{join(lj, "\n")}")

    // 6. fat needs the baseline class
    var ln : array<string>
    t |> success(spawn_child("{tune_env_prefix("fat", "", "")}\"{bin}\" -no-module-cache -exe -output \"{exe}\" \"{client}\"", ln) != 0, "case 6: fat without a baseline refuses")
    t |> success(ln |> has("needs DAS_JIT_BASELINE"), "case 6: the refusal names the knob\n{join(ln, "\n")}")

    delete ladder
    rmdir_rec(work)
}

[export]
def main {
    return
}
