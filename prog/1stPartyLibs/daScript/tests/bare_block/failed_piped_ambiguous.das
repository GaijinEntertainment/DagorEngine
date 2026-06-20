options gen2
expect 30341

// equal padding, indistinguishable prefix - ambiguous
def amb(a : int; b : int = 1; blk : block<() : int>) : int {
    return invoke(blk)
}

def amb(a : int; c : float = 2.0; blk : block<() : int>) : int {
    return invoke(blk)
}

[export]
def main {
    let r = amb(1) $ {
        return 7;
    }
}
