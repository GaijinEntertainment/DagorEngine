// cdp/common/action_utils.das — Module that requires "profile" from a subdirectory
//
// This is the key test case for the CDP scenario: action_utils lives in
// common/ but requires "profile". Without the custom resolver in
// project.das_project, the compiler would look for common/profile.das.
// The custom resolver intercepts "profile" and routes it to
// DAS_PAK_ROOT + "profile.das" (the root-level profile.das).

options gen2
require profile

def fn_actionutils {
    return true
}
