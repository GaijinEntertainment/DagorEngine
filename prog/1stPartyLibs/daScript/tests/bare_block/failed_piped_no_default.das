options gen2
expect 30341

// `b` has no default - the piped block can't shift across it
def no_default(a : int; b : float; blk : block<() : void>) {
    invoke(blk)
}

[export]
def main {
    no_default(1) $ {
    }
}
