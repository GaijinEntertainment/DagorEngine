options gen2

require daslib/linq
require daslib/linq_boost
require daslib/linq_fold
require dastest/testing_boost public

// PR A R3c — Source × terminator matrix for buffer-emitting splice emit fns.
//
// For each chain shape that goes through a buffer-emitting emit fn, the splice must produce a result
// whose static type matches the un-spliced chain. The full matrix is:
//
//                     │ no terminator (chain ends bare)  │ explicit `.to_array()` terminator
//   ──────────────────┼──────────────────────────────────┼──────────────────────────────────
//   iter source       │ iterator<T>                       │ array<T>
//   (`.to_sequence()`)│                                   │
//   array source      │ iterator<T>                       │ array<T>
//   (`each(arr)`)     │                                   │
//
// The `iter source → iterator<T>` and `array source → iterator<T>` rows are the regression coverage
// for R1/R2/R3 — `buffer_return(name, ctx.expr_is_iterator)`. The two array<T> corners protect against
// future over-correction (always wrapping with `to_sequence_move`). `typeinfo typename(got)` is the
// load-bearing assertion. Array-bound tests use `let got` (immutable, no consuming for-loop) and the
// assertion strings reflect the resulting `array<int> const` binding type.
//
// Shapes covered per matrix:
//   1. `reverse()` only             → emit_reverse_buffer_inplace (catch-all R1-R4)
//   2. `distinct()` / `distinct_by` → emit_hashtable_dedup
//   3. `reverse() + distinct()`     → emit_reverse_backward_walk_dset_gate (array path, R-2a)
//                                  /  emit_hashtable_dedup (iter path)
//   4. `reverse() + take(N)`        → emit_reverse_backward_index_walk (array path, R6)
//                                  /  emit_reverse_buffer_inplace (iter path)

// ═════════════════════════════════════════════════════════════════════════════
// 1. reverse() — matrix
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_matrix_reverse_iter_to_iter(t : T?) {
    t |> run("reverse(): iter src, no to_array → iterator<int>") @(tt : T?) {
        var got <- _fold([1, 2, 3, 4, 5].to_sequence() |> reverse())
        tt |> equal(typeinfo typename(got), "iterator<int>")
        var total = 0
        for (v in got) {
            total += v
        }
        tt |> equal(total, 15)
    }
}

[test]
def test_matrix_reverse_iter_to_array(t : T?) {
    t |> run("reverse(): iter src + to_array() → array<int> const") @(tt : T?) {
        let got <- _fold([1, 2, 3, 4, 5].to_sequence() |> reverse() |> to_array())
        tt |> equal(typeinfo typename(got), "array<int> const")
        tt |> equal(length(got), 5)
        tt |> equal(got[0], 5)
        tt |> equal(got[4], 1)
    }
}

[test]
def test_matrix_reverse_array_to_iter(t : T?) {
    t |> run("reverse(): array src, no to_array → iterator<int>") @(tt : T?) {
        let arr = [1, 2, 3, 4, 5]
        var got <- _fold(each(arr) |> reverse())
        tt |> equal(typeinfo typename(got), "iterator<int>")
        var total = 0
        for (v in got) {
            total += v
        }
        tt |> equal(total, 15)
    }
}

