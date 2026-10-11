options gen2

module _inline_counter_helper public

// fixture for the cross-module inlineTempIndex bump (inline_counter_cross_module.das):
// the interior splice below mints _inl0_* temps INSIDE cnt_outer's body, so a
// cross-module splice of cnt_outer must mint its own temps at a bumped id

var private g_ticks = 0

def private bump() : int {
    g_ticks++
    return g_ticks
}

[inline]
def private cnt_inner(v : int) : int {
    return v + v
}

[inline]
def public cnt_outer(v : int) : int {
    return cnt_inner(v + bump())
}
