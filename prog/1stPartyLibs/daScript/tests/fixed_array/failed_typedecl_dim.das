// Stage 0 characterization (FIXED_ARRAY_REWORK.md): typedecl(...) can't be an array
// base type — the parser rejects it (dims and the expression payload shared dimExpr
// historically; the restriction is preserved by the rework).
options gen2
expect 30112

[export]
def main {
    var x = 5
    var a : typedecl(x)[3]
}
