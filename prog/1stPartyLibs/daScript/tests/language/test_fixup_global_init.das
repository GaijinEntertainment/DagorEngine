options gen2
options indenting = 4

// Pipeline contract test: a global init installed by an annotation's fixup() — after infer is
// over — still resolves and evaluates. fixup-set inits get a gated dirty re-infer in
// compileDaScript (ast_parse.cpp), positioned before foldUnsafe so the re-infer cannot re-trip
// already-folded unsafe. This is the mechanism dasSpirv's shader-blob capture (and any future
// backend emitter) relies on; without it the call-shaped init fails 50607.

require dastest/testing_boost public
require _fixup_init_macro

[fixup_init_capture(name="fixup_captured")]
def annotated_stub {
    pass
}

[test]
def test_fixup_global_init(t : T?) {
    t |> run("call-shaped global init set in annotation fixup resolves + evaluates") <| @(t : T?) {
        t |> equal(fixup_captured |> length, 4)
        t |> equal(fixup_captured[0], 7u)
        t |> equal(fixup_captured[1], 11u)
        t |> equal(fixup_captured[2], 13u)
        t |> equal(fixup_captured[3], 0u)   // annotated_stub arity, read at fixup time
    }
}
