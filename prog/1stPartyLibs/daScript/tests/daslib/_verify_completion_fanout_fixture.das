options gen2
options indenting = 4

require daslib/validate_code

//! Fan-out fixture for the `[verify_completion]` recursion walk: every level calls both
//! functions of the level below, so a walk without a visited-memo explores 2^N paths
//! while a memoized walk sees each function once. `[never_inline]` keeps the inliner
//! from collapsing the graph before the annotation ever runs.

[never_inline]
def leaf0a() : int {
    return 1
}

[never_inline]
def leaf0b() : int {
    return 2
}

[never_inline]
def leaf1a() : int {
    return leaf0a() + leaf0b()
}

[never_inline]
def leaf1b() : int {
    return leaf0a() + leaf0b()
}

[never_inline]
def leaf2a() : int {
    return leaf1a() + leaf1b()
}

[never_inline]
def leaf2b() : int {
    return leaf1a() + leaf1b()
}

[never_inline]
def leaf3a() : int {
    return leaf2a() + leaf2b()
}

[never_inline]
def leaf3b() : int {
    return leaf2a() + leaf2b()
}

[never_inline]
def leaf4a() : int {
    return leaf3a() + leaf3b()
}

[never_inline]
def leaf4b() : int {
    return leaf3a() + leaf3b()
}

[never_inline]
def leaf5a() : int {
    return leaf4a() + leaf4b()
}

[never_inline]
def leaf5b() : int {
    return leaf4a() + leaf4b()
}

[never_inline]
def leaf6a() : int {
    return leaf5a() + leaf5b()
}

[never_inline]
def leaf6b() : int {
    return leaf5a() + leaf5b()
}

[never_inline]
def leaf7a() : int {
    return leaf6a() + leaf6b()
}

[never_inline]
def leaf7b() : int {
    return leaf6a() + leaf6b()
}

[never_inline]
def leaf8a() : int {
    return leaf7a() + leaf7b()
}

[never_inline]
def leaf8b() : int {
    return leaf7a() + leaf7b()
}

[never_inline]
def leaf9a() : int {
    return leaf8a() + leaf8b()
}

[never_inline]
def leaf9b() : int {
    return leaf8a() + leaf8b()
}

[never_inline]
def leaf10a() : int {
    return leaf9a() + leaf9b()
}

[never_inline]
def leaf10b() : int {
    return leaf9a() + leaf9b()
}

[never_inline]
def leaf11a() : int {
    return leaf10a() + leaf10b()
}

[never_inline]
def leaf11b() : int {
    return leaf10a() + leaf10b()
}

[never_inline]
def leaf12a() : int {
    return leaf11a() + leaf11b()
}

[never_inline]
def leaf12b() : int {
    return leaf11a() + leaf11b()
}

[never_inline]
def leaf13a() : int {
    return leaf12a() + leaf12b()
}

[never_inline]
def leaf13b() : int {
    return leaf12a() + leaf12b()
}

[never_inline]
def leaf14a() : int {
    return leaf13a() + leaf13b()
}

[never_inline]
def leaf14b() : int {
    return leaf13a() + leaf13b()
}

[never_inline]
def leaf15a() : int {
    return leaf14a() + leaf14b()
}

[never_inline]
def leaf15b() : int {
    return leaf14a() + leaf14b()
}

[never_inline]
def leaf16a() : int {
    return leaf15a() + leaf15b()
}

[never_inline]
def leaf16b() : int {
    return leaf15a() + leaf15b()
}

[never_inline]
def leaf17a() : int {
    return leaf16a() + leaf16b()
}

[never_inline]
def leaf17b() : int {
    return leaf16a() + leaf16b()
}

[never_inline]
def leaf18a() : int {
    return leaf17a() + leaf17b()
}

[never_inline]
def leaf18b() : int {
    return leaf17a() + leaf17b()
}

[never_inline]
def leaf19a() : int {
    return leaf18a() + leaf18b()
}

[never_inline]
def leaf19b() : int {
    return leaf18a() + leaf18b()
}

[never_inline]
def leaf20a() : int {
    return leaf19a() + leaf19b()
}

[never_inline]
def leaf20b() : int {
    return leaf19a() + leaf19b()
}

[never_inline]
def leaf21a() : int {
    return leaf20a() + leaf20b()
}

[never_inline]
def leaf21b() : int {
    return leaf20a() + leaf20b()
}

[never_inline]
def leaf22a() : int {
    return leaf21a() + leaf21b()
}

[never_inline]
def leaf22b() : int {
    return leaf21a() + leaf21b()
}

[verify_completion]
def fanout_root() : int {
    return leaf22a() + leaf22b()
}

[export]
def main {
    feint("{fanout_root()}")
}
