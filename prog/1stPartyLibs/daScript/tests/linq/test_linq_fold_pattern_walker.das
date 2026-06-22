options gen2

require daslib/ast_boost
require daslib/linq_fold
require dastest/testing_boost public

// PR A — pattern-table refactor: walker + prefix-conflict lint tests.
//
// The per-plan pattern tables (`plan_reverse_patterns`, `plan_distinct_patterns`) are populated by
// `[_macro]` functions at MACRO time, not at runtime — the function pointers in the rows reference
// `[macro_function]` emit fns whose bodies the LLVM JIT can't lower (quote() nodes). So at runtime
// these tables read as empty; the lint helpers below are exercised on synthetic tables instead.
// End-to-end pattern selection is covered by the existing test_linq_fold_*.das suite (each user
// chain exercises the emit fns through `_fold`).

[test]
def test_alias_table_resolves(t : T?) {
    t |> run("alias_table: PR A aliases populated (runtime-initialized via literal)") @(tt : T?) {
        tt |> equal(alias_table |> key_exists("distinct_family"), true)
        tt |> equal(alias_table |> key_exists("first_family"), true)
        tt |> equal(alias_table |> key_exists("count_family"), true)
        tt |> equal(alias_table |> key_exists("distinct_terminator_family"), true)
        tt |> equal(length(alias_table["distinct_family"]), 2)
        tt |> equal(length(alias_table["first_family"]), 2)
        tt |> equal(length(alias_table["distinct_terminator_family"]), 3)   // count / long_count / sum — see linq_fold.das alias_table
    }
}

// ─── Prefix-conflict helper: positive case (synthetic patterns we KNOW shadow) ───

[test]
def test_chain_prefix_of_positive(t : T?) {
    t |> run("chain_prefix_of: A is a strict prefix of B when A's slots structurally match B's first N") @(tt : T?) {
        let prefix : array<Slot> <- [
            Slot(matcher = m_literal("reverse"), cardinality = c_one())
        ]
        let longer : array<Slot> <- [
            Slot(matcher = m_literal("reverse"), cardinality = c_one()),
            Slot(matcher = m_literal("take"), cardinality = c_one(), capture_name = "take")
        ]
        tt |> equal(chain_prefix_of(prefix, longer), true)
        tt |> equal(chain_prefix_of(longer, prefix), false)  // longer is not a prefix of shorter
    }
}

[test]
def test_chain_prefix_of_negative(t : T?) {
    t |> run("chain_prefix_of: different matchers at any position break the prefix") @(tt : T?) {
        let a : array<Slot> <- [
            Slot(matcher = m_literal("reverse"), cardinality = c_one()),
            Slot(matcher = m_literal("count"), cardinality = c_one())
        ]
        let b : array<Slot> <- [
            Slot(matcher = m_literal("reverse"), cardinality = c_one()),
            Slot(matcher = m_literal("take"), cardinality = c_one()),
            Slot(matcher = m_literal("first"), cardinality = c_one())
        ]
        tt |> equal(chain_prefix_of(a, b), false)
    }
}

[test]
def test_chain_prefix_of_alias_equality(t : T?) {
    t |> run("chain_prefix_of: alias matchers compare by alias name") @(tt : T?) {
        let a : array<Slot> <- [
            Slot(matcher = m_alias("distinct_family"), cardinality = c_one())
        ]
        let b : array<Slot> <- [
            Slot(matcher = m_alias("distinct_family"), cardinality = c_one()),
            Slot(matcher = m_literal("take"), cardinality = c_opt())
        ]
        let c : array<Slot> <- [
            Slot(matcher = m_alias("first_family"), cardinality = c_one()),
            Slot(matcher = m_literal("take"), cardinality = c_opt())
        ]
        tt |> equal(chain_prefix_of(a, b), true)
        tt |> equal(chain_prefix_of(a, c), false)
    }
}

def null_emit(var c : Captures; var ctx : EmitCtx; at : LineInfo) : Expression? {
    return null
}

