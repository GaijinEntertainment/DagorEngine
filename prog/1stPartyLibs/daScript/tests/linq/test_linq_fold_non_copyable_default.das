options gen2
require daslib/linq
require daslib/linq_boost
require dastest/testing_boost public

// Regression for PR #2822 Copilot review (commit after `550494bfc`).
//
// emit_default_bind / emit_default_return in daslib/linq_fold.das emit the
// can_copy / clone_to_move branch matching daslib/linq.das:
//   copyable    → let dv = arg            ; return dv
//   non-copyable → var dv <- clone_to_move(arg); return <- clone_to_move(dv)
//
// Prior emit (`let dv = arg` + `return dv`) compile-errored on the prelude
// bind itself for non-copyable types (e.g. array<int>).
//
// Scope: default-return path for last_or_default / single_or_default.
// element_at_or_default's success path (`return $i(valueName)`) still
// plain-returns the element — pre-existing on both paths, separate fix.

[test]
def test_last_or_default_non_copyable(t : T?) {
    t |> run("array<int> default — empty source returns default") @(tt : T?) {
        var src : array<array<int>>
        var dflt : array<int>
        dflt |> push(100)
        dflt |> push(200)
        let r <- _fold(each(src).last_or_default(dflt))
        tt |> equal(length(r), 2, "non-copyable default returned via clone_to_move")
        tt |> equal(r[0], 100)
        tt |> equal(r[1], 200)
    }
}

[test]
def test_single_or_default_non_copyable_empty(t : T?) {
    t |> run("array<int> default — empty source returns default") @(tt : T?) {
        var src : array<array<int>>
        var dflt : array<int>
        dflt |> push(7)
        let r <- _fold(each(src).single_or_default(dflt))
        tt |> equal(length(r), 1)
        tt |> equal(r[0], 7)
    }
}

[test]
def test_single_or_default_non_copyable_multi_match(t : T?) {
    t |> run("array<int> default — multi-match returns default") @(tt : T?) {
        var src : array<array<int>>
        var a : array<int>
        a |> push(1)
        src |> emplace(a)
        var b : array<int>
        b |> push(2)
        src |> emplace(b)
        var dflt : array<int>
        dflt |> push(99)
        // 2 matches → single_or_default returns default
        let r <- _fold(each(src).single_or_default(dflt))
        tt |> equal(length(r), 1, "default returned on 2nd-match")
        tt |> equal(r[0], 99)
    }
}
