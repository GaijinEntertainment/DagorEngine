// Regression test for invalid_empty_name (30296) on Variable inside ExprLet.
// This is the EXACT shape that bit PR D3: `Foo(field := value)` in a
// named-arg struct constructor silently dropped the field, propagating an
// empty string `resBindName` into a macro-built `Variable(name := "")`,
// emitting `var :tuple<...> = ...` (nameless var) and triggering
// nonsensical "tuple<X> vs tuple<Y>" type errors downstream.
//
// The validator now catches this at preVisitLet so it surfaces with
// a clear diagnostic instead. No cascading error — the empty variable
// declaration is rejected before any further inference touches it.
options gen2
expect 30296

require _helper_empty_names

[export]
def main {
    let _ = make_empty_let_var()
}
