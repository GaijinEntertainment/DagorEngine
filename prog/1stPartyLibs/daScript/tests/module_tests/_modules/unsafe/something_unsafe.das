// unsafe/something_unsafe.das — Module with implicit parameter
//
// Declares a `module something_unsafe` so it has a non-empty module name.
// The project's module_allowed_unsafe() returns true for non-empty names,
// which allows this module to use the `implicit` parameter qualifier.
//
// `implicit` parameters are automatically passed from the caller's scope
// when not explicitly provided — this is an unsafe feature because it can
// lead to unexpected behavior if the caller doesn't realize a variable is
// being implicitly captured.

options gen2
module something_unsafe

def public implicit_feint(a : string implicit) {
    feint(a)
}
