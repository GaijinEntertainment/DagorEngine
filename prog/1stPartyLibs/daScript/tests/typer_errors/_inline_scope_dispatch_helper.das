// helper for _inline_scope_dispatch.das / _inline_scope_in_with.das: an [inline]
// body with user-spelled `_::` dispatch binds the PROGRAM module - it means this
// module here, but would re-bind to the caller's after a cross-module splice
options gen2

module _inline_scope_dispatch_helper public

def help_scale(x : int) : int => x * 2

[inline]
def dispatched(x : int) : int {
    return _::help_scale(x) + 1
}
