options gen2
options auto_inline_functions = true
require dastest/testing_boost
require _inline_shadow_helper

// A spliced block literal declares ARGUMENTS in the caller's scope chain, and infer re-resolves
// every ExprVar by name. Unless the splice renames them, a substituted argument expression that
// spells the same name captures the block's variable instead: a plain block reports 30701 at the
// callee, and a can_shadow block (host query blocks, linq_boost) is silently wrong.

[never_inline] def apply_team(blk : block<(team : int) : int>) : int { return invoke(blk, 5) }

def callee(squad_team : int) : int {
    return apply_team() $ [shadowable] (team : int) : int {
        return squad_team * 100 + team
    }
}

def caller(team : int) : int { return callee(team) }

// the explicit [inline] flavor of this shape is a hard error by design (30100) - the can_shadow
// argument name is semantic, so the capture cannot be renamed away and the demanded splice must
// be refused; pinned in failed_inline_shadow_capture.das

// same shape without can_shadow: the capture is a compile error (30701) rather than a wrong value,
// so this function failing to rename does not compile at all
def plain_callee(squad_team : int) : int {
    return apply_team() $(team : int) : int {
        return squad_team * 100 + team
    }
}

def plain_caller(team : int) : int { return plain_callee(team) }

[test]
def test_no_capture_when_inlined(t : T?) {
    t |> equal(caller(7), 705)
}

[test]
def test_no_capture_without_can_shadow(t : T?) {
    t |> equal(plain_caller(7), 705)
}
