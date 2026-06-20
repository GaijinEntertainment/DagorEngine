options gen2
expect 30341

// piped block signature does not match the block parameter
def typed_block(a : int; b : float = 1.0; blk : block<(x : int) : void>) {
    invoke(blk, a)
}

[export]
def main {
    typed_block(1) $(x : string) {
    }
}
