options gen2
options indenting = 4

require dastest/testing_boost public

// no gc, no debugger: the program carries no frame positions and the intrinsic says so

[test]
def test_frame_position_nogc(t : T?) {
    t |> run("frame_position is 0 without positions") @(t : T?) {
        t |> equal(frame_position(), 0)
    }
}
