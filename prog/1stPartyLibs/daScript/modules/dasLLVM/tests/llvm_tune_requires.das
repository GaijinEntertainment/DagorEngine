options gen2

require dastest/testing_boost
require llvm/daslib/llvm_tune

// The per-ISA fallback rail: `requires=` feature expressions and the `;`-chain that lets ONE
// annotation ship a different default per target. Nothing here asserts WHICH features this box
// has — only the grammar and the selection rule, so it holds on every CI lane.

[test]
def test_empty_requires_always_passes(t : T?) {
    t |> success(tune_requires_ok(""))
}

[test]
def test_unknown_feature_never_passes(t : T?) {
    // fail-closed: a kernel gated on a name nothing recognizes must not be selected
    t |> success(!tune_requires_ok("no_such_feature_xyz"))
}

[test]
def test_or_alternatives(t : T?) {
    // `|` is an OR — a real feature on either side carries the term
    t |> success(!tune_requires_ok("no_such_feature_xyz|also_bogus"))
    t |> equal(tune_requires_ok("no_such_feature_xyz|neon"), tune_requires_ok("neon"))
}

[test]
def test_and_conjunction(t : T?) {
    // `,` is an AND — one failing term fails the whole expression
    t |> success(!tune_requires_ok("neon,no_such_feature_xyz"))
    t |> success(!tune_requires_ok("no_such_feature_xyz,neon"))
}

[test]
def test_plain_chain_is_the_first_entry(t : T?) {
    // no `:` anywhere means an ordered list with no gating — backward compatible with a bare name
    t |> equal(tune_pick_fallback("vec8_u2"), "vec8_u2")
    t |> equal(tune_pick_fallback("vec16;vec8_u2"), "vec16")
}

[test]
def test_gated_entry_is_skipped_when_it_cannot_pass(t : T?) {
    t |> equal(tune_pick_fallback("vec16:no_such_feature_xyz;vec8_u2"), "vec8_u2")
    t |> equal(tune_pick_fallback("a:no_such_feature_xyz;b:also_bogus;c"), "c")
}

[test]
def test_chain_with_nothing_eligible_picks_nothing(t : T?) {
    // "" lets the caller fall through to its own default rather than inventing a perm
    t |> equal(tune_pick_fallback("a:no_such_feature_xyz"), "")
    t |> equal(tune_pick_fallback(""), "")
}

[test]
def test_gated_entry_wins_when_its_feature_is_present(t : T?) {
    // neon is baseline on arm64 and absent on x64, so this asserts the SELECTION rule against
    // whatever the host answers rather than against a fixed architecture
    let expected = tune_requires_ok("neon") ? "armrow" : "generic"
    t |> equal(tune_pick_fallback("armrow:neon;generic"), expected)
}

[test]
def test_earlier_entry_wins_over_later(t : T?) {
    t |> equal(tune_pick_fallback("first;second"), "first")
    t |> equal(tune_pick_fallback("first:no_such_feature_xyz;second;third"), "second")
}
