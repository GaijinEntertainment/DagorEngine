options gen2
options rtti
options indenting = 4
options aot_macros

// Twin fixture for the quote-lowering A/B suite: _quote_shapes runs quotes through
// the interpreter SimNode path (clone of the parsed tree), _quote_shapes_lowered
// (`options aot_macros`) through the lowered reconstruction code. The two files are
// IDENTICAL except for the module name and the `options aot_macros` line — keep
// the bodies in sync.

module _quote_shapes_lowered shared

require daslib/ast
require daslib/templates_boost
require daslib/strings_boost
require _quote_entities // nolint:STYLE030 referenced only inside quoted trees (parse-time type resolution), invisible to symbol-use

def public shape_consts : string {
    var res = ""
    ast_gc_guard() {
        var e1 = quote("a\nb\t\"q\" — π λ")
        var e2 = quote(13.5lf)
        var e3 = quote(0xff)
        res = "{describe_expression(e1)}|{describe_expression(e2)}|{describe_expression(e3)}"
    }
    return res
}

def public shape_call : string {
    var res = ""
    ast_gc_guard() {
        var e = quote(foo(1 + 2, "bar"))
        res = describe_expression(e)
    }
    return res
}

def public shape_splices : string {
    var res = ""
    ast_gc_guard() {
        let n = "spliced"
        var e = qmacro($c(n)(true, 13.5, [1, 2, 3]))
        res = describe_expression(e)
    }
    return res
}

def public shape_ident_splice : string {
    var res = ""
    ast_gc_guard() {
        let nm = "counter"
        var e = qmacro($i(nm) + 1)
        res = describe_expression(e)
    }
    return res
}

def public shape_block : string {
    var res = ""
    ast_gc_guard() {
        var b = qmacro_block() {
            var x = 5
            for (i in range(x)) {
                print("{i}\n")
            }
        }
        res = describe_expression(b)
    }
    return res
}

def public shape_finally_block : string {
    var res = ""
    ast_gc_guard() {
        var b = qmacro_block() {
            var arr : array<int>
            {
                arr |> push(1)
            } finally {
                delete arr
            }
        }
        res = describe_expression(b)
    }
    return res
}

def public shape_make_struct : string {
    var res = ""
    ast_gc_guard() {
        var e = qmacro(Foo(a = 1, b = "two"))
        res = describe_expression(e)
    }
    return res
}

def public shape_named_enum_type : string {
    var res = ""
    ast_gc_guard() {
        var t = qmacro_type(type<Color>)
        res = describe(t)
    }
    return res
}

def public shape_named_struct_type : string {
    var res = ""
    ast_gc_guard() {
        var t = qmacro_type(type<Pair>)
        res = describe(t)
    }
    return res
}

def public shape_nested_type : string {
    var res = ""
    ast_gc_guard() {
        var t = qmacro_type(type<table<string; array<int>>>)
        res = describe(t)
    }
    return res
}

def public shape_table_literal : string {
    var res = ""
    ast_gc_guard() {
        var e = qmacro({ "k1" => 1, "k2" => 2 })
        res = describe_expression(e)
    }
    return res
}

def public shape_tuple_literal : string {
    var res = ""
    ast_gc_guard() {
        var e = qmacro((1, "a", 2.5))
        res = describe_expression(e)
    }
    return res
}

def public shape_lambda_capture : string {
    var res = ""
    ast_gc_guard() {
        var e = qmacro(@ capture(:= y) (x : int) { return x + y; })
        res = describe_expression(e)
    }
    return res
}

def public shape_generator : string {
    var res = ""
    ast_gc_guard() {
        var e = qmacro($ { yield 1; yield 2; })
        res = describe_expression(e)
    }
    return res
}

def public shape_block_to_array : string {
    var res = ""
    ast_gc_guard() {
        var arr <- qmacro_block_to_array() {
            foo()
            bar(1, "x")
        }
        var parts <- [for (e in arr); describe_expression(e)]
        res = parts |> join(";")
        delete parts
    }
    return res
}
