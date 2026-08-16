options gen2

require UnitTest
require dastest/testing_boost public
require _string_clone_init_helper

def borrow_string(src : string) : string# {
    unsafe {
        return reinterpret<string#>(src)
    }
}

[test]
def test_temporary_string_clone_initialization(t : T?) {
    t |> run("string# := initialization retains a cloned string") @(t : T?) {
        let source = get_das_root()
        let retained := borrow_string(source)
        t |> equal(retained, source)
        t |> success(!same_string_storage(retained, source))
    }
}
