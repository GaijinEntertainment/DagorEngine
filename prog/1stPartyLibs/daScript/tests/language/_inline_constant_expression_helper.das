options gen2

module _inline_constant_expression_helper public

require daslib/constant_expression

// fixture for the locked-name dispatch fence (ast_inline.cpp PrivateUseScan::checkName):
// [constant_expression] clones the resolved instance into the module that first sees the
// call and dispatches it as `__::<instance-name>`constant_expression`<hash>`. The clone
// carries no fromGeneric (Function::clone does not copy it), so nothing re-resolves that
// locked name from another module - a block-taking wrapper spliced into the test module
// would leave the call unresolvable there and break compilation outright

[generic, constant_expression(name), unused_argument(name)]
def pick_by_name(x : int; name : string) {
    static_if (!typeinfo is_argument(name)) {
        static_if (name == "a") {
            return x + 1
        } else {
            return x + 2
        }
    }
}

// the block argument is what makes a caller splice this body into ITS module
def public with_picked(x : int; cb : block<(v : int) : void>) {
    invoke(cb, pick_by_name(x, "a"))
}
