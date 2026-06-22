// A for-loop whose body simulates to exactly ONE SimNode plus a label (a goto target).
// The fused single-statement (`loop1`) for-node reads list[0] directly and cannot service
// jumpToLabel, so node selection must follow the simulated body count AND require no labels:
// a label makes total==1 but must still take the general labeled node, or the in-loop goto
// leaks out (returns garbage) instead of jumping back to the label.
options gen2
require dastest/testing_boost public

def loop_back_to_label : int {
    var counter = 0
    var guard = 0
    for (_i in range(2)) {
        label 0:
        if (guard < 3) {
            guard ++
            counter += 10
            goto 0
        }
    }
    return counter
}

[test]
def test_for_label_body(t : T?) {
    t |> run("in-loop goto to a label in a single-statement body jumps back, not out") @(t : T?) {
        t |> equal(loop_back_to_label(), 30)
    }
}
