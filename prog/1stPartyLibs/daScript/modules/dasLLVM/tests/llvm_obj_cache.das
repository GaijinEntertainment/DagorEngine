options gen2
options no_aot

require dastest/testing_boost
require daslib/fio
require daslib/module_path
require _llvm_cache_common

def private check_round(t : T?; round, client : string; cached, acc : int) : SplitRound {
    let r = split_round(client)
    t |> success(r.ok, "{round}: the backend ran")
    t |> equal(r.partitions, 3, "{round}: one partition per das module")
    t |> equal(r.cached, cached, "{round}: cached objects")
    t |> equal(r.acc, acc, "{round}: the cross-partition answer")
    return r
}

[test]
def test_jit_obj_cache(t : T?) {
    if (!jit_enabled() || !das_is_dll_build()) {
        t |> skip("needs -jit and a DLL daslang build")
        return
    }
    let client = stage_split_client(t, "das_objcache")
    if (empty(client)) return
    let r1 = check_round(t, "round 1 cold", client, 0, 340)
    t |> success(copy_fixture(get_this_module_dir(), dir_name(client), "_llvm_obj_cache_client.das", "var a = 0", "var a = 1000"),
        "round 2 edit client")
    let r2 = check_round(t, "round 2 tail edit", client, 2, 1340)
    var err : string
    rmdir_rec(dir_name(r1.output_path), err)
    rmdir_rec(dir_name(r2.output_path), err)
    rmdir_rec(dir_name(client), err)
}
