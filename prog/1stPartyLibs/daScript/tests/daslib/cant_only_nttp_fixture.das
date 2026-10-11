options gen2
options no_aot
expect 50501
require daslib/only_nttp
require daslib/fio

[only_nttp, sideeffects]
def kernel {
    var total = 0
    for (_i in range(10)) {
        total += length(get_env_variable("PATH"))
    }
    return total
}

[export]
def main {
    kernel()
}
