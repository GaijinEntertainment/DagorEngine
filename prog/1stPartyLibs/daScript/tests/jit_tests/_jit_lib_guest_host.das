options gen2

require _jit_lib_guest_c

[export]
def main() : int {
    var ctx = jit_lib_guest_create()
    if (ctx == null) {
        print("create failed: {jit_lib_guest_last_error(null)}\n")
        return 1
    }
    var bad = 0
    let answered = jit_lib_guest_probe(ctx, 35)
    if (answered != 42) {
        print("probe answered {answered}, last_error `{jit_lib_guest_last_error(ctx)}`\n")
        bad++
    }
    jit_lib_guest_destroy(ctx)
    return bad
}