[test]
def test_matrix_reverse_array_to_array(t : T?) {
    t |> run("reverse(): array src + to_array() → array<int> const") @(tt : T?) {
        let arr = [1, 2, 3, 4, 5]
        let got <- _fold(each(arr) |> reverse() |> to_array())
        tt |> equal(typeinfo typename(got), "array<int> const")
        tt |> equal(length(got), 5)
        tt |> equal(got[0], 5)
        tt |> equal(got[4], 1)
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 2. distinct() / distinct_by — matrix
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_matrix_distinct_iter_to_iter(t : T?) {
    t |> run("distinct(): iter src, no to_array → iterator<int>") @(tt : T?) {
        var got <- _fold([1, 2, 1, 3, 2, 3, 1].to_sequence() |> distinct())
        tt |> equal(typeinfo typename(got), "iterator<int>")
        var seen : array<int>
        for (v in got) {
            seen |> push(v)
        }
        tt |> equal(length(seen), 3)
    }
}

[test]
def test_matrix_distinct_iter_to_array(t : T?) {
    t |> run("distinct(): iter src + to_array() → array<int> const") @(tt : T?) {
        let got <- _fold([1, 2, 1, 3, 2, 3, 1].to_sequence() |> distinct() |> to_array())
        tt |> equal(typeinfo typename(got), "array<int> const")
        tt |> equal(length(got), 3)
    }
}

[test]
def test_matrix_distinct_array_to_iter(t : T?) {
    t |> run("distinct(): array src, no to_array → iterator<int>") @(tt : T?) {
        let arr = [1, 2, 1, 3, 2, 3, 1]
        var got <- _fold(each(arr) |> distinct())
        tt |> equal(typeinfo typename(got), "iterator<int>")
        var seen : array<int>
        for (v in got) {
            seen |> push(v)
        }
        tt |> equal(length(seen), 3)
    }
}

[test]
def test_matrix_distinct_array_to_array(t : T?) {
    t |> run("distinct(): array src + to_array() → array<int> const") @(tt : T?) {
        let arr = [1, 2, 1, 3, 2, 3, 1]
        let got <- _fold(each(arr) |> distinct() |> to_array())
        tt |> equal(typeinfo typename(got), "array<int> const")
        tt |> equal(length(got), 3)
    }
}

[test]
def test_matrix_distinct_by_iter_to_iter(t : T?) {
    t |> run("distinct_by(_): iter src, no to_array → iterator<int>") @(tt : T?) {
        var got <- _fold([10, 21, 32, 11, 23, 30, 13].to_sequence()._distinct_by(_ % 10))
        tt |> equal(typeinfo typename(got), "iterator<int>")
        var count = 0
        for (_v in got) {
            count ++
        }
        tt |> equal(count, 4)   // mod-10 keys: {0,1,2,3} from 10,21,32,_,23,_,_
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 3. reverse() + distinct() — matrix
//   Array path goes through emit_reverse_backward_walk_dset_gate (R-2a, single backward walk).
//   Iter path falls through to emit_hashtable_dedup with an upstream reverse iterator.
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_matrix_reverse_distinct_iter_to_iter(t : T?) {
    t |> run("reverse() + distinct(): iter src, no to_array → iterator<int>") @(tt : T?) {
        var got <- _fold([3, 1, 2, 1, 3, 2].to_sequence() |> reverse() |> distinct())
        tt |> equal(typeinfo typename(got), "iterator<int>")
        var collected : array<int>
        for (v in got) {
            collected |> push(v)
        }
        // reverse → [2,3,1,2,1,3]; distinct first-seen → [2,3,1]
        tt |> equal(length(collected), 3)
        tt |> equal(collected[0], 2)
    }
}

[test]
def test_matrix_reverse_distinct_iter_to_array(t : T?) {
    t |> run("reverse() + distinct(): iter src + to_array() → array<int> const") @(tt : T?) {
        let got <- _fold([3, 1, 2, 1, 3, 2].to_sequence() |> reverse() |> distinct() |> to_array())
        tt |> equal(typeinfo typename(got), "array<int> const")
        tt |> equal(length(got), 3)
        tt |> equal(got[0], 2)
    }
}

[test]
def test_matrix_reverse_distinct_array_to_iter(t : T?) {
    t |> run("reverse() + distinct(): array src, no to_array → iterator<int>") @(tt : T?) {
        let arr = [3, 1, 2, 1, 3, 2]
        var got <- _fold(each(arr) |> reverse() |> distinct())
        tt |> equal(typeinfo typename(got), "iterator<int>")
        var collected : array<int>
        for (v in got) {
            collected |> push(v)
        }
        tt |> equal(length(collected), 3)
        tt |> equal(collected[0], 2)
    }
}

[test]
def test_matrix_reverse_distinct_array_to_array(t : T?) {
    t |> run("reverse() + distinct(): array src + to_array() → array<int> const") @(tt : T?) {
        let arr = [3, 1, 2, 1, 3, 2]
        let got <- _fold(each(arr) |> reverse() |> distinct() |> to_array())
        tt |> equal(typeinfo typename(got), "array<int> const")
        tt |> equal(length(got), 3)
        tt |> equal(got[0], 2)
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// 4. reverse() + take(N) — matrix
//   Array path goes through emit_reverse_backward_index_walk (R6, visits only last N indices).
//   Iter path falls through to emit_reverse_buffer_inplace (full buffer + reverse_inplace + resize).
// ═════════════════════════════════════════════════════════════════════════════

[test]
def test_matrix_reverse_take_iter_to_iter(t : T?) {
    t |> run("reverse() + take(N): iter src, no to_array → iterator<int>") @(tt : T?) {
        var got <- _fold([10, 20, 30, 40, 50].to_sequence() |> reverse() |> take(2))
        tt |> equal(typeinfo typename(got), "iterator<int>")
        var collected : array<int>
        for (v in got) {
            collected |> push(v)
        }
        tt |> equal(length(collected), 2)
        tt |> equal(collected[0], 50)
        tt |> equal(collected[1], 40)
    }
}

[test]
def test_matrix_reverse_take_iter_to_array(t : T?) {
    t |> run("reverse() + take(N): iter src + to_array() → array<int> const") @(tt : T?) {
        let got <- _fold([10, 20, 30, 40, 50].to_sequence() |> reverse() |> take(2) |> to_array())
        tt |> equal(typeinfo typename(got), "array<int> const")
        tt |> equal(length(got), 2)
        tt |> equal(got[0], 50)
        tt |> equal(got[1], 40)
    }
}

[test]
def test_matrix_reverse_take_array_to_iter(t : T?) {
    t |> run("reverse() + take(N): array src, no to_array → iterator<int>") @(tt : T?) {
        let arr = [10, 20, 30, 40, 50]
        var got <- _fold(each(arr) |> reverse() |> take(2))
        tt |> equal(typeinfo typename(got), "iterator<int>")
        var collected : array<int>
        for (v in got) {
            collected |> push(v)
        }
        tt |> equal(length(collected), 2)
        tt |> equal(collected[0], 50)
        tt |> equal(collected[1], 40)
    }
}

[test]
def test_matrix_reverse_take_array_to_array(t : T?) {
    t |> run("reverse() + take(N): array src + to_array() → array<int> const") @(tt : T?) {
        let arr = [10, 20, 30, 40, 50]
        let got <- _fold(each(arr) |> reverse() |> take(2) |> to_array())
        tt |> equal(typeinfo typename(got), "array<int> const")
        tt |> equal(length(got), 2)
        tt |> equal(got[0], 50)
        tt |> equal(got[1], 40)
    }
}
