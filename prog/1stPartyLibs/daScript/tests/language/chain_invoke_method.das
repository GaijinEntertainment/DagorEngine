options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

var g_chain_ctor_count = 0
var g_chain_upper_count = 0
var g_chain_utf8_count = 0

class ChainString {
    def ChainString {
        g_chain_ctor_count++
    }
    def to_upper() : ChainString? {
        g_chain_upper_count++
        return addr(self)
    }
    def utf8() : ChainString? {
        g_chain_utf8_count++
        return addr(self)
    }
}

[test]
def test_chain_invoke_method(t : T?) {
    t |> run("new->method1->method2 chaining") @(t : T?) {
        g_chain_ctor_count = 0
        g_chain_upper_count = 0
        g_chain_utf8_count = 0
        let res = new ChainString()->to_upper()->utf8()
        t |> equal(g_chain_ctor_count, 1)
        t |> equal(g_chain_upper_count, 1)
        t |> equal(g_chain_utf8_count, 1)
    }
}
