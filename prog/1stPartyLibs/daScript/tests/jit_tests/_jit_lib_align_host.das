options gen2

require daslib/safe_addr
require _jit_lib_align_c

[export]
def main() : int {
    var ctx = jit_lib_align_create()
    if (ctx == null) {
        print("create failed: {jit_lib_align_last_error(null)}\n")
        return 1
    }
    var bad = 0
    var w = jit_lib_align_Wide(a = int8(0), b = 0l, c = 0.0)
    jit_lib_align_make(ctx, 7, safe_addr(w))
    if (w.a != int8(7) || w.b != 7000000000l || w.c != 7.5) {
        print("make answered a={w.a} b={w.b} c={w.c}\n")
        bad++
    }
    jit_lib_align_destroy(ctx)
    return bad
}
