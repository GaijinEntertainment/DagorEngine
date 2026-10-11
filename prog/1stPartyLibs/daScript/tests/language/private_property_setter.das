// private property operators of a class stay callable inside the declaring module
// verifies the public getter, the in-module setter path, and a private getter behind a public method
options gen2
require UnitTest
require dastest/testing_boost public

require _private_property_helper

[test]
def test_private_property_in_module(t : T?) {
    var g = new Gate()
    t |> equal(false, g.flag)
    set_flag_from_helper(g, true)
    t |> equal(true, g.flag)
    t |> equal(7, g->read_hidden())
    t |> equal(true, read_hidden_flag(make_hidden()))
}
