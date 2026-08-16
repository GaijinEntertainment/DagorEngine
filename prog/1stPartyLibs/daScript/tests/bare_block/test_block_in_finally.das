options gen2
require dastest/testing_boost public
require daslib/defer

var private g_trace = 0

def private block_used_in_defer() {
    let bump = $ {
        g_trace ++
    }
    defer() {
        invoke(bump)    // block invoked from finally - hoisted with its backing in AOT
    }
    invoke(bump)
}

def private block_after_defer() {
    defer() {
        g_trace ++
    }
    let bump = $ {      // block declared after the finally was armed
        g_trace ++
    }
    invoke(bump)
}

def private block_after_early_return(early : bool) {
    defer() {
        invoke(bump)
    }
    return if (early)
    let bump = $ {
        g_trace ++
    }
}

def private block_in_plain_finally() {
    {
        let bump = $ {
            g_trace ++
        }
        invoke(bump)
    } finally {
        invoke(bump)
    }
}

def private block_in_loop() {
    for (i in range(3)) {
        {
            let bump = $ {
                g_trace += i + 1    // captures the loop iterator
            }
            defer() {
                invoke(bump)
            }
            invoke(bump)
        }
    }
}

def private block_sees_later_mutation() {
    var x = 1
    let bump = $ {
        g_trace += x    // blocks capture by reference - reads x at invoke time
    }
    defer() {
        invoke(bump)
    }
    x = 10
}

[test]
def test_block_in_finally(t : T?) {
    t |> run("block invoked from defer") @(t : T?) {
        g_trace = 0
        block_used_in_defer()
        t |> equal(g_trace, 2)
    }
    t |> run("block declared after defer") @(t : T?) {
        g_trace = 0
        block_after_defer()
        t |> equal(g_trace, 2)
    }
    t |> run("block declared after an early-return guard") @(t : T?) {
        g_trace = 0
        block_after_early_return(false)
        t |> equal(g_trace, 1)
    }
    t |> run("block invoked from plain finally") @(t : T?) {
        g_trace = 0
        block_in_plain_finally()
        t |> equal(g_trace, 2)
    }
    t |> run("block in a loop invoked from defer") @(t : T?) {
        g_trace = 0
        block_in_loop()
        t |> equal(g_trace, 12)
    }
    t |> run("finally-invoked block sees later mutations") @(t : T?) {
        g_trace = 0
        block_sees_later_mutation()
        t |> equal(g_trace, 10)
    }
}
