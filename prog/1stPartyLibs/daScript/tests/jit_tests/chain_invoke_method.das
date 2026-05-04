options gen2
require dastest/testing_boost

var ctor_count = 0
var to_upper_count = 0
var utf8_count = 0

class MyString {
    def MyString {
        ctor_count ++
    }
    def to_upper() : MyString? {
        to_upper_count ++
        return addr(self)
    }
    def utf8() : MyString? {
        utf8_count ++
        return addr(self)
    }
}

[test]
def test_chain_invoke_methods(t : T?) {
    t |> success(!jit_enabled() || is_jit_function(@@MyString))
    let res = new MyString()->to_upper()->utf8()
    t |> equal(1, ctor_count)
    t |> equal(1, to_upper_count)
    t |> equal(1, utf8_count)
}

