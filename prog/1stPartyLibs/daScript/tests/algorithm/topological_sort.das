options gen2
require daslib/algorithm
require daslib/functional
require dastest/testing_boost public

struct Node {
    id     : int
    before : table<int>
}

[test]
def test_topological_sort(t : T?) {
    // 0 -> 1 -> 3
    // 0 -> 2 -> 3
    var nodes <- [Node(
        id=3, before <- {1, 2}), Node(
        id=0), Node(
        id=2, before <- {0}), Node(
        id=1, before <- {0}
    )]
    var res <- topological_sort(nodes)
    t |> success(length(res) == 4, "expecting all the same nodes")
    var good : bool[4]
    for (n, i in res, range(4)) {
        good[n.id] = true
        if (n.id == 0) {
            t |> success(i == 0, "0 is first")
        } elif (n.id == 3) {
            t |> success(i == 3, "3 is last")
        } elif (n.id == 1 || n.id == 2) {
            t |> success(i == 1 || i == 2, "1 or 2 is second and third")
        }
    }
    t |> success(all(good), "all nodes are present")
}
