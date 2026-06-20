// options/main_blocked.das — Script that should COMPILE despite a blocked option
//
// `options persistent_heap` is blocked by the project policy: it is tolerated in
// source but silently ignored (not applied). Compilation and execution succeed.
// `options indenting` is allowed and not blocked — it serves as the control the
// test asserts IS applied, so the "persistent_heap absent" check has teeth.

options gen2
options indenting = 4
options persistent_heap

[export]
def test {
    return true
}
