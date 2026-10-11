options gen2

module llvm_exe_shared_addr_lib shared

// Library-side fixture for llvm_exe_shared_addr.das (no [test] on purpose): a shared module
// whose global initializer takes a function's address, the shape the icon catalogs and
// dispatch tables of daslib modules use.

struct Ctx {
    n : int
}

def private draw_a(var ctx : Ctx) {
    ctx.n++
}

def private draw_b(var ctx : Ctx) {
    ctx.n += 10
}

struct private Def {
    name : string
    draw : function<(var ctx : Ctx) : void>
}

let private CATALOG <- [
    Def(name = "a", draw = @@ draw_a),
    Def(name = "b", draw = @@ draw_b)
]

let private DRAW_B = @@ draw_b

var private DISPATCH : table<string; function<(var ctx : Ctx) : void>>

def public draw_named(name : string; var ctx : Ctx) {
    if (empty(DISPATCH)) {
        for (d in CATALOG) {
            DISPATCH[d.name] = d.draw
        }
    }
    if (key_exists(DISPATCH, name)) {
        invoke(DISPATCH[name], ctx)
    }
}

def public draw_b_direct(var ctx : Ctx) {
    invoke(DRAW_B, ctx)
}
