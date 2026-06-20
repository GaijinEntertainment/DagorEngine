// Regression test for invalid_empty_name (30296) on ExprVar.
// The call_macro splices `ExprVar(name := "")` — typer rejects via
// the preVisit empty-name check, then 30838 fires as the downstream
// "can't locate variable ''" follow-up.
options gen2
expect 30296, 30838

require _helper_empty_names

[export]
def main {
    let _ = make_empty_var_ref()
}
