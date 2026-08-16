options gen2
// Auto-inline would splice the block into the callee and there would be no runtime block left to
// carry the data. Stands in for a native call boundary -- a host's `query()` is a C++ function and
// never inlines its block argument.
options disable_auto_inline = true

require UnitTest
require dastest/testing_boost

// `[block_ann_data]` (dasUnitTest BlockAnnotationDataAnnotation) writes annotationData +
// annotationDataSid onto the block in finalize, the way a host ECS stashes a baked query
// description on every query block. testBlockAnnotationData reads it back off the runtime Block.
// The two must agree on every execution tier: the interpreter bakes the value into
// SimNode_ClosureBlock, C++ AOT emits __context__->adBySid(sid), and the LLVM JIT calls
// jit_ad_by_sid(sid, ctx) -- three routes to one table, so this test is what keeps them in step.

def probe(blk : block<(x : int) : int>) : uint64 {
    return testBlockAnnotationData(blk)
}

def call_block(blk : block<(x : int) : int>) : int {
    return invoke(blk, 20)
}

[test]
def test_block_annotation_data(t : T?) {
    let ad = probe() $ [block_ann_data] (x : int) : int {
        return x + 2
    }
    t |> equal(ad, testBlockAnnotationDataPayload())
}

// An annotated block still has to run normally -- the data rides along, it does not replace the body.
[test]
def test_annotated_block_still_invokes(t : T?) {
    let r = call_block() $ [block_ann_data] (x : int) : int {
        return x + 2
    }
    t |> equal(r, 22)
}

// A block with no annotation must report 0, or the test above would pass on a stale read.
[test]
def test_unannotated_block_has_no_data(t : T?) {
    let ad = probe() $(x : int) : int {
        return x + 2
    }
    t |> equal(ad, 0ul)
}
