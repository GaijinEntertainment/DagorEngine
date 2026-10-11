options gen2
options rtti
options aot_macros
options indenting = 4

// Lowered quotes in the program's MAIN (anonymous) module: entities declared here can't
// be reconstructed by module-name lookup, so TypeDecls referencing them must fall back
// to the by-name alias shape (make_alias_type_decl) and resolve at splice-site re-infer.

require dastest/testing_boost public
require daslib/ast
require daslib/templates_boost
require _quote_lit_macro

enum LocalColor {
    red
    green
}

struct LocalPair {
    a : int
    b : string
}

[test]
def test_lowered_local_entities(t : T?) {
    t |> run("local enum type reconstructs as by-name alias") @(t : T?) {
        ast_gc_guard() {
            var ty = qmacro_type(type<LocalColor>)
            t |> equal(describe(ty), "LocalColor")
        }
    }
    t |> run("local struct type reconstructs as by-name alias") @(t : T?) {
        ast_gc_guard() {
            var ty = qmacro_type(type<LocalPair>)
            t |> equal(describe(ty), "LocalPair")
        }
    }
    t |> run("lowered splice still substitutes") @(t : T?) {
        ast_gc_guard() {
            let nm = "renamed"
            var e = qmacro($c(nm)(1 + 2))
            t |> equal(describe_expression(e), "renamed(1 + 2)")
        }
    }
    t |> run("a quoted literal keeps adapting after lowering") @(t : T?) {
        let u = 5u
        let l = 5l
        t |> equal(plus_one_lit(u), 6u)
        t |> equal(plus_one_lit(l), 6l)
        t |> equal(plus_spliced_lit(u), 7u)
        t |> equal(plus_spliced_lit(l), 7l)
    }
}
