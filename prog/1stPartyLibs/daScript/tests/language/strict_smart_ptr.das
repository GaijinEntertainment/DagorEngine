options gen2
options strict_smart_pointers

require dastest/testing_boost public

require daslib/ast

struct Foo {
    a : ExpressionPtr
}

[test]
def test_resize_array(t : T?) {
    t |> run("emplace") @(t : T?) {
        ast_gc_guard() {
            var a : array<ExpressionPtr>
            var b <- new ExprOp2(op := "==")
            a |> emplace(b)
            t |> equal(1, length(a))
            t |> equal(null, b)
        }
    }
    t |> run("array init") @(t : T?) {
        ast_gc_guard() {
            var b : ExpressionPtr <- new ExprOp2(op := "==")
            var a <- array<ExpressionPtr>(b)
            t |> equal(1, length(a))
            // Note: GC-managed ExpressionPtr is not zeroed by array construction (pass by value)
        }
        ast_gc_guard() {
            var b : ExpressionPtr <- new ExprOp2(op := "==")
            var a <- array<ExpressionPtr>(move_to_local(b))
            t |> equal(1, length(a))
            // Note: GC-managed ExpressionPtr is not zeroed by move_to_local
        }
    }
        /*
        if true
            var b : ExpressionPtr <- new [[ExprOp2() op:="=="]]
            var a <- [{ExpressionPtr b}]    // 40103: can't create array from smart pointer by value. did u mean [{Type[1] value}] instead of [{Type value}]
            t |> equal(1, length(a))
            t |> equal(null, b)
        */
    t |> run("struct init") @(t : T?) {
        ast_gc_guard() {
            var b : ExpressionPtr <- new ExprOp2(op := "==")
            var a <- Foo(a <- b)
            // Note: GC-managed ExpressionPtr is not zeroed by struct construction move
        }
    }
}


