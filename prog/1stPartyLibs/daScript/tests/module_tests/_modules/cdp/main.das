// cdp/main.das — Entry point for the CDP module resolution test
//
// Enables `log_require = true` so that the compiler feints each module
// resolution step to the log (useful for debugging module_get behavior).
// Requires `actions`, which in turn requires `common.metalink`,
// `common.action_utils`, and `profile` — exercising the custom resolver
// in project.das_project that routes "profile" to DAS_PAK_ROOT.

options gen2
options log_require = true

require actions

[export]
def test {
    feint("in cdp\n")
    return true
}