// ─── c_chain cardinality (PR B) — constructor + lint-helper structural tests ───
// Semantic coverage of c_chain (empty / all-match / prefix-then-stop / arity gate) is end-to-end
// via tests/linq/test_linq_fold_loop_or_count.das — the walker stays private and is exercised
// through plan_loop_or_count's pattern table just like plan_reverse / plan_distinct in PR A.

[test]
def test_slot_chain_of_constructs_expected_shape(t : T?) {
    t |> run("slot_chain_of: produces c_chain cardinality + one_of matcher + capture_name") @(tt : T?) {
        let s = slot_chain_of(["where_", "select"], "head")
        tt |> equal(s.cardinality is chain, true)
        tt |> equal(s.cardinality is one, false)
        tt |> equal(s.cardinality is optional, false)
        tt |> equal(s.matcher is one_of, true)
        let names & = unsafe(s.matcher as one_of)
        tt |> equal(length(names), 2)
        tt |> equal(names[0], "where_")
        tt |> equal(names[1], "select")
        tt |> equal(s.capture_name, "head")
        tt |> equal(s.arity, -1)
    }
}

[test]
def test_slots_structurally_match_distinguishes_c_chain(t : T?) {
    t |> run("slots_structurally_match: c_chain vs c_one vs c_opt at same matcher are distinct shapes") @(tt : T?) {
        // c_chain ≢ c_one ≢ c_opt at the structural level — chain_prefix_of must reject mixed-cardinality matches.
        let chainOnly : array<Slot> <- [slot_chain_of(["x"], "h")]
        let oneOpt : array<Slot> <- [
            Slot(matcher = SlotMatcher(one_of <- ["x"]), cardinality = c_one(), capture_name = "h"),
            Slot(matcher = SlotMatcher(one_of <- ["x"]), cardinality = c_opt(), capture_name = "h")
        ]
        let chainPair : array<Slot> <- [
            slot_chain_of(["x"], "h"),
            Slot(matcher = SlotMatcher(one_of <- ["x"]), cardinality = c_opt(), capture_name = "h")
        ]
        tt |> equal(chain_prefix_of(chainOnly, oneOpt), false)
        tt |> equal(chain_prefix_of(chainOnly, chainPair), true)
    }
}

[test]
def test_check_pattern_table_reachable_accepts_c_chain(t : T?) {
    t |> run("check_pattern_table_reachable: c_chain head + distinct trailers stay reachable (no shadow)") @(tt : T?) {
        var patterns : array<SplicePattern>
        patterns |> emplace <| SplicePattern(
            name = "chain_then_take",
            chain <- [
                slot_chain_of(["where_", "select"], "head"),
                Slot(matcher = m_literal("take"), cardinality = c_opt(), capture_name = "take")
            ],
            emit = @@ < EmitFn > null_emit
        )
        patterns |> emplace <| SplicePattern(
            name = "chain_then_count",
            chain <- [
                slot_chain_of(["where_", "select"], "head"),
                Slot(matcher = m_literal("count"), cardinality = c_opt(), capture_name = "term")
            ],
            emit = @@ < EmitFn > null_emit
        )
        tt |> equal(check_pattern_table_reachable("synthetic-c_chain-trailers", patterns), true)
    }
}

[test]
def test_check_pattern_table_reachable_catches_shadow(t : T?) {
    t |> run("check_pattern_table_reachable: returns false for a table with a strict-prefix shadow") @(tt : T?) {
        var bad : array<SplicePattern>
        bad |> emplace <| SplicePattern(
            name = "short",
            chain <- [
                Slot(matcher = m_literal("reverse"), cardinality = c_one())
            ],
            emit = @@ < EmitFn > null_emit
        )
        bad |> emplace <| SplicePattern(
            name = "long_shadowed",
            chain <- [
                Slot(matcher = m_literal("reverse"), cardinality = c_one()),
                Slot(matcher = m_literal("take"), cardinality = c_opt())
            ],
            emit = @@ < EmitFn > null_emit
        )
        tt |> equal(check_pattern_table_reachable("synthetic-shadowed", bad), false)
    }
}
