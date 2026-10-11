options gen2
require dastest/testing_boost public
require daslib/fio
require daslib/strings_boost
require strings
require math

// A das global reaches JIT code through a jit_get_global_mnh lookup at function entry. The emitter
// resolves each global ONCE per function: every access then shares one base pointer, so LLVM sees
// xs[j] and xs[j + 1] as adjacent (vectorizable, alias-analyzable) and the runtime pays one hash
// lookup per global per call instead of one per access.

def private das_exe() : string {
    let argv <- get_command_line_arguments()
    return find(to_generic_path(argv[0]), "/") < 0 ? argv[0] : get_full_file_name(normalize(argv[0]))
}

def private tail(s : string) : string {
    return slice(s, max(0, length(s) - 400))
}

//! the dumped IR of one function, by its dumped name - mangled (`branchy Cb`) unless the signature is nullary; signature to closing brace
def private function_ir(dump : string; name : string) : string {
    let start = find(dump, "`{name}\"(")
    if (start < 0) {
        return ""
    }
    let body = slice(dump, start)
    let close = find(body, "\n}")
    return close < 0 ? body : slice(body, 0, close)
}

//! the entry block of a dumped function: everything before its first branch
def private entry_block_ir(fn_ir : string) : string {
    let br = find(fn_ir, "\n  br ")
    return br < 0 ? fn_ir : slice(fn_ir, 0, br)
}

def private count_of(s : string; needle : string) : int {
    var n = 0
    var at = find(s, needle, 0)
    while (at >= 0) {
        n++
        at = find(s, needle, at + length(needle))
    }
    return n
}

[test]
def test_global_lookup_once_per_function(t : T?) {
    t |> run("four accesses to two globals emit one lookup per global") @(t : T?) {
        let root = "modules/dasLLVM/tests/_global_lookup_root.das"
        var out = ""
        //! -jit-no-cache: the dump only exists when codegen runs; -no-module-cache: nothing is written beside the fixture
        let rc = run_and_capture([das_exe(), "-no-module-cache", "-jit", "-jit-no-cache", root, "--", "--jit-dump"], out, 300.0)
        t |> equal(0, rc, "the fixture runs under -jit: {tail(out)}")
        t |> success(find(out, "FOUR_SUMS ") >= 0, "the fixture printed its sum: {tail(out)}")
        let ir = function_ir(out, "four_sums")
        t |> success(!empty(ir), "the dump carries four_sums: {tail(out)}")
        let lookups = count_of(ir, "@jit_get_global_mnh(")
        t |> equal(2, lookups, "one jit_get_global_mnh per global (xs, ys) in four_sums, not one per access:\n{ir}")
        //! the lazy half: a global written on one branch only is never looked up ahead of the branch - the
        //! entry block carries no constant-hash lookup (LLVM may fold both branches into ONE lookup whose
        //! hash is a select, which still probes once per call and never for the untaken global)
        let bir = function_ir(out, "branchy Cb")
        t |> success(!empty(bir), "the dump carries branchy: {tail(out)}")
        let total = count_of(bir, "@jit_get_global_mnh(")
        t |> success(total >= 1 && total <= 2, "at most one lookup per branch global (ga, gb) in branchy, got {total}:\n{bir}")
        let entry = entry_block_ir(bir)
        let hoisted = count_of(entry, "@jit_get_global_mnh(") - count_of(entry, "@jit_get_global_mnh(i64 %")
        t |> equal(0, hoisted, "no constant-hash lookup in branchy's entry block - an untaken branch pays nothing:\n{bir}")
    }
    t |> run("a solid-context global is one base load per function and no lookup") @(t : T?) {
        let root = "modules/dasLLVM/tests/_global_lookup_solid_root.das"
        var out = ""
        let rc = run_and_capture([das_exe(), "-no-module-cache", "-jit", "-jit-no-cache", root, "--", "--jit-dump"], out, 300.0)
        t |> equal(0, rc, "the solid fixture runs under -jit: {tail(out)}")
        t |> success(find(out, "FOUR_SUMS ") >= 0, "the solid fixture printed its sum: {tail(out)}")
        let ir = function_ir(out, "four_sums")
        t |> success(!empty(ir), "the dump carries four_sums: {tail(out)}")
        t |> equal(0, count_of(ir, "@jit_get_global_mnh("), "a solid-context global never goes through the hash lookup:\n{ir}")
        t |> equal(1, count_of(ir, "load ptr, ptr"), "one load of the context's globals base serves every access of both globals:\n{ir}")
    }
}
