options gen2
require _inline_shadow_helper
// an explicit [inline] callee whose substituted argument expression would be captured by a
// can_shadow block argument is a hard error: the argument name is semantic (a host ECS resolves
// the query component by it), so the capture cannot be renamed away, and the author demanded the
// splice - refusing silently is not an option. The auto-inline flavor declines silently instead
// (inline_shadow_capture.das).
expect 30100

[never_inline] def apply_team(blk : block<(team : int) : int>) : int { return invoke(blk, 5) }

[inline] def must_callee(squad_team : int) : int {
    return apply_team() $ [shadowable] (team : int) : int {
        return squad_team * 100 + team
    }
}

def must_caller(team : int) : int { return must_callee(team) }

[export]
def main {
    print("{must_caller(7)}\n")
}
