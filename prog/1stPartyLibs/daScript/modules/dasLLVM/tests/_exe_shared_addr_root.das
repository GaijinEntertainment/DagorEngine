options gen2

require llvm_exe_shared_addr_lib

// A function address taken in a shared module's global initializer reaches the standalone exe:
// the catalog's entries dispatch through a table, and the bare function-pointer global invokes.

[export]
def main {
    var ctx = Ctx(n = 0)
    draw_named("a", ctx)
    draw_named("b", ctx)
    print("SHARED_ADDR_TABLE {ctx.n}\n")
    draw_b_direct(ctx)
    print("SHARED_ADDR_DIRECT {ctx.n}\n")
}
