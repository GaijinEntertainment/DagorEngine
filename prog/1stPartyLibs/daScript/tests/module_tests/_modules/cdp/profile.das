// cdp/profile.das — Root-level profile module
//
// This module is required by common/action_utils.das. When action_utils
// (located in common/) requires "profile", the custom resolver in
// project.das_project routes it to DAS_PAK_ROOT + "profile.das" (this file)
// instead of resolving relative to common/. This tests the project's ability
// to override resolution for specific module names.

options gen2
require common.metalink

def fn_profile {
    return true
}
