options gen2

require daslib/safe_addr
require _jit_lib_probe_c

[export]
def main() : int {
    var ctx = jit_lib_probe_create()
    if (ctx == null) {
        print("create failed: {jit_lib_probe_last_error(null)}\n")
        return 1
    }
    var bad = 0
    var p = jit_lib_probe_Pair(lo = 10, hi = 20)
    var out = jit_lib_probe_Pair(lo = 0, hi = 0)
    jit_lib_probe_widen(ctx, safe_addr(p), safe_addr(out))
    if (out.lo != 9 || out.hi != 21) {
        print("widen answered ({out.lo},{out.hi})\n")
        bad++
    }
    let seen = jit_lib_probe_tally(ctx)
    if (seen != 1) {
        print("tally answered {seen}\n")
        bad++
    }
    let mapped = jit_lib_probe_mapped(ctx, 5)
    if (mapped != 15) {
        print("mapped answered {mapped}\n")
        bad++
    }
    jit_lib_probe_destroy(ctx)
    return bad
}
