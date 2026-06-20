options gen2
expect 30347

// named-argument call can't take a piped block (parse error, was a silent drop)
struct Holder {
    v : int
}

def setv(h : Holder; x : int; blk : block<() : void>) {
    invoke(blk)
}

[export]
def main {
    var h = Holder(v = 1)
    h.setv([x = 1]) $ {
    }
}
