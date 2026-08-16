options gen2
expect 30341:4

require dastest/testing_boost public

// 'mid' has no default, so the piped block cannot pad across it to reach 'body'
def blocked(tag : int; mid : int; opt : int = 0; body : block<() : int>) : int {
    return tag + mid + opt + invoke(body)
}

// no block parameter at all -> the piped block has nowhere to land
def noblock(tag : int; opt : int = 0) : int {
    return tag + opt
}

// genuine class methods have the same limits: a non-default param that is not provided blocks
// padding, and a method with no block param has nowhere for the piped block to land
class Widget {
    d : int
    // 'mid' has no default and is not provided positionally -> can't pad across it to reach 'body'
    def blocked(a : int; mid : int; opt : int = 0; body : block<() : int>) : int {
        return d + a + mid + opt + invoke(body)
    }
    def noblock(tag : int; opt : int = 0) : int {
        return d + tag + opt
    }
}

[test]
def test_named_piped_cannot_pad(t : T?) {
    blocked(5, opt = 3) {
        return 1
    }
    noblock(5, opt = 3) {
        return 1
    }
    var w = new Widget(d = 1)
    w.blocked(5, opt = 3) {
        return 1
    }
    w.noblock(5, opt = 3) {
        return 1
    }
    unsafe { delete w }
}
