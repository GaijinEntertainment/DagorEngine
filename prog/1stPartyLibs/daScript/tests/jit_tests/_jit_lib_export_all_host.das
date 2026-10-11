options gen2

require _jit_lib_probe_all_c

[export]
def main() : int {
    var ctx = jit_lib_probe_all_create()
    if (ctx == null) {
        print("create failed: {jit_lib_probe_all_last_error(null)}\n")
        return 1
    }
    var bad = 0
    let bumped = jit_lib_probe_all_helper_public(ctx, 41)
    if (bumped != 42) {
        print("helper_public answered {bumped}\n")
        bad++
    }
    jit_lib_probe_all_destroy(ctx)
    return bad
}
