options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// Regression coverage for ``TemplateVisitor`` peel-aware substitution.
// ``replaceVariable`` is the single unified rule — it handles bare ``ExprVar``
// substitution AND ``ExprRef2Value(ExprVar)`` peel-and-substitute.
//
// Pre-fix bug (caught by Copilot on PR #2817): the gate used to suppress
// ``visitExprVar`` substitution under ANY ``ExprRef2Value`` wrapper. That
// broke the nested case below, where the wrapper is around a field access
// (not directly the var) — ``visitExprRef2Value``'s peel only fires when
// ``subexpr is ExprVar``, so the inner var lost its substitution.

// ── 1. Bare-var substitution (no wrapper) ──────────────────────────────────
[test]
def test_substitutes_bare_var(t : T?) {
    ast_gc_guard() {
        var bare : Expression? = new ExprVar(at = LineInfo(), name := "x")
        var tmpl : Template
        var replacement : Expression? = new ExprVar(at = LineInfo(), name := "REPLACEMENT")
        tmpl |> replaceVariable("x", replacement)
        bare = apply_template(tmpl, bare.at, bare)
        t |> success(bare is ExprVar, "result is ExprVar")
        t |> equal(string((bare as ExprVar).name), "REPLACEMENT", "bare x substituted")
    }
}

// ── 2. Wrapper around bare ExprVar — wrapper stripped, substitution fires ──
[test]
def test_strips_r2v_wrapper_around_bare_var(t : T?) {
    ast_gc_guard() {
        var bare : Expression? = new ExprVar(at = LineInfo(), name := "x")
        var wrapped : Expression? = new ExprRef2Value(at = LineInfo(), subexpr = bare)
        var tmpl : Template
        var replacement : Expression? = new ExprVar(at = LineInfo(), name := "REPLACEMENT")
        tmpl |> replaceVariable("x", replacement)
        wrapped = apply_template(tmpl, wrapped.at, wrapped)
        t |> success(wrapped is ExprVar, "R2V wrapper stripped, result is bare ExprVar")
        t |> equal(string((wrapped as ExprVar).name), "REPLACEMENT", "x substituted")
    }
}

// ── 3. THE COPILOT REGRESSION CASE: wrapper around field access ────────────
// R2V(ExprField(ExprVar(x), "f")) — wrapper is around the field access, not
// directly the var. visitExprRef2Value's peel won't fire (subexpr isn't
// ExprVar). The inner ExprVar(x) must still substitute via visitExprVar.
[test]
def test_substitutes_var_under_r2v_around_field(t : T?) {
    ast_gc_guard() {
        var inner_var = new ExprVar(at = LineInfo(), name := "x")
        var fld : Expression? = new ExprField(at = LineInfo(), value = inner_var, name := "field")
        var wrapped : Expression? = new ExprRef2Value(at = LineInfo(), subexpr = fld)
        var tmpl : Template
        var replacement : Expression? = new ExprVar(at = LineInfo(), name := "REPLACEMENT")
        tmpl |> replaceVariable("x", replacement)
        wrapped = apply_template(tmpl, wrapped.at, wrapped)
        // Expected: R2V(ExprField(ExprVar(REPLACEMENT), "field"))
        t |> success(wrapped is ExprRef2Value, "outer R2V preserved (not a peel target)")
        let inner = (wrapped as ExprRef2Value).subexpr
        t |> success(inner is ExprField, "field access preserved")
        let post_field_value = (inner as ExprField).value
        t |> success(post_field_value is ExprVar, "field.value is substituted ExprVar")
        t |> equal(string((post_field_value as ExprVar).name), "REPLACEMENT",
                   "x substituted inside field-under-R2V (regression: gate must allow nested-var substitution)")
    }
}

// ── 4. Non-matching var: wrapper + var stay as-is ──────────────────────────
[test]
def test_no_substitute_when_name_does_not_match(t : T?) {
    ast_gc_guard() {
        var bare : Expression? = new ExprVar(at = LineInfo(), name := "other")
        var wrapped : Expression? = new ExprRef2Value(at = LineInfo(), subexpr = bare)
        var tmpl : Template
        var replacement : Expression? = new ExprVar(at = LineInfo(), name := "REPLACEMENT")
        tmpl |> replaceVariable("x", replacement)  // rule for "x", not "other"
        wrapped = apply_template(tmpl, wrapped.at, wrapped)
        t |> success(wrapped is ExprRef2Value, "wrapper unchanged (no matching rule)")
        let inner = (wrapped as ExprRef2Value).subexpr
        t |> success(inner is ExprVar, "inner var unchanged")
        t |> equal(string((inner as ExprVar).name), "other", "name unchanged")
    }
}
