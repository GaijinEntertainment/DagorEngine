options gen2
require dastest/testing_boost public

require daslib/bitfield_trait

bitfield Test {
    one
    two
    three
}

[test]
def test_bitfield_traits(t : T?) {
    for (name, bit in each_bit_name(type<Test>), count()) {
        if (bit == 0) {
            t |> equal("one", name)
        } elif (bit == 1) {
            t |> equal("two", name)
        } elif (bit == 2) {
            t |> equal("three", name)
        } else {
            t |> failure("unexpected bitfield value")
        }
    }
}



