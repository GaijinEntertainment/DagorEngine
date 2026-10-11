// The explicit-element-type ctor literal — array<Base?>(new Derived(), ...) — must
// upcast each element to the declared element type in every tier.
options gen2
require dastest/testing_boost public

class Node {
    def kind() : int => 1
}

class LeafNode : Node {
    def override kind() : int => 2
}

[test]
def test_ctor_literal_pointer_upcast(t : T?) {
    t |> run("derived instances land upcast in a base-pointer array") @(t : T?) {
        var arr <- array<Node?>(new LeafNode(), new Node())
        t |> equal(length(arr), 2)
        t |> equal(arr[0]->kind(), 2)
        t |> equal(arr[1]->kind(), 1)
        unsafe {
            delete arr
        }
    }
}
