options gen2
options no_aot

require dastest/testing_boost
require daslib/module_path
require _llvm_cache_common

[test]
def test_jit_compile_only(t : T?) {
    if (!jit_enabled()) {
        t |> success(true)
        return
    }
    var got = -1
    compile_for_jit("{get_this_module_dir()}/_llvm_compile_only_client.das", false) $(var program) {
        simulate(program) $(sok, context, _serrors) {
            if (!sok) {
                return
            }
            var cfg : JitCliOptions
            cfg.compile_only = some(true)
            let r = run_jit(get_ptr(program), get_ptr(context), cfg)
            t |> success(r.ok, "the jit backend ran to its escape")
            t |> success(r.compile_only, "the compile-only path was taken")
            t |> equal(r.partitions, 0, "compile-only emits no partition")
            t |> equal(r.cached, 0, "compile-only reads no cached object")
            unsafe {
                invoke_in_context(context, "answer", addr(got))
            }
        }
    }
    t |> equal(got, 402, "interpreted execution still produces the right answer")
}
