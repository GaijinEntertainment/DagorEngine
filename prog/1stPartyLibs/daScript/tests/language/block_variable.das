options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct BlockVarFoo {
    a : int
}

var g_block_var = 0

[test]
def test_block_variable(t : T?) {
    t |> run("void no-arg block") @(t : T?) {
        let localVar = BlockVarFoo(a = 34)
        var localVoidNoArgBlock = $ {
            g_block_var = localVar.a
        }
        g_block_var = 0
        localVoidNoArgBlock |> invoke()
        t |> equal(g_block_var, localVar.a)
    }
    t |> run("void with-arg block") @(t : T?) {
        let localVar = BlockVarFoo(a = 34)
        var localVoidArgBlock = $(x : int) : void {
            g_block_var = localVar.a + x
        }
        g_block_var = 0
        localVoidArgBlock |> invoke(1)
        t |> equal(g_block_var, localVar.a + 1)
    }
    t |> run("result no-arg block") @(t : T?) {
        let localVar = BlockVarFoo(a = 34)
        let localResultNoArgBlock = $ {
            g_block_var = localVar.a
            return localVar.a
        }
        g_block_var = 0
        let res1 = localResultNoArgBlock |> invoke()
        t |> equal(g_block_var, localVar.a)
        t |> equal(res1, localVar.a)
    }
    t |> run("result with-arg block") @(t : T?) {
        let localVar = BlockVarFoo(a = 34)
        let localResultArgBlock = $(x : int) : int {
            g_block_var = localVar.a + x
            return localVar.a + x
        }
        g_block_var = 0
        let res2 = localResultArgBlock |> invoke(3)
        t |> equal(g_block_var, localVar.a + 3)
        t |> equal(res2, localVar.a + 3)
    }
    t |> run("cmres result no-arg block") @(t : T?) {
        let localVar = BlockVarFoo(a = 34)
        let localCmresResultNoArgBlock = $ {
            g_block_var = localVar.a
            return localVar
        }
        g_block_var = 0
        let res3 = localCmresResultNoArgBlock |> invoke()
        t |> equal(g_block_var, localVar.a)
        t |> equal(res3.a, localVar.a)
    }
    t |> run("cmres result with-arg block") @(t : T?) {
        let localVar = BlockVarFoo(a = 34)
        let localCmresResultArgBlock = $(x : int) : BlockVarFoo {
            g_block_var = localVar.a + x
            return BlockVarFoo(a = localVar.a + x)
        }
        g_block_var = 0
        let res4 = localCmresResultArgBlock |> invoke(3)
        t |> equal(g_block_var, localVar.a + 3)
        t |> equal(res4.a, localVar.a + 3)
    }
}
