options gen2

require daslib/safe_addr
require strings
require _bindlib_c

[export]
def main() : int {
    var ctx = bindlib_create()
    if (ctx == null) {
        print("create failed: {bindlib_last_error(null)}\n")
        return 1
    }
    var bad = 0
    if (bindlib_bump(ctx) != 1 || bindlib_bump(ctx) != 2) {
        print("the library's global does not persist across calls\n")
        bad++
    }
    var p = bindlib_Pair(lo = 10, hi = 20)
    var out = bindlib_Pair(lo = 0, hi = 0)
    bindlib_widen(ctx, safe_addr(p), safe_addr(out))
    if (out.lo != 9 || out.hi != 21) {
        print("widen answered ({out.lo},{out.hi})\n")
        bad++
    }
    let greeting = bindlib_greet(ctx, "mundo")
    if (greeting != "hola mundo") {
        print("greet answered `{greeting}`\n")
        bad++
    }
    if (bindlib_double_or_raise(ctx, 21) != 42) {
        print("a plain call answered wrong\n")
        bad++
    }
    let raised = bindlib_double_or_raise(ctx, -5)
    let err = bindlib_last_error(ctx)
    if (raised != 0 || find(err, "negative: -5") < 0) {
        print("a raising call answered {raised}, last_error `{err}`\n")
        bad++
    }
    if (bindlib_double_or_raise(ctx, 3) != 6) {
        print("the instance is unusable after a panic\n")
        bad++
    }
    bindlib_destroy(ctx)
    return bad
}
