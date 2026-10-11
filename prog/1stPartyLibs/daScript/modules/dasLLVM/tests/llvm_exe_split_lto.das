options gen2
require dastest/testing_boost public
require daslib/fio
require daslib/strings_boost
require strings
require _exe_test_common

// A standalone exe built split (`--jit-split-modules`: one partition a das module, the entry in a
// partition of its own) and split-plus-LTO (`--jit-lto`: bitcode partitions, the link inlines across
// them) runs the fixture the way the one-unit exe does, and leaves no partition object beside itself.

let ROOT = "modules/dasLLVM/tests/_exe_split_lto_root.das"

//! an empty `out` is a build or run that already failed above; its lines are not checked again
def private expect_lines(t : T?; out : string; tag : string) {
    if (empty(out)) return
    t |> success(find(out, "WORDS 3") >= 0, "{tag}: the strings_boost partition serves the split: {tail(out)}")
    t |> success(find(out, "MIX 4901") >= 0, "{tag}: the fixture's own partition folds the word lengths: {tail(out)}")
    t |> success(find(out, "SQRT 12") >= 0, "{tag}: the math partition answers: {tail(out)}")
    t |> success(find(out, "ADDR 42") >= 0, "{tag}: a function address taken in a body resolves through its partition's slot: {tail(out)}")
    t |> success(find(out, "TABLE 8") >= 0, "{tag}: the entry's global table literal reaches the accessor slot a partition collected first: {tail(out)}")
    t |> success(find(out, "VEC 211") >= 0, "{tag}: vector-constant literals at, past and below the constant-data floor read back whole: {tail(out)}")
}

//! a split build leaves its exe and the linker's map alone: the partition objects, bitcode and response file go with the link, and an exe that exports nothing gets no import library or export file
def private expect_no_partition_files(t : T?; dir : string; tag : string) {
    let link_only = [".o", ".bc", ".rsp", ".lib", ".exp"]
    var left : array<string>
    fio::dir(dir) $(name) {
        if (name |> starts_with("{tag}.")) {
            for (suffix in link_only) {
                if (name |> ends_with(suffix)) {
                    left |> push(name)
                }
            }
        }
    }
    t |> equal(0, length(left), "the split build ({tag}) removed its partition files and exported nothing: {join(left, ", ")}")
}

[test]
def test_exe_split(t : T?) {
    let dir_r = create_temp_directory_result("llvm_exe_split")
    if (!(dir_r is value)) {
        t |> failure("temp dir failed")
        return
    }
    let dir = unsafe(dir_r.value)
    t |> run("a split -exe (one partition a das module, the entry in its own) runs the fixture like the one-unit exe") @(t : T?) {
        let one = build_and_run(t, dir, "one", ROOT, [], [])
        expect_lines(t, one, "one unit")
        let split = build_and_run(t, dir, "split", ROOT, ["--jit-split-modules=-1"], ["split codegen (--jit-split-modules)"])
        expect_lines(t, split, "split")
        expect_no_partition_files(t, dir, "split")
    }
    rmdir_rec_result(dir)
}

[test]
def test_exe_flags_outside_their_rail(t : T?) {
    let dir_r = create_temp_directory_result("llvm_exe_flags")
    if (!(dir_r is value)) {
        t |> failure("temp dir failed")
        return
    }
    let dir = unsafe(dir_r.value)
    t |> run("--jit-lto and --jit-obj-cache without the split say they are ignored, and the one-unit exe still runs") @(t : T?) {
        let out = build_and_run(t, dir, "flags", ROOT, ["--jit-lto", "--jit-obj-cache=1"],
            ["--jit-lto applies to a split -exe", "--jit-obj-cache applies under --jit-split-modules"])
        expect_lines(t, out, "flags outside their rail")
    }
    rmdir_rec_result(dir)
}

[test]
def test_lto_driver_gate_parsing(t : T?) {
    t |> run("the LTO driver gate reads a driver's major and the bundled LLVM's on every platform") @(t : T?) {
        t |> equal(18, driver_major("Ubuntu clang version 18.1.3 (1ubuntu1)\nTarget: x86_64-pc-linux-gnu\n"))
        t |> equal(22, driver_major("clang version 22.1.5\n"))
        t |> equal(0, driver_major("g++ (GCC) 13.2.0\n"))
        t |> success(bundled_llvm_major() >= 16, "the CMake pin names the bundled LLVM's major: {bundled_llvm_major()}")
    }
}

[test]
def test_exe_split_lto(t : T?) {
    let why_not = lto_driver_missing()
    if (!empty(why_not)) {
        t |> skip(why_not)
        return
    }
    let dir_r = create_temp_directory_result("llvm_exe_split_lto")
    if (!(dir_r is value)) {
        t |> failure("temp dir failed")
        return
    }
    let dir = unsafe(dir_r.value)
    t |> run("a split -exe linked with LTO runs the fixture like the one-unit exe") @(t : T?) {
        let out = build_and_run(t, dir, "lto", ROOT, ["--jit-split-modules=-1", "--jit-lto"], ["the link runs LTO"])
        expect_lines(t, out, "split + LTO")
        expect_no_partition_files(t, dir, "lto")
    }
    rmdir_rec_result(dir)
}
