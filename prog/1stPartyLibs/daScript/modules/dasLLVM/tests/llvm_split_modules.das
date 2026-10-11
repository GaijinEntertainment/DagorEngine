options gen2
options no_aot

require dastest/testing_boost
require daslib/fio
require _llvm_cache_common

def private check_answer(t : T?; round : string; r : SplitRound) {
    t |> success(r.ok, "{round}: the backend ran")
    t |> equal(r.acc, 340, "{round}: cross-partition direct calls")
    t |> equal(r.lo, 19, "{round}: the cmres cross-call's low half")
    t |> equal(r.hi, 36, "{round}: the cmres cross-call's high half")
    t |> equal(r.hit, 33, "{round}: the shared table-method glob")
    t |> equal(r.miss, -1, "{round}: the table-method glob's miss answer")
}

[test]
def test_jit_split_modules(t : T?) {
    if (!jit_enabled() || !das_is_dll_build()) {
        t |> skip("needs -jit and a DLL daslang build")
        return
    }
    let client = stage_split_client(t, "das_split")
    if (empty(client)) return
    let r1 = split_round(client)
    check_answer(t, "round 1 split", r1)
    t |> equal(r1.partitions, 3, "round 1 split: one partition per das module")
    t |> equal(r1.cached, 0, "round 1 split: nothing was cached to link")
    let r2 = split_round(client)
    check_answer(t, "round 2 dll cache hit", r2)
    t |> equal(r2.partitions, 0, "round 2 dll cache hit: no partition re-emitted")
    t |> strictEqual(r2.output_path, r1.output_path, "round 2 dll cache hit: the same content-addressed DLL")
    var err : string
    rmdir_rec(dir_name(r1.output_path), err)
    rmdir_rec(dir_name(client), err)
}
