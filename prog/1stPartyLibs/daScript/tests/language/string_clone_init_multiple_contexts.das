options gen2
options multiple_contexts

require UnitTest
require dastest/testing_boost public
require _string_clone_init_helper

var global_source = get_das_root()
var global_clone := global_source

[test]
def test_string_clone_local_init_multiple_contexts(t : T?) {
    t |> run("string := local initialization clones with multiple_contexts") @(t : T?) {
        let source = get_das_root()
        let cloned := source
        t |> equal(cloned, source)
        t |> success(!same_string_storage(cloned, source))
    }
}

[test]
def test_string_clone_global_init_multiple_contexts(t : T?) {
    t |> run("string := global initialization clones with multiple_contexts") @(t : T?) {
        t |> equal(global_clone, global_source)
        t |> success(!same_string_storage(global_clone, global_source))
    }
}
