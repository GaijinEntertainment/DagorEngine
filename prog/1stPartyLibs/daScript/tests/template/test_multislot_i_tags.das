options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// `$i(...)` splices in EVERY slot of a multi-name list (daslang #3096) — for-loop
// iterators, comprehensions, let declarations, and shared-type block argument lists.
// Runtime execution of multi-slot-emitted loops is pinned by tests/linq (the zip and
// table-kv adapters emit through these shapes).

// ── for-loop: both iterator slots tagged ────────────────────────────────

[test]
def test_for_both_slots(t : T?) {
    ast_gc_guard() {
        let kName = "kk"
        let vName = "vv"
        let srcName = "tab"
        var blk = qmacro_block() {
            for ($i(kName), $i(vName) in keys($i(srcName)), values($i(srcName))) {
                probe_sink($i(kName), $i(vName))
            }
        }
        var loop = (blk as ExprBlock).list[0]
        t |> success(loop is ExprFor, "should build an ExprFor")
        var pFor = loop as ExprFor
        t |> equal(length(pFor.iterators), 2)
        t |> success(pFor.iterators[0] == "kk", "slot 0 should splice")
        t |> success(pFor.iterators[1] == "vv", "slot 1 should splice")
        t |> success(pFor.iteratorsTags[0] == null && pFor.iteratorsTags[1] == null,
            "no tags should survive substitution")
        t |> equal(length(pFor.sources), 2)
    }
}

// ── comprehension: tag in the second slot ───────────────────────────────

[test]
def test_comprehension_slot1(t : T?) {
    ast_gc_guard() {
        let kName = "kk"
        let vName = "vv"
        let srcName = "tab"
        var compr = qmacro([for ($i(kName), $i(vName) in keys($i(srcName)), values($i(srcName))); $i(vName)])
        t |> success(compr is ExprArrayComprehension, "should build a comprehension")
        var pFor = (compr as ExprArrayComprehension).exprFor as ExprFor
        t |> equal(length(pFor.iterators), 2)
        t |> success(pFor.iterators[0] == "kk", "slot 0 should splice")
        t |> success(pFor.iterators[1] == "vv", "slot 1 should splice")
    }
}

// ── let declaration: both names tagged / literal+tag mix ────────────────

[test]
def test_let_both_slots(t : T?) {
    ast_gc_guard() {
        let kName = "kk"
        let vName = "vv"
        var blk = qmacro_block() {
            var $i(kName), $i(vName) : int
        }
        var letE = (blk as ExprBlock).list[0]
        t |> success(letE is ExprLet, "should build an ExprLet")
        var pLet = letE as ExprLet
        t |> equal(length(pLet.variables), 2)
        t |> success(pLet.variables[0].name == "kk", "slot 0 should splice")
        t |> success(pLet.variables[1].name == "vv", "slot 1 should splice")
    }
}

[test]
def test_let_literal_then_tag(t : T?) {
    ast_gc_guard() {
        let vName = "vv"
        var blk = qmacro_block() {
            var fixed, $i(vName) : int
        }
        var pLet = (blk as ExprBlock).list[0] as ExprLet
        t |> equal(length(pLet.variables), 2)
        t |> success(pLet.variables[0].name == "fixed", "literal slot 0 stays")
        t |> success(pLet.variables[1].name == "vv", "tagged slot 1 should splice")
    }
}

// ── block argument list: shared-type group with both names tagged ───────

[test]
def test_block_args_shared_type(t : T?) {
    ast_gc_guard() {
        let aName = "kk"
        let bName = "vv"
        var call = qmacro(foo() $($i(aName), $i(bName) : int) {
            probe_sink($i(aName), $i(bName))
        })
        var blk = ((call as ExprCall).arguments[0] as ExprMakeBlock)._block as ExprBlock
        t |> equal(length(blk.arguments), 2)
        t |> success(blk.arguments[0].name == "kk", "arg 0 should splice")
        t |> success(blk.arguments[1].name == "vv", "arg 1 should splice")
    }
}
