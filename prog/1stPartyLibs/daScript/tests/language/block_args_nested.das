options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

def passthrough(a : int; blk : block<(b : int) : void>) {
    invoke(blk, a)
}

def passthroughref(a : int&; blk : block<(b : int&) : void>) {
    invoke(blk, a)
}

def passthroughptr(a : int?; blk : block<(b : int?) : void>) {
    invoke(blk, a)
}

struct BlockFoo {
    v : int
}

def passthroughFoo(a : BlockFoo; blk : block<(b : BlockFoo) : void>) {
    invoke(blk, a)
}

[test]
def test_block_args_nested(t : T?) {
    t |> run("nested int blocks") @(t : T?) {
        passthrough(1) <| $(a) {
            t |> equal(a, 1)
            passthrough(2) <| $(b) {
                t |> equal(a, 1)
                t |> equal(b, 2)
                passthrough(3) <| $(c) {
                    t |> equal(a, 1)
                    t |> equal(b, 2)
                    t |> equal(c, 3)
                }
            }
        }
    }
    t |> run("nested ref blocks") @(t : T?) {
        var A = 1
        var B = 2
        var C = 3
        passthroughref(A) <| $(a) {
            t |> equal(a, 1)
            passthroughref(B) <| $(b) {
                t |> equal(a, 1)
                t |> equal(b, 2)
                passthroughref(C) <| $(c) {
                    t |> equal(a, 1)
                    t |> equal(b, 2)
                    t |> equal(c, 3)
                }
            }
        }
    }
    t |> run("nested pointer blocks") @(t : T?) {
        var A = 1
        var B = 2
        var C = 3
        unsafe {
            passthroughptr(addr(A)) <| $(a) {
                t |> equal(*a, 1)
                passthroughptr(addr(B)) <| $(b) {
                    t |> equal(*a, 1)
                    t |> equal(*b, 2)
                    passthroughptr(addr(C)) <| $(c) {
                        t |> equal(*a, 1)
                        t |> equal(*b, 2)
                        t |> equal(*c, 3)
                    }
                }
            }
        }
    }
    t |> run("nested struct blocks") @(t : T?) {
        passthroughFoo(BlockFoo(v = 1)) <| $(a) {
            t |> equal(a.v, 1)
            passthroughFoo(BlockFoo(v = 2)) <| $(b) {
                t |> equal(a.v, 1)
                t |> equal(b.v, 2)
                passthroughFoo(BlockFoo(v = 3)) <| $(c) {
                    t |> equal(a.v, 1)
                    t |> equal(b.v, 2)
                    t |> equal(c.v, 3)
                }
            }
        }
    }
}
