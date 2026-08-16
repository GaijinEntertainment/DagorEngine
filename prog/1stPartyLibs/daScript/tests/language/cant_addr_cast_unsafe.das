// addr<T?>(x) without unsafe: both gates fire (address + reinterpret);
// the exact-type form folds the cast away but the addr gate still holds
options gen2
expect 31000:2, 31004    // unsafe_address, unsafe_cast

def test {
    var v = float3(1., 2., 3.)
    var p = addr<float?>(v)     // 31000 + 31004
    debug(p)
    var i = 42
    var pi = addr<int?>(i)      // exact type: cast folds, still 31000
    debug(pi)
}

[export]
def main {
    test()
}
