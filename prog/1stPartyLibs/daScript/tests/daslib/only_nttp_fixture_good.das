options gen2
options no_aot
require daslib/only_nttp
require strings

[only_nttp, sideeffects]
def kernel {
    var summ = 0.0
    for (_i in range(10)) {
        summ += to_float("1.5")
    }
    return summ
}

[export]
def main {
    kernel()
}
