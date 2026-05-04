// cdp/actions.das — Module that requires cross-directory dependencies
//
// Tests the resolver's ability to handle dotted module names (common.metalink,
// common.action_utils) which map to subdirectory paths. Also requires `fio`
// (a built-in module), testing that the resolver correctly falls through to
// the default resolution for built-in modules.

options gen2
require common.metalink
require common.action_utils

require daslib/fio

def fn_actions {
    return true
}
